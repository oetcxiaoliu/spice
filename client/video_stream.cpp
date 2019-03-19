#include "video_stream.h"

#ifdef WIN32
static int create_bitmap(HDC *dc, HBITMAP *prev_bitmap,
                         uint8_t **data, int *nstride,
                         int width, int height, bool top_down)
{
    HBITMAP bitmap;
    struct {
        BITMAPINFO inf;
        RGBQUAD palette[255];
    } bitmap_info;

    memset(&bitmap_info, 0, sizeof(bitmap_info));
    bitmap_info.inf.bmiHeader.biSize = sizeof(bitmap_info.inf.bmiHeader);
    bitmap_info.inf.bmiHeader.biWidth = width;
    bitmap_info.inf.bmiHeader.biHeight = top_down ? -height : height;

    bitmap_info.inf.bmiHeader.biPlanes = 1;
    bitmap_info.inf.bmiHeader.biBitCount = 32;
    bitmap_info.inf.bmiHeader.biCompression = BI_RGB;
    *nstride = width * 4;

    *dc = create_compatible_dc();
    if (!*dc) {
        return 0;
    }

    bitmap = CreateDIBSection(*dc, &bitmap_info.inf, 0, (void **)data, NULL, 0);
    if (!bitmap) {
        DeleteObject(*dc);
        return 0;
    }

    *prev_bitmap = (HBITMAP)SelectObject(*dc, bitmap);
    return 1;
}

#endif

VideoStream::VideoStream(RedClient& client, Canvas& canvas, DisplayChannel& channel,
                         uint32_t codec_type, bool top_down, uint32_t stream_width,
                         uint32_t stream_height, uint32_t src_width, uint32_t src_height,
                         SpiceRect* dest, int clip_type, uint32_t num_clip_rects,
                         SpiceRect* clip_rects)
    : _client (client)
    , _canvas (canvas)
    , _channel (channel)
    , _decoder (NULL)
    , _stream_width (stream_width)
    , _stream_height (stream_height)
    , _codec_type(codec_type)
    , _stride (stream_width * sizeof(uint32_t))
    , _top_down (top_down)
    , _dest (*dest)
    , _clip (NULL)
    , _frames_head (0)
    , _frames_tail (0)
    , _kill_mark (0)
    , _uncompressed_data (NULL)
    , _update_mark (0)
    , _update_time (0)
    , next (NULL)
{
    memset(_frames, 0, sizeof(_frames));
    region_init(&_clip_region);

    try {
#ifdef WIN32
        if (!create_bitmap(&_dc, &_prev_bitmap, &_uncompressed_data, &_stride,
                           stream_width, stream_height, _top_down)) {
            THROW("create_bitmap failed");
        }
#else
        _uncompressed_data = new uint8_t[_stride * stream_height];
#endif
        _pixmap.width = src_width;
        _pixmap.height = src_height;

        _decoder = new Decoder(_codec_type, stream_width, stream_height, _stride,
                               _uncompressed_data, channel.get_peer_major() == 1);
        
#ifdef WIN32
        SetViewportOrgEx(_dc, 0, stream_height - src_height, NULL);
#endif

    // this doesn't have effect when using gdi_canvas. The sign of BITMAPINFO's biHeight
    // determines the orientation (see create_bitmap).
    if (_top_down) {
        _pixmap.data = _uncompressed_data;
        _pixmap.stride = _stride;
    } else {
        _pixmap.data = _uncompressed_data + _stride * (src_height - 1);
        _pixmap.stride = -_stride;
    }

    set_clip(clip_type, num_clip_rects, clip_rects);
    } catch (...) {
        if (_decoder) {
            delete _decoder;
            _decoder = NULL;
        }

        release_all_bufs();
        throw;
    }
}

VideoStream::~VideoStream()
{
    if (_decoder) {
        delete _decoder;
        _decoder = NULL;
    }

    release_all_bufs();
    region_destroy(&_clip_region);
}

void VideoStream::release_all_bufs()
{
    for (int i = 0; i < MAX_VIDEO_FRAMES; i++) {
        delete[] _frames[i].compressed_data;
    }
#ifdef WIN32
    if (_dc) {
        HBITMAP bitmap = (HBITMAP)SelectObject(_dc, _prev_bitmap);
        DeleteObject(bitmap);
        DeleteObject(_dc);
    }
#else
    delete[] _uncompressed_data;
#endif
}

void VideoStream::free_frame(uint32_t frame_index)
{
    int slot = frame_slot(frame_index);
    delete[] _frames[slot].compressed_data;
    _frames[slot].compressed_data = NULL;
}

void VideoStream::remove_dead_frames(uint32_t mm_time)
{
    while (_frames_head != _frames_tail) {
        if (int(_frames[frame_slot(_frames_tail)].mm_time - mm_time) >= MAX_UNDER) {
            return;
        }
        free_frame(_frames_tail);
        _frames_tail++;
    }
}

void VideoStream::drop_one_frame()
{
    ASSERT(MAX_VIDEO_FRAMES > 2 && (_frames_head - _frames_tail) == MAX_VIDEO_FRAMES);
    unsigned frame_index = _frames_head - _kill_mark++ % (MAX_VIDEO_FRAMES - 2) - 2;

    free_frame(frame_index);

    while (frame_index != _frames_tail) {
        --frame_index;
        _frames[frame_slot(frame_index + 1)] = _frames[frame_slot(frame_index)];
    }
    _frames_tail++;
}

void VideoStream::do_qos(uint32_t mm_time, uint32_t ref_mm_time)
{
    if (mm_time - ref_mm_time >= MAX_UNDER) {
        if (_decoder->_codec_context->skip_frame != AVDISCARD_DEFAULT) {
            _decoder->_codec_context->skip_frame = AVDISCARD_DEFAULT;
        }
        else if (_decoder->_codec_context->skip_frame != AVDISCARD_NONREF) {
            _decoder->_codec_context->skip_frame = AVDISCARD_NONREF;
        }
    }
}

bool VideoStream::is_time_to_display(uint32_t now, uint32_t frame_time)
{
    int delta = frame_time - now;
    return delta <= MAX_OVER && delta >= MAX_UNDER;
}

void VideoStream::maintenance()
{
    uint32_t mm_time = _client.get_mm_time();

    if (!_update_mark && !_update_time && _frames_head != _frames_tail) {
        VideoFrame* tail = &_frames[frame_slot(_frames_tail)];

        ASSERT(tail->compressed_data);
        uint8_t* data = tail->compressed_data;
        uint32_t length = tail->compressed_data_size;
        int got_picture = 0;

        do_qos(tail->mm_time, mm_time);
        got_picture = _decoder->decode_data(data, length);

        if (got_picture) {
#ifdef WIN32
            _canvas.put_image(_dc, _pixmap, _dest, _clip);
#else
            _canvas.put_image(_pixmap, _dest, _clip);
#endif
            if (is_time_to_display(mm_time, tail->mm_time)) {
                _update_mark = _channel.invalidate(_dest, true);
                Platform::yield();
            } else {
                _update_time = tail->mm_time;
                _channel.stream_update_request(_update_time);
            }
        }
        free_frame(_frames_tail++);
    }
}

uint32_t VideoStream::handle_timer_update(uint32_t now)
{
    if (!_update_time) {
        return 0;
    }

    if (is_time_to_display(now, _update_time)) {
        _update_time = 0;
        _update_mark = _channel.invalidate(_dest, true);
    } else if ((int)(_update_time - now) < 0) {
        DBG(0, "to late");
        _update_time = 0;
    }
    return _update_time;
}

void VideoStream::handle_update_mark(uint64_t update_mark)
{
    if (!_update_mark || update_mark < _update_mark) {
        return;
    }
    _update_mark = 0;
    maintenance();
}

uint32_t VideoStream::alloc_frame_slot()
{
    if ((_frames_head - _frames_tail) == MAX_VIDEO_FRAMES) {
        drop_one_frame();
    }
    return frame_slot(_frames_head++);
}

void VideoStream::push_data(uint32_t mm_time, uint32_t length, uint8_t* data)
{
    maintenance();
    uint32_t frame_slot = alloc_frame_slot();
    _frames[frame_slot].compressed_data = new uint8_t[length];
    memcpy(_frames[frame_slot].compressed_data, data, length);
    _frames[frame_slot].compressed_data_size = length;
    _frames[frame_slot].mm_time = mm_time ? mm_time : 1;
    maintenance();
}

void VideoStream::set_clip(int type, uint32_t num_clip_rects, SpiceRect* clip_rects)
{
    if (type == SPICE_CLIP_TYPE_NONE) {
        _clip = NULL;
        return;
    }

    ASSERT(type == SPICE_CLIP_TYPE_RECTS)
    region_clear(&_clip_region);

    for (unsigned int i = 0; i < num_clip_rects; i++) {
        region_add(&_clip_region, &clip_rects[i]);
    }
    _clip = &_clip_region;
}

