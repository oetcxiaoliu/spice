#ifndef __GSREAMER_H_
#define __GSREAMER_H_
#include "utils.h"
#include "region.h"
#include "red_client.h"
#include "display_channel.h"

#define NUM_STREAMS 50
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/videooverlay.h>
#include <queue>
using namespace std;

struct VideoApp;

class VideoGstreamer {
public:
    VideoGstreamer(RedClient& client, DisplayChannel& channel,
                uint32_t codec_type, bool top_down, uint32_t stream_width,
                uint32_t stream_height, uint32_t src_width, uint32_t src_height,
                SpiceRect* dest, int clip_type, uint32_t num_clip_rects, 
                SpiceRect* clip_rects, Window wnd);
    ~VideoGstreamer();

    void push_data(uint32_t mm_time, uint32_t length, uint8_t* data);
    void set_clip(int type, uint32_t num_clip_rects, SpiceRect* clip_rects);

    void init_stream();


private:
    static void* video_player_thread(void *data);     
    static gboolean bus_message (GstBus * bus, GstMessage * message, void *data);
    static gboolean read_data(void *data);
    static void start_feed(GstElement *pipeline, guint size, void *data);
    static void stop_feed (GstElement * pipeline,  void *data);

private:
    RedClient& _client;
    DisplayChannel& _channel;
    int _codec_type;
    int _stream_width;
    int _stream_height;
    SpiceRect _dest;
    QRegion _clip_region;    
    QRegion* _clip;
    bool _top_down;
    Thread* _worker;
    Window _wnd;

public:
    VideoApp *_video_app;
    VideoGstreamer* next;
};
#endif

