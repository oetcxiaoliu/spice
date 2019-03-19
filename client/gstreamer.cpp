#include "gstreamer.h"

#define MAX_FRAME_SIZE 100

GST_DEBUG_CATEGORY_EXTERN(spicec_video_debug);
#define GST_CAT_DEFAULT spicec_video_debug

//#define USE_GSTREAMER_DEBUG

struct VideoApp {
    GstElement *pipeline;
    GstElement *videosource;
    GstElement *videoparse;
    GstElement *videodecoder;
    GstElement *videofliper;
    GstElement *videoconverter;
    GstElement *videosink;
    GMainLoop *loop;
    GstBus *bus;
    guint bus_watch_id;
    guint sourceid;
    Window wnd;
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
    std::queue<GstBuffer *> video_frames;
    pthread_mutex_t frame_lock;
};

VideoGstreamer::VideoGstreamer(RedClient& client, DisplayChannel& channel,
                         uint32_t codec_type, bool top_down, uint32_t stream_width,
                         uint32_t stream_height, uint32_t src_width, uint32_t src_height,
                         SpiceRect* dest, int clip_type, uint32_t num_clip_rects,
                         SpiceRect* clip_rects, Window wnd)
    : _client (client)
    , _channel (channel)
    , _codec_type (codec_type)
    , _stream_width (stream_width)
    , _stream_height (stream_height)
    , _dest (*dest) 
    , _clip (NULL)    
    , _top_down (top_down)
    , _wnd (wnd)
    , next (NULL)
{
    _video_app = new VideoApp;   

#ifdef USE_GSTREAMER_DEBUG
    gst_debug_category_set_threshold(spicec_video_debug, GST_LEVEL_DEBUG);
#endif
    
    GST_DEBUG("create stream, stream width=%d, stream height=%d", stream_width, stream_height);

    /* init  frame locak */
    pthread_mutex_init(&_video_app->frame_lock, NULL);

    /* setup video render window */
    _video_app->wnd = _wnd;

    /* setup video render rect */
    _video_app->top = _dest.top;
    _video_app->left = _dest.left;
    _video_app->width = _dest.right - _dest.left;
    _video_app->height = _dest.bottom -_dest.top; 
    
    region_init(&_clip_region);

    if (codec_type != SPICE_VIDEO_CODEC_TYPE_MJPEG
        && codec_type != SPICE_VIDEO_CODEC_TYPE_H264) {
      THROW("invalid video codec type %u", codec_type);
    }
    set_clip(clip_type, num_clip_rects, clip_rects);
    init_stream();
    _worker = new Thread(VideoGstreamer::video_player_thread, _video_app);
}

VideoGstreamer::~VideoGstreamer()
{
    GstFlowReturn ret;
     
    GST_DEBUG("destory stream");
    region_destroy(&_clip_region);    
    gst_element_set_state(_video_app->pipeline, GST_STATE_NULL); 
    g_signal_emit_by_name(_video_app->videosource, "end-of-stream", &ret);    
    g_main_loop_quit (_video_app->loop);            
    g_source_remove(_video_app->bus_watch_id);
    gst_object_unref(_video_app->bus);
    gst_object_unref(GST_OBJECT(_video_app->pipeline));
    pthread_mutex_destroy(&_video_app->frame_lock);
    if (_worker) {
        _worker->join();
        delete _worker;
        _worker = NULL;
    }
    delete _video_app;
}

gboolean VideoGstreamer::bus_message (GstBus * bus, GstMessage * message, void *data)
{    
    VideoApp *video_app = (VideoApp *)data;
    GST_DEBUG("got message %s, by %s",
        gst_message_type_get_name(GST_MESSAGE_TYPE(message)), GST_MESSAGE_SRC_NAME(message));

    switch (GST_MESSAGE_TYPE (message)) 
    {
        case GST_MESSAGE_ERROR:
            gchar *debug;
            GError *error;
            gst_message_parse_error(message,&error,&debug);
            g_free(debug);
            g_printerr("ERROR from element %s:%s\n", GST_OBJECT_NAME(message->src), error->message);
            g_error_free(error);        
            break;
        case GST_MESSAGE_EOS:
            g_main_loop_quit (video_app->loop);        
            break;
        default:
            break;
    }

    return TRUE;
}

gboolean VideoGstreamer::read_data(void *data)
{
    GstBuffer *buffer;
    GstFlowReturn ret;
    int size;

    VideoApp *video_app = (VideoApp *)data;
    pthread_mutex_lock(&video_app->frame_lock);
    size = video_app->video_frames.size();
    if (size <= 0)
    {        
        pthread_mutex_unlock(&video_app->frame_lock);
        return TRUE;
    }

    buffer = (GstBuffer *)video_app->video_frames.front();
    video_app->video_frames.pop();
    g_signal_emit_by_name(video_app->videosource, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);    
    pthread_mutex_unlock(&video_app->frame_lock);
    if (ret != GST_FLOW_OK)
    {
        return FALSE;
    } 
    GST_DEBUG("read stream data OK");

    return TRUE;
}

void VideoGstreamer::start_feed(GstElement *pipeline, guint size, void *data)
{
    VideoApp *video_app = (VideoApp *)data;
    if (video_app->sourceid == 0) {
        video_app->sourceid = g_idle_add((GSourceFunc)(VideoGstreamer::read_data), video_app);
    }
}

void VideoGstreamer::stop_feed (GstElement * pipeline,  void *data)
{   
    VideoApp *video_app = (VideoApp *)data;
    if (video_app->sourceid != 0) 
    {
        g_source_remove(video_app->sourceid);
        video_app->sourceid = 0;
    }
}

void* VideoGstreamer::video_player_thread(void *data)
{
    VideoApp *video_app = (VideoApp *)data;
    g_main_loop_run(video_app->loop);    
    return NULL;
}

void VideoGstreamer::init_stream()
{
    gboolean ret;

    /* build */
    _video_app->sourceid = 0;
  
    _video_app->loop = g_main_loop_new(NULL, TRUE);
    
    _video_app->pipeline = gst_pipeline_new("video player");
    g_assert (_video_app->pipeline);
 
    _video_app->videosource = gst_element_factory_make ("appsrc", "video source");
    g_assert(_video_app->videosource != NULL);

    if (_codec_type == SPICE_VIDEO_CODEC_TYPE_MJPEG)
    {
        _video_app->videoparse = gst_element_factory_make("jpegparse", "mjpeg parse");
        g_assert(_video_app->videoparse != NULL);

        _video_app->videodecoder = gst_element_factory_make("avdec_mjpeg", "mjpeg decoder");
        g_assert(_video_app->videodecoder != NULL);
    }
    else if (_codec_type == SPICE_VIDEO_CODEC_TYPE_H264)
    {
        _video_app->videoparse = gst_element_factory_make ("h264parse", "h264 parse");
        g_assert(_video_app->videoparse != NULL);

        _video_app->videodecoder = gst_element_factory_make("avdec_h264", "h264 decoder");
        g_assert(_video_app->videodecoder != NULL);
    }   
    if ((!_top_down && _codec_type == SPICE_VIDEO_CODEC_TYPE_MJPEG)
        || (_top_down && _codec_type == SPICE_VIDEO_CODEC_TYPE_H264))
    {
        _video_app->videofliper = gst_element_factory_make("videoflip", "video fliper");
        g_assert(_video_app->videofliper != NULL);

        /* setup videoflip */        
        g_object_set(G_OBJECT(_video_app->videofliper), "method", 5, NULL);
    }

    _video_app->videoconverter = gst_element_factory_make("videoconvert", "video convert");
    g_assert(_video_app->videoconverter != NULL);
          
    _video_app->videosink = gst_element_factory_make("xvimagesink", "video sink");
    g_assert(_video_app->videosink != NULL);

    /* add watch bus */
    _video_app->bus = gst_pipeline_get_bus(GST_PIPELINE(_video_app->pipeline));
    _video_app->bus_watch_id = gst_bus_add_watch(_video_app->bus, (GstBusFunc)(VideoGstreamer::bus_message), _video_app);

    if ((!_top_down && _codec_type == SPICE_VIDEO_CODEC_TYPE_MJPEG)
        || (_top_down && _codec_type == SPICE_VIDEO_CODEC_TYPE_H264))
    {
        gst_bin_add_many(GST_BIN(_video_app->pipeline), _video_app->videosource,
                         _video_app->videoparse, _video_app->videodecoder,
                         _video_app->videoconverter, _video_app->videofliper, 
                         _video_app->videosink, NULL);
        ret = gst_element_link_many(_video_app->videosource, _video_app->videoparse,
                                    _video_app->videodecoder, _video_app->videofliper,
                                    _video_app->videoconverter, _video_app->videosink, NULL);
    }
    else
    {
        gst_bin_add_many(GST_BIN(_video_app->pipeline), _video_app->videosource,
                         _video_app->videoparse, _video_app->videodecoder,
                         _video_app->videoconverter, _video_app->videosink, NULL);
        ret = gst_element_link_many(_video_app->videosource, _video_app->videoparse,
                                    _video_app->videodecoder, _video_app->videoconverter,
                                    _video_app->videosink, NULL);
    }
    g_assert(ret);

    /* setup appsrc */                 
    g_object_set(G_OBJECT(_video_app->videosource), "stream-type", 0, 
                                                   "format", GST_FORMAT_TIME,
                                                   "is-live", true, NULL);
    gst_app_src_set_latency(GST_APP_SRC(_video_app->videosource), -1, 15);
 
    g_signal_connect(_video_app->videosource, "need-data", G_CALLBACK (VideoGstreamer::start_feed), _video_app);
    g_signal_connect (_video_app->videosource, "end-of-stream", G_CALLBACK (VideoGstreamer::stop_feed), _video_app);
  
    /* setup sink */
    g_object_set (_video_app->videosink, "qos", false, "force-aspect-ratio", true, "sync", false, NULL);

    /* play */
    gst_element_set_state (_video_app->pipeline, GST_STATE_PLAYING);

    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(_video_app->videosink), _video_app->wnd);
    gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(_video_app->videosink), 
                                           _video_app->left, _video_app->top,
                                           _video_app->width, _video_app->height);
    gst_video_overlay_expose(GST_VIDEO_OVERLAY(_video_app->videosink));
}

void VideoGstreamer::push_data(uint32_t mm_time, uint32_t length, uint8_t* data)
{
    int size;

    pthread_mutex_lock(&_video_app->frame_lock); 
    size = _video_app->video_frames.size();
    if (size >  MAX_FRAME_SIZE)
    {
        GstBuffer *buffer;
        buffer = (GstBuffer *)_video_app->video_frames.front();
        _video_app->video_frames.pop();
        gst_buffer_unref(buffer); 
    }
    GstBuffer *buffer = gst_buffer_new_allocate(NULL, length, NULL);
    g_assert(buffer != NULL);
    gst_buffer_fill(buffer, 0, data, length);
    gst_buffer_set_size(buffer, length);
    GST_BUFFER_PTS(buffer) = mm_time;
    _video_app->video_frames.push(buffer);
    pthread_mutex_unlock(&_video_app->frame_lock);    
    GST_DEBUG("push stream data");
}

void VideoGstreamer::set_clip(int type, uint32_t num_clip_rects, SpiceRect* clip_rects)
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
