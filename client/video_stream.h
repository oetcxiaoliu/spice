#ifndef _VIDEO_STREAM_H_
#define _VIDEO_STREAM_H_
#define MAX_VIDEO_FRAMES 30
#define MAX_OVER 15
#define MAX_UNDER -15
#include "decoder.h"
#include "region.h"
#include "canvas.h"
#include "red_client.h"
#include "display_channel.h"

class VideoStream {
public:
    VideoStream(RedClient& client, Canvas& canvas, DisplayChannel& channel,
                uint32_t codec_type, bool top_down, uint32_t stream_width,
                uint32_t stream_height, uint32_t src_width, uint32_t src_height,
                SpiceRect* dest, int clip_type, uint32_t num_clip_rects, SpiceRect* clip_rects);
    ~VideoStream();

    void push_data(uint32_t mm_time, uint32_t length, uint8_t* data);
    void set_clip(int type, uint32_t num_clip_rects, SpiceRect* clip_rects);
    const SpiceRect& get_dest() {return _dest;}
    void handle_update_mark(uint64_t update_mark);
    uint32_t handle_timer_update(uint32_t now);

private:
    void free_frame(uint32_t frame_index);
    void release_all_bufs();
    void remove_dead_frames(uint32_t mm_time);
    uint32_t alloc_frame_slot();
    void maintenance();
    void drop_one_frame();
    uint32_t frame_slot(uint32_t frame_index) { return frame_index % MAX_VIDEO_FRAMES;}
    static bool is_time_to_display(uint32_t now, uint32_t frame_time);
	void do_qos(uint32_t mm_time, uint32_t ref_mm_time);

private:
    RedClient& _client;
    Canvas& _canvas;
    DisplayChannel& _channel;
    Decoder *_decoder;
    int _stream_width;
    int _stream_height;
    int _codec_type;
    int _stride;
    bool _top_down;
    SpiceRect _dest;
    QRegion _clip_region;
    QRegion* _clip;

    struct VideoFrame {
        uint32_t mm_time;
        uint32_t compressed_data_size;
        uint8_t* compressed_data;
    };

    uint32_t _frames_head;
    uint32_t _frames_tail;
    uint32_t _kill_mark;
    VideoFrame _frames[MAX_VIDEO_FRAMES];

#ifdef WIN32
    HBITMAP _prev_bitmap;
    HDC _dc;
#endif
    uint8_t *_uncompressed_data;
    PixmapHeader _pixmap;
    uint64_t _update_mark;
    uint32_t _update_time;

public:
    VideoStream* next;
};
#endif
