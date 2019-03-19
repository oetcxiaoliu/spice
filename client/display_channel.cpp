/* -*- Mode: C; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
   Copyright (C) 2009 Red Hat, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "common.h"
#include "canvas.h"
#include "red_pixmap.h"
#ifdef USE_OPENGL
#include "red_pixmap_gl.h"
#endif
#include "debug.h"
#include "utils.h"
#include "common.h"
#include "display_channel.h"
#include "application.h"
#include "screen.h"
#ifdef USE_OPENGL
#include "red_gl_canvas.h"
#endif
#include "red_sw_canvas.h"
#include "red_client.h"
#include "utils.h"
#include "debug.h"
#ifdef WIN32
#include "red_gdi_canvas.h"
#endif
#include "platform_utils.h"
#include "inputs_channel.h"
#include "cursor_channel.h"

#ifdef USE_GSTREAMER
#include "gstreamer.h"
#else
#include "decoder.h"
#include "video_stream.h"
#endif

typedef enum H264CodecPresetType {
    H264_CODEC_PRESET_TYPE_ULTRAFAST = 1,
    H264_CODEC_PRESET_TYPE_SUPERFAST,
    H264_CODEC_PRESET_TYPE_VERYAFAST,
    H264_CODEC_PRESET_TYPE_FASTER,

    H264_CODEC_PRESET_ENUM_END
} H264CodecPresetType;

static char *video_type_to_str(uint32_t type)
{
    switch (type) {
    case SPICE_VIDEO_CODEC_TYPE_MJPEG:
        return (char *)"mjpeg";
    case SPICE_VIDEO_CODEC_TYPE_H264:
        return (char *)"h264";
    default:
        return (char *)"unknown";
    }
}

static char *video_h264_preset_to_str(uint32_t type)
{
    switch (type) {
    case H264_CODEC_PRESET_TYPE_ULTRAFAST:
        return (char *)"ultrafast";
    case H264_CODEC_PRESET_TYPE_SUPERFAST:
        return (char *)"superfast";
    case H264_CODEC_PRESET_TYPE_VERYAFAST:
        return (char *)"veryfast";
    case H264_CODEC_PRESET_TYPE_FASTER:
        return (char *)"faster";
    default:
        return (char *)"unknown";
    }
}

class CreatePrimarySurfaceEvent: public SyncEvent {
public:
   CreatePrimarySurfaceEvent(DisplayChannel& channel, int width, int height, uint32_t format)
        : _channel (channel)
        , _width (width)
        , _height (height)
        , _format (format)
    {
    }

    virtual void do_response(AbstractProcessLoop& events_loop)
    {
        Application* app = (Application*)events_loop.get_owner();
        _channel.screen()->lock_size();
        app->resize_screen(_channel.screen(), _width, _height);
        _channel.create_canvas(0, app->get_canvas_types(), _width, _height, _format);
    }

private:
    DisplayChannel& _channel;
    int _width;
    int _height;
    uint32_t _format;
};

class DestroyPrimarySurfaceEvent: public SyncEvent {
public:
    DestroyPrimarySurfaceEvent(DisplayChannel& channel)
        : _channel (channel)
    {
    }

    virtual void do_response(AbstractProcessLoop& events_loop)
    {
        _channel.destroy_canvas(0);
    }

private:
    DisplayChannel& _channel;
};

class DestroyAllSurfacesEvent: public SyncEvent {
public:
    DestroyAllSurfacesEvent(DisplayChannel& channel, bool include_primary = true)
        : _channel(channel)
        , _include_primary(include_primary)
    {
    }

    virtual void do_response(AbstractProcessLoop& events_loop)
    {
        if (_include_primary) {
            _channel.do_destroy_all_surfaces();
        } else {
            _channel.do_destroy_off_screen_surfaces();
        }
    }

private:
    DisplayChannel& _channel;
    bool _include_primary;
};

class CreateSurfaceEvent: public SyncEvent {
public:
   CreateSurfaceEvent(DisplayChannel& channel, int surface_id, int width, int height,
                      uint32_t format)
        : _channel (channel)
        , _surface_id (surface_id)
        , _width (width)
        , _height (height)
        , _format (format)
    {
    }

    virtual void do_response(AbstractProcessLoop& events_loop)
    {
        Application* app = (Application*)events_loop.get_owner();
        _channel.create_canvas(_surface_id, app->get_canvas_types(), _width, _height, _format);
    }

private:
    DisplayChannel& _channel;
    int _surface_id;
    int _width;
    int _height;
    uint32_t _format;
};

class DestroySurfaceEvent: public SyncEvent {
public:
    DestroySurfaceEvent(DisplayChannel& channel, int surface_id)
        : _channel (channel)
        , _surface_id (surface_id)
    {
    }

    virtual void do_response(AbstractProcessLoop& events_loop)
    {
        _channel.destroy_canvas(_surface_id);
    }

private:
    DisplayChannel& _channel;
    int _surface_id;
};

class UnlockScreenEvent: public Event {
public:
    UnlockScreenEvent(RedScreen* screen)
        : _screen (screen->ref())
    {
    }

    virtual void response(AbstractProcessLoop& events_loop)
    {
        (*_screen)->unlock_size();
    }

private:
    AutoRef<RedScreen> _screen;
};




StreamsTrigger::StreamsTrigger(DisplayChannel& channel)
    : _channel (channel)
{
}

void StreamsTrigger::on_event()
{
    _channel.on_streams_trigger();
}

#ifdef USE_OPENGL

GLInterruptRecreate::GLInterruptRecreate(DisplayChannel& channel)
    : _channel (channel)
{
}

void GLInterruptRecreate::trigger()
{
    Lock lock(_lock);
    EventSources::Trigger::trigger();
    _cond.wait(lock);
}

void GLInterruptRecreate::on_event()
{
    Lock lock(_lock);
    _channel.recreate_ogl_context_interrupt();
    _cond.notify_one();
}

#endif

void InterruptUpdate::on_event()
{
    _channel.update_interrupt();
}

InterruptUpdate::InterruptUpdate(DisplayChannel& channel)
    : _channel (channel)
{
}

StreamsTimer::StreamsTimer(DisplayChannel& channel)
    : _channel (channel)
{
}

void StreamsTimer::response(AbstractProcessLoop& events_loop)
{
    _channel.streams_time();
}

#define RESET_TIMEOUT (1000 * 5)

class ResetTimer: public Timer {
public:
    ResetTimer(RedScreen* screen, RedClient& client) : _screen(screen), _client(client) {}
    virtual void response(AbstractProcessLoop& events_loop);
private:
    RedScreen* _screen;
    RedClient& _client;
};

void ResetTimer::response(AbstractProcessLoop& events_loop)
{
    _screen->unref();
    _client.deactivate_interval_timer(this);
}

#define MIGRATION_PRIMARY_SURFACE_TIMEOUT (1000 * 5)

class MigPrimarySurfaceTimer: public Timer {
public:
    virtual void response(AbstractProcessLoop& events_loop)
    {
        DisplayChannel *channel =  static_cast<DisplayChannel*>(events_loop.get_owner());
        if (channel->_mig_wait_primary) {
            channel->destroy_primary_surface();
            channel->_mig_wait_primary = false;
        }
        channel->get_process_loop().deactivate_interval_timer(this);
    }
};

class DisplayHandler: public MessageHandlerImp<DisplayChannel, SPICE_CHANNEL_DISPLAY> {
public:
    DisplayHandler(DisplayChannel& channel)
        : MessageHandlerImp<DisplayChannel, SPICE_CHANNEL_DISPLAY>(channel) {}
};

DisplayChannel::DisplayChannel(RedClient& client, uint32_t id,
                               PixmapCache& pixmap_cache, GlzDecoderWindow& glz_window)
    : RedChannel(client, SPICE_CHANNEL_DISPLAY, id, new DisplayHandler(*this),
                 Platform::PRIORITY_LOW)
    , ScreenLayer (SCREEN_LAYER_DISPLAY, true)
    , _pixmap_cache (pixmap_cache)
    , _glz_window (glz_window)
    , _mark (false)
    , _update_mark (0)
    , _streams_timer (new StreamsTimer(*this))
    , _next_timer_time (0)
    , _cursor_visibal (false)
    , _active_pointer (false)
    , _capture_mouse_mode (false)
    , _inputs_channel (NULL)
    , _active_streams (NULL)
    , _streams_trigger (*this)
#ifdef USE_OPENGL
    , _gl_interrupt_recreate (*this)
#endif
    , _interrupt_update (*this)
    , _mig_wait_primary (false)
{
	LOG_INFO("channelID: %d", id);

    DisplayHandler* handler = static_cast<DisplayHandler*>(get_message_handler());

    handler->set_handler(SPICE_MSG_MIGRATE, &DisplayChannel::handle_migrate);
    handler->set_handler(SPICE_MSG_SET_ACK, &DisplayChannel::handle_set_ack);
    handler->set_handler(SPICE_MSG_PING, &DisplayChannel::handle_ping);
    handler->set_handler(SPICE_MSG_WAIT_FOR_CHANNELS, &DisplayChannel::handle_wait_for_channels);
    handler->set_handler(SPICE_MSG_DISCONNECTING, &DisplayChannel::handle_disconnect);
    handler->set_handler(SPICE_MSG_NOTIFY, &DisplayChannel::handle_notify);

    handler->set_handler(SPICE_MSG_DISPLAY_MODE, &DisplayChannel::handle_mode);
    handler->set_handler(SPICE_MSG_DISPLAY_MARK, &DisplayChannel::handle_mark);
    handler->set_handler(SPICE_MSG_DISPLAY_RESET, &DisplayChannel::handle_reset);

    handler->set_handler(SPICE_MSG_DISPLAY_INVAL_LIST,
                         &DisplayChannel::handle_inval_list);
    handler->set_handler(SPICE_MSG_DISPLAY_INVAL_ALL_PIXMAPS,
                         &DisplayChannel::handle_inval_all_pixmaps);
    handler->set_handler(SPICE_MSG_DISPLAY_INVAL_PALETTE,
                         &DisplayChannel::handle_inval_palette);
    handler->set_handler(SPICE_MSG_DISPLAY_INVAL_ALL_PALETTES,
                         &DisplayChannel::handle_inval_all_palettes);

    handler->set_handler(SPICE_MSG_DISPLAY_STREAM_CREATE, &DisplayChannel::handle_stream_create);
    handler->set_handler(SPICE_MSG_DISPLAY_STREAM_CLIP, &DisplayChannel::handle_stream_clip);
    handler->set_handler(SPICE_MSG_DISPLAY_STREAM_DESTROY, &DisplayChannel::handle_stream_destroy);
    handler->set_handler(SPICE_MSG_DISPLAY_STREAM_DESTROY_ALL,
                         &DisplayChannel::handle_stream_destroy_all);

    handler->set_handler(SPICE_MSG_DISPLAY_SURFACE_CREATE, &DisplayChannel::handle_surface_create);
    handler->set_handler(SPICE_MSG_DISPLAY_SURFACE_DESTROY, &DisplayChannel::handle_surface_destroy);

    get_process_loop().add_trigger(_streams_trigger);
#ifdef USE_OPENGL
    get_process_loop().add_trigger(_gl_interrupt_recreate);
#endif
    get_process_loop().add_trigger(_interrupt_update);

    set_draw_handlers();
}

DisplayChannel::~DisplayChannel()
{
    if (screen()) {
        screen()->set_update_interrupt_trigger(NULL);
    }

    destroy_streams();
    do_destroy_all_surfaces();
}

void DisplayChannel::destroy_streams()
{
    Lock lock(_streams_lock);
    for (unsigned int i = 0; i < _streams.size(); i++) {
        delete _streams[i];
        _streams[i] = NULL;
    }
    _active_streams = NULL;
}

void DisplayChannel::set_draw_handlers()
{
    DisplayHandler* handler = static_cast<DisplayHandler*>(get_message_handler());

    handler->set_handler(SPICE_MSG_DISPLAY_COPY_BITS, &DisplayChannel::handle_copy_bits);

    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_FILL, &DisplayChannel::handle_draw_fill);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_OPAQUE, &DisplayChannel::handle_draw_opaque);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_COPY, &DisplayChannel::handle_draw_copy);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_BLEND, &DisplayChannel::handle_draw_blend);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_BLACKNESS, &DisplayChannel::handle_draw_blackness);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_WHITENESS, &DisplayChannel::handle_draw_whiteness);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_INVERS, &DisplayChannel::handle_draw_invers);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_ROP3, &DisplayChannel::handle_draw_rop3);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_STROKE, &DisplayChannel::handle_draw_stroke);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_TEXT, &DisplayChannel::handle_draw_text);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_TRANSPARENT,
                         &DisplayChannel::handle_draw_transparent);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_ALPHA_BLEND,
                         &DisplayChannel::handle_draw_alpha_blend);
    handler->set_handler(SPICE_MSG_DISPLAY_STREAM_DATA, &DisplayChannel::handle_stream_data);
}

void DisplayChannel::clear_draw_handlers()
{
    DisplayHandler* handler = static_cast<DisplayHandler*>(get_message_handler());

    handler->set_handler(SPICE_MSG_DISPLAY_COPY_BITS, NULL);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_FILL, NULL);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_OPAQUE, NULL);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_COPY, NULL);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_BLEND, NULL);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_BLACKNESS, NULL);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_WHITENESS, NULL);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_INVERS, NULL);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_ROP3, NULL);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_STROKE, NULL);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_TEXT, NULL);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_TRANSPARENT, NULL);
    handler->set_handler(SPICE_MSG_DISPLAY_DRAW_ALPHA_BLEND, NULL);
    handler->set_handler(SPICE_MSG_DISPLAY_STREAM_DATA, NULL);
}

void DisplayChannel::copy_pixels(const QRegion& dest_region,
                                 const PixmapHeader &dest_pixmap)
{
    Canvas *canvas;

    if (!_surfaces_cache.exist(0)) {
        return;
    }

    canvas = _surfaces_cache[0];
    canvas->copy_pixels(dest_region, NULL, &dest_pixmap);
}

#ifdef USE_OPENGL
void DisplayChannel::recreate_ogl_context_interrupt()
{
    Canvas* canvas;

    if (_surfaces_cache.exist(0)) { //fix me to all surfaces
        canvas = _surfaces_cache[0];
        ((GCanvas *)(canvas))->touch_context();
        ((GCanvas *)canvas)->textures_lost();
        delete canvas;
    }

    if (!create_ogl_canvas(0, _x_res, _y_res, _format, 0, _rendertype)) {
        THROW("create_ogl_canvas failed");
    }

    canvas = _surfaces_cache[0];
    ((GCanvas *)(canvas))->touch_context();
}

void DisplayChannel::recreate_ogl_context()
{
    if (_surfaces_cache.exist(0) && _surfaces_cache[0]->get_pixmap_type() ==
        CANVAS_TYPE_GL) {
        if (!screen()->need_recreate_context_gl()) {
            _gl_interrupt_recreate.trigger();
        }
    }
}

#endif

void DisplayChannel::update_cursor()
{
    if (!screen() || !_active_pointer) {
        return;
    }

    if (_capture_mouse_mode) {
        //todo: use special cursor for capture mode
        AutoRef<LocalCursor> default_cursor(Platform::create_default_cursor());
        screen()->set_cursor(*default_cursor);
        return;
    }

    if (!_cursor_visibal || !*_cursor) {
        screen()->hide_cursor();
        return;
    }


    if (!(*_cursor)->get_local()) {
        AutoRef<LocalCursor> local_cursor(Platform::create_local_cursor(*_cursor));
        if (*local_cursor == NULL) {
            THROW("create local cursor failed");
        }
        (*_cursor)->set_local(*local_cursor);
    }
    screen()->set_cursor((*_cursor)->get_local());
}

void DisplayChannel::set_cursor(CursorData* cursor)
{
    ASSERT(cursor);
    _cursor.reset(cursor->ref());
    _cursor_visibal = true;
    update_cursor();
}

void DisplayChannel::hide_cursor()
{
    _cursor_visibal = false;
    update_cursor();
}

void DisplayChannel::attach_inputs(InputsChannel* inputs_channel)
{
    if (_inputs_channel) {
        return;
    }

    _inputs_channel = inputs_channel;
    if (_active_pointer && !_capture_mouse_mode) {
        _inputs_channel->on_mouse_position(_pointer_pos.x, _pointer_pos.y,
                                           _buttons_state, get_id());
    }
}

void DisplayChannel::detach_inputs()
{
    _inputs_channel = NULL;
}

bool DisplayChannel::pointer_test(int x, int y)
{
    return contains_point(x, y);
}

void DisplayChannel::on_pointer_enter(int x, int y, unsigned int buttons_state)
{
    _active_pointer = true;
    update_cursor();
    on_pointer_motion(x, y, buttons_state);
}

void DisplayChannel::on_pointer_motion(int x, int y, unsigned int buttons_state)
{
    _pointer_pos.x = x;
    _pointer_pos.y = y;
    _buttons_state = buttons_state;
    if (!_capture_mouse_mode && _inputs_channel) {
        _inputs_channel->on_mouse_position(x, y, buttons_state, get_id());
    }
}

void DisplayChannel::on_pointer_leave()
{
    _active_pointer = false;
}

void DisplayChannel::on_mouse_button_press(int button, int buttons_state)
{
    _buttons_state = buttons_state;
    if (!_capture_mouse_mode && _inputs_channel) {
        _inputs_channel->on_mouse_down(button, buttons_state);
    }
}

void DisplayChannel::on_mouse_button_release(int button, int buttons_state)
{
    _buttons_state = buttons_state;
    if (_capture_mouse_mode) {
        if (button == SPICE_MOUSE_BUTTON_LEFT) {
            get_client().on_mouse_capture_trigger(*screen());
        }
        return;
    }

    if (_inputs_channel) {
        _inputs_channel->on_mouse_up(button, buttons_state);
    }
}

void DisplayChannel::set_capture_mode(bool on)
{
    if (_capture_mouse_mode == on) {
        return;
    }
    _capture_mouse_mode = on;
    update_cursor();
    if (_inputs_channel && !_capture_mouse_mode && _active_pointer) {
        _inputs_channel->on_mouse_position(_pointer_pos.x, _pointer_pos.y, _buttons_state,
                                           get_id());
    }
}

void DisplayChannel::update_interrupt()
{
#ifdef USE_OPENGL
    Canvas *canvas;
#endif

    if (!_surfaces_cache.exist(0) || !screen()) {
        return;
    }

#ifdef USE_OPENGL
    canvas = _surfaces_cache[0];
    if (canvas->get_pixmap_type() == CANVAS_TYPE_GL) {
        ((GCanvas *)(canvas))->pre_gl_copy();
    }
#endif

    screen()->update();

#ifdef USE_OPENGL
    if (canvas->get_pixmap_type() == CANVAS_TYPE_GL) {
        ((GCanvas *)(canvas))->post_gl_copy();
    }
#endif
}

#ifdef USE_OPENGL

void DisplayChannel::pre_migrate()
{
}

void DisplayChannel::post_migrate()
{
#ifdef USE_OPENGL
    if (_surfaces_cache.exist(0) && _surfaces_cache[0]->get_pixmap_type() == CANVAS_TYPE_GL) {
        _gl_interrupt_recreate.trigger();
    }
#endif
}

#endif

void DisplayChannel::copy_pixels(const QRegion& dest_region,
                                 RedDrawable& dest_dc)
{
    if (!_surfaces_cache.exist(0)) {
        return;
    }

    _surfaces_cache[0]->copy_pixels(dest_region, dest_dc);
}

class ActivateTimerEvent: public Event {
public:
    ActivateTimerEvent(DisplayChannel& channel)
        : _channel (channel)
    {
    }

    virtual void response(AbstractProcessLoop& events_loop)
    {
        _channel.activate_streams_timer();
    }

private:
    DisplayChannel& _channel;
};

class AttachChannelsEvent : public Event {
public:
    AttachChannelsEvent(DisplayChannel& channel) : Event(), _channel (channel) {}

    class AttachChannels: public ForEachChannelFunc {
    public:
        AttachChannels(DisplayChannel& channel)
            : _channel (channel)
        {
        }

        virtual bool operator() (RedChannel& channel)
        {
            if (channel.get_type() == SPICE_CHANNEL_CURSOR && channel.get_id() == _channel.get_id()) {
                static_cast<CursorChannel&>(channel).attach_display(&_channel);
            } else if (channel.get_type() == SPICE_CHANNEL_INPUTS) {
                _channel.attach_inputs(&static_cast<InputsChannel&>(channel));
            }
            return false;
        }

    private:
        DisplayChannel& _channel;
    };

    virtual void response(AbstractProcessLoop& events_loop)
    {
        uint32_t mouse_mode = _channel.get_client().get_mouse_mode();
        _channel._capture_mouse_mode =  (mouse_mode == SPICE_MOUSE_MODE_SERVER);
        AttachChannels for_each_func(_channel);
        _channel.get_client().for_each_channel(for_each_func);
    }

private:
    DisplayChannel& _channel;
};

class DetachChannelsEvent : public Event {
public:
    DetachChannelsEvent(DisplayChannel& channel) : Event(), _channel (channel) {}

    class DetatchChannels: public ForEachChannelFunc {
    public:
        DetatchChannels(DisplayChannel& channel)
            : _channel (channel)
        {
        }

        virtual bool operator() (RedChannel& channel)
        {
            if (channel.get_type() == SPICE_CHANNEL_CURSOR && channel.get_id() == _channel.get_id()) {
                static_cast<CursorChannel&>(channel).detach_display();
                return true;
            }
            return false;
        }

    private:
        DisplayChannel& _channel;
    };

    virtual void response(AbstractProcessLoop& events_loop)
    {
        DetatchChannels for_each_func(_channel);
        _channel.get_client().for_each_channel(for_each_func);
    }

private:
    DisplayChannel& _channel;
};

void DisplayChannel::on_connect()
{
    Message* message = new Message(SPICE_MSGC_DISPLAY_INIT);
    SpiceMsgcDisplayInit init;
    init.pixmap_cache_id = 1;
    init.pixmap_cache_size = get_client().get_pixmap_cache_size();
    init.glz_dictionary_id = 1;
    init.glz_dictionary_window_size = get_client().get_glz_window_size();
    _marshallers->msgc_display_init(message->marshaller(), &init);
    post_message(message);
    AutoRef<AttachChannelsEvent> attach_channels(new AttachChannelsEvent(*this));
    get_client().push_event(*attach_channels);
    spice_display_set_params();
}

void DisplayChannel::spice_display_set_params()
{
    SpiceMsgcDisplaySetParams sDisplayParams;

    Message* message = new Message(SPICE_MSGC_DISPLAY_SET_PARAMS);
    sDisplayParams.enable_jpeg = get_client().get_set_params()._enable_jpeg;
    sDisplayParams.jpeg_quality = get_client().get_set_params()._jpeg_quality;
    sDisplayParams.video_codec_type = get_client().get_set_params()._video_codec_type;
    sDisplayParams.video_mjpeg_quality = get_client().get_set_params()._mjpeg_quality;
    sDisplayParams.video_h264_preset = get_client().get_set_params()._h264_preset;
    sDisplayParams.video_h264_keyint = get_client().get_set_params()._h264_keyint;
    _marshallers->msgc_display_set_params(message->marshaller(), &sDisplayParams);
    LOG_INFO("set_params:jpeg_enable:%d, jpeg_quality:%d, codec_type:%s, mjpeg_quality:%d, h264_preset:%s, h264_keyint:%d",
             sDisplayParams.enable_jpeg,
             sDisplayParams.jpeg_quality,
             video_type_to_str(sDisplayParams.video_codec_type),
             sDisplayParams.video_mjpeg_quality,
             video_h264_preset_to_str(sDisplayParams.video_h264_preset),
             sDisplayParams.video_h264_keyint);
    post_message(message);            
}

void DisplayChannel::on_disconnect()
{
    if (_surfaces_cache.exist(0)) {
        _surfaces_cache[0]->clear();
    }

    clear();

    AutoRef<DetachChannelsEvent> detach_channels(new DetachChannelsEvent(*this));
    get_client().push_event(*detach_channels);
    if (screen()) {
        AutoRef<UnlockScreenEvent> unlock_event(new UnlockScreenEvent(screen()));
        get_client().push_event(*unlock_event);
        detach_from_screen(get_client().get_application());
    }
    AutoRef<SyncEvent> sync_event(new SyncEvent());
    get_client().push_event(*sync_event);
    (*sync_event)->wait();
}

void DisplayChannel::do_destroy_all_surfaces()
{
   SurfacesCache::iterator s_iter;

    for (s_iter = _surfaces_cache.begin(); s_iter != _surfaces_cache.end(); s_iter++) {
       delete (*s_iter).second;
    }
    _surfaces_cache.clear();
}

void DisplayChannel::do_destroy_off_screen_surfaces()
{
    SurfacesCache::iterator s_iter;
    Canvas *primary_canvas = NULL;

    for (s_iter = _surfaces_cache.begin(); s_iter != _surfaces_cache.end(); s_iter++) {
        if (s_iter->first == 0) {
            primary_canvas = s_iter->second;
        } else {
            delete s_iter->second;
        }
    }
    _surfaces_cache.clear();
    if (primary_canvas) {
        _surfaces_cache[0] = primary_canvas;
    }
}

void DisplayChannel::destroy_all_surfaces()
{
    AutoRef<DestroyAllSurfacesEvent> destroy_event(new DestroyAllSurfacesEvent(*this));

    get_client().push_event(*destroy_event);
    (*destroy_event)->wait();
    if (!(*destroy_event)->success()) {
        THROW("destroy all surfaces failed");
    }
}

void DisplayChannel::destroy_off_screen_surfaces()
{
    AutoRef<DestroyAllSurfacesEvent> destroy_event(new DestroyAllSurfacesEvent(*this, false));

    get_client().push_event(*destroy_event);
    (*destroy_event)->wait();
    if (!(*destroy_event)->success()) {
        THROW("destroy all surfaces failed");
    }
}

void DisplayChannel::clear(bool destroy_primary)
{
    _palette_cache.clear();
    destroy_streams();
    if (screen()) {
        screen()->set_update_interrupt_trigger(NULL);
    }
    _update_mark = 0;
    _next_timer_time = 0;
    get_client().deactivate_interval_timer(*_streams_timer);
    if (destroy_primary) {
        destroy_all_surfaces();
    } else {
        destroy_off_screen_surfaces();
    }
}

void DisplayChannel::on_disconnect_mig_src()
{
    clear(false);
    // Not clrearing the primary surface till we receive a new one (or a timeout).
    if (_surfaces_cache.exist(0)) {
        AutoRef<MigPrimarySurfaceTimer> mig_timer(new MigPrimarySurfaceTimer());
        get_process_loop().activate_interval_timer(*mig_timer, MIGRATION_PRIMARY_SURFACE_TIMEOUT);
        _mig_wait_primary = true;
    }
}

bool DisplayChannel::create_sw_canvas(int surface_id, int width, int height, uint32_t format)
{
    try {
        SCanvas *canvas = new SCanvas(surface_id == 0, width, height, format,
                                      screen()->get_window(),
                                      _pixmap_cache, _palette_cache, _glz_window,
                                      _surfaces_cache);
        _surfaces_cache[surface_id] = canvas;
        if (surface_id == 0) {
            LOG_INFO("display %d: using sw", get_id());
        }
    } catch (...) {
        return false;
    }
    return true;
}

#ifdef USE_OPENGL
bool DisplayChannel::create_ogl_canvas(int surface_id, int width, int height, uint32_t format,
                                       bool recreate, RenderType rendertype)
{
    try {
        RedWindow *win;

        win = screen()->get_window();
        GCanvas *canvas = new GCanvas(width, height, format, win, rendertype,
                                      _pixmap_cache,
                                      _palette_cache,
                                      _glz_window,
                                      _surfaces_cache);

        screen()->untouch_context();

        _surfaces_cache[surface_id] = canvas;
        _rendertype = rendertype;
        if (surface_id == 0) {
            LOG_INFO("display %d: using ogl", get_id());
        }
    } catch (...) {
        return false;
    }
    return true;
}

#endif

#ifdef WIN32
bool DisplayChannel::create_gdi_canvas(int surface_id, int width, int height, uint32_t format)
{
    try {
        GDICanvas *canvas = new GDICanvas(width, height, format,
                                          _pixmap_cache, _palette_cache, _glz_window,
                                          _surfaces_cache);
        _surfaces_cache[surface_id] = canvas;
        if (surface_id == 0) {
            LOG_INFO("display %d: using gdi", get_id());
        }
    } catch (...) {
        return false;
    }
    return true;
}

#endif

void DisplayChannel::destroy_canvas(int surface_id)
{
    Canvas *canvas;

    if (!_surfaces_cache.exist(surface_id)) {
        LOG_INFO("surface does not exist: %d", surface_id);
        return;
    }

    canvas = _surfaces_cache[surface_id];
    _surfaces_cache.erase(surface_id);

#ifdef USE_OPENGL
    if (canvas->get_pixmap_type() == CANVAS_TYPE_GL) {
        ((GCanvas *)(canvas))->touch_context();
    }
#endif

    delete canvas;
}

void DisplayChannel::create_canvas(int surface_id, const std::vector<int>& canvas_types, int width,
                                   int height, uint32_t format)
{
#ifdef USE_OPENGL
    bool recreate = true;
#endif
    unsigned int i;

    if (screen()) {
#ifdef USE_OPENGL
        if (screen()->need_recreate_context_gl()) {
            recreate = false;
        }
#endif
        screen()->set_update_interrupt_trigger(NULL);
    }

    if (_surfaces_cache.exist(surface_id)) {
        LOG_WARN("surface already exists: %d", surface_id);
    }

    for (i = 0; i < canvas_types.size(); i++) {

        if (canvas_types[i] == CANVAS_OPTION_SW && create_sw_canvas(surface_id, width, height, format)) {
            break;
        }
#ifdef USE_OPENGL
        if (canvas_types[i] == CANVAS_OPTION_OGL_FBO && create_ogl_canvas(surface_id, width, height, format,
                                                                          recreate,
                                                                          RENDER_TYPE_FBO)) {
            break;
        }
        if (canvas_types[i] == CANVAS_OPTION_OGL_PBUFF && create_ogl_canvas(surface_id, width, height, format,
                                                                            recreate,
                                                                            RENDER_TYPE_PBUFF)) {
            break;
        }
#endif
#ifdef WIN32
        if (canvas_types[i] == CANVAS_OPTION_GDI && create_gdi_canvas(surface_id, width, height, format)) {
            break;
        }
#endif
    }

    if (i == canvas_types.size()) {
        THROW("create canvas failed");
    }
}

void DisplayChannel::handle_mode(RedPeer::InMessage* message)
{
    SpiceMsgDisplayMode *mode = (SpiceMsgDisplayMode *)message->data();

    if (_surfaces_cache.exist(0)) {
        destroy_primary_surface();
    }
    create_primary_surface(mode->x_res, mode->y_res,
                           mode->bits == 32 ? SPICE_SURFACE_FMT_32_xRGB : SPICE_SURFACE_FMT_16_555);
}

void DisplayChannel::handle_mark(RedPeer::InMessage *message)
{
    _mark = true;
    SpiceRect area;
    area.top = area.left = 0;
    area.right = _x_res;
    area.bottom = _y_res;

    AutoRef<VisibilityEvent> event(new VisibilityEvent(get_id()));
    get_client().push_event(*event);
    set_rect_area(area);
}

void DisplayChannel::reset_screen()
{
    AutoRef<UnlockScreenEvent> unlock_event(new UnlockScreenEvent(screen()));
    get_client().push_event(*unlock_event);

    screen()->set_update_interrupt_trigger(NULL);
    AutoRef<ResetTimer> reset_timer(new ResetTimer(screen()->ref(), get_client()));

    detach_from_screen(get_client().get_application());
    
    get_client().activate_interval_timer(*reset_timer, RESET_TIMEOUT);
}

void DisplayChannel::handle_reset(RedPeer::InMessage *message)
{
    if (_surfaces_cache.exist(0)) {
        _surfaces_cache[0]->clear();
    }

    _palette_cache.clear();

    reset_screen();
}

void DisplayChannel::handle_inval_list(RedPeer::InMessage* message)
{
    SpiceResourceList *inval_list = (SpiceResourceList *)message->data();

    if (message->size() <
                        sizeof(*inval_list) + inval_list->count * sizeof(inval_list->resources[0])) {
        THROW("access violation");
    }

    for (int i = 0; i < inval_list->count; i++) {
        if (inval_list->resources[i].type != SPICE_RES_TYPE_PIXMAP) {
            THROW("invalid res type");
        }

        _pixmap_cache.remove(inval_list->resources[i].id);
    }
}

void DisplayChannel::handle_inval_all_pixmaps(RedPeer::InMessage* message)
{
    SpiceMsgWaitForChannels *wait = (SpiceMsgWaitForChannels *)message->data();
    if (message->size() < sizeof(*wait) + wait->wait_count * sizeof(wait->wait_list[0])) {
        THROW("access violation");
    }
    get_client().wait_for_channels(wait->wait_count, wait->wait_list);
    _pixmap_cache.clear();
}

void DisplayChannel::handle_inval_palette(RedPeer::InMessage* message)
{
    SpiceMsgDisplayInvalOne* inval = (SpiceMsgDisplayInvalOne*)message->data();
    _palette_cache.remove(inval->id);
}

void DisplayChannel::handle_inval_all_palettes(RedPeer::InMessage* message)
{
    _palette_cache.clear();
}

void DisplayChannel::set_clip_rects(const SpiceClip& clip, uint32_t& num_clip_rects,
                                    SpiceRect*& clip_rects)
{
    switch (clip.type) {
    case SPICE_CLIP_TYPE_RECTS: {
        num_clip_rects = clip.rects->num_rects;
        clip_rects = clip.rects->rects;
        break;
    }
    case SPICE_CLIP_TYPE_NONE:
        num_clip_rects = 0;
        clip_rects = NULL;
        break;
    default:
        THROW("unexpected clip type");
    }
}

void DisplayChannel::handle_stream_create(RedPeer::InMessage* message)
{
    SpiceMsgDisplayStreamCreate* stream_create = (SpiceMsgDisplayStreamCreate*)message->data();
    int surface_id = stream_create->surface_id;

    Lock lock(_streams_lock);
    if (_streams.size() <= stream_create->id) {
        _streams.resize(stream_create->id + 1);
    }

    if (_streams[stream_create->id]) {
        THROW("stream exist");
    }

    if (!_surfaces_cache.exist(surface_id)) {
        THROW("surface does not exist: %d", surface_id);
    }

    uint32_t num_clip_rects;
    SpiceRect* clip_rects;
    set_clip_rects(stream_create->clip, num_clip_rects, clip_rects);
#ifdef USE_GSTREAMER
    _streams[stream_create->id] = new VideoGstreamer(get_client(), *this,
                                                     stream_create->codec_type,
                                                     !!(stream_create->flags & SPICE_STREAM_FLAGS_TOP_DOWN),
                                                     stream_create->stream_width,
                                                     stream_create->stream_height,
                                                     stream_create->src_width,
                                                     stream_create->src_height,
                                                     &stream_create->dest,
                                                     stream_create->clip.type,
                                                     num_clip_rects,
                                                     clip_rects,
                                                     screen()->get_window()->get_window()); 
                                                        
#else
    _streams[stream_create->id] = new VideoStream(get_client(), *_surfaces_cache[surface_id],
                                                  *this, stream_create->codec_type,
                                                  !!(stream_create->flags & SPICE_STREAM_FLAGS_TOP_DOWN),
                                                  stream_create->stream_width,
                                                  stream_create->stream_height,
                                                  stream_create->src_width,
                                                  stream_create->src_height,
                                                  &stream_create->dest,
                                                  stream_create->clip.type,
                                                  num_clip_rects,
                                                  clip_rects);
#endif

    _streams[stream_create->id]->next = _active_streams;
    _active_streams = _streams[stream_create->id];

}

void DisplayChannel::handle_stream_data(RedPeer::InMessage* message)
{
    SpiceMsgDisplayStreamData* stream_data = (SpiceMsgDisplayStreamData*)message->data();
#ifdef USE_GSTREAMER
    VideoGstreamer *stream;
#else
    VideoStream* stream;
#endif

    if (stream_data->id >= _streams.size() || !(stream = _streams[stream_data->id])) {
        THROW("invalid stream");
    }

    if (message->size() < sizeof(SpiceMsgDisplayStreamData) + stream_data->data_size) {
        THROW("access violation");
    }

    stream->push_data(stream_data->multi_media_time, stream_data->data_size, stream_data->data);
}

void DisplayChannel::handle_stream_clip(RedPeer::InMessage* message)
{
    SpiceMsgDisplayStreamClip* clip_data = (SpiceMsgDisplayStreamClip*)message->data();
#ifdef USE_GSTREAMER
    VideoGstreamer *stream;
#else
    VideoStream* stream;
#endif
    uint32_t num_clip_rects;
    SpiceRect* clip_rects;

    if (clip_data->id >= _streams.size() || !(stream = _streams[clip_data->id])) {
        THROW("invalid stream");
    }

    if (message->size() < sizeof(SpiceMsgDisplayStreamClip)) {
        THROW("access violation");
    }
    set_clip_rects(clip_data->clip, num_clip_rects, clip_rects);
    Lock lock(_streams_lock);
    stream->set_clip(clip_data->clip.type, num_clip_rects, clip_rects);
}

void DisplayChannel::handle_stream_destroy(RedPeer::InMessage* message)
{
    SpiceMsgDisplayStreamDestroy* stream_destroy = (SpiceMsgDisplayStreamDestroy*)message->data();

    if (stream_destroy->id >= _streams.size() || !_streams[stream_destroy->id]) {
        THROW("invalid stream");
    }
    Lock lock(_streams_lock);
#ifdef USE_GSTREAMER
    VideoGstreamer **active_stream = &_active_streams;
#else
    VideoStream **active_stream = &_active_streams;
#endif
    for (;;) {
        if (!*active_stream) {
            THROW("not in active streams");
        }

        if (*active_stream == _streams[stream_destroy->id]) {
            *active_stream = _streams[stream_destroy->id]->next;
            break;
        }
        active_stream = &(*active_stream)->next;
    }

    delete _streams[stream_destroy->id];
    _streams[stream_destroy->id] = NULL;
}

void DisplayChannel::handle_stream_destroy_all(RedPeer::InMessage* message)
{
    destroy_streams();
}

void DisplayChannel::create_primary_surface(int width, int height, uint32_t format)
{
    bool do_create_primary = true;
#ifdef USE_OPENGL
    Canvas *canvas;
#endif
    _mark = false;

    /*
     * trying to avoid artifacts when the display hasn't changed much
     * between the disconnection from the migration src and the
     * connection to the target.
     */
    if (_mig_wait_primary) {
        ASSERT(_surfaces_cache.exist(0));
        if (_x_res != width || _y_res != height || format != format) {
            LOG_INFO("destroy the primary surface of the mig src session");
            destroy_primary_surface();
        } else {
            LOG_INFO("keep the primary surface of the mig src session");
            _surfaces_cache[0]->clear();
            clear_area();
            do_create_primary = false;
        }
    }

    if (do_create_primary) {
        LOG_INFO("");
        attach_to_screen(get_client().get_application(), get_id());
        clear_area();
        AutoRef<CreatePrimarySurfaceEvent> event(new CreatePrimarySurfaceEvent(*this, width, height,
                                                                           format));
        get_client().push_event(*event);
        (*event)->wait();
        if (!(*event)->success()) {
            THROW("Create primary surface failed");
        }
    }

    _mig_wait_primary = false;

    _x_res = width;
    _y_res = height;
    _format = format;

#ifdef USE_OPENGL
    canvas = _surfaces_cache[0];

    if (canvas->get_pixmap_type() == CANVAS_TYPE_GL) {
        ((GCanvas *)(canvas))->touch_context();
        screen()->set_update_interrupt_trigger(&_interrupt_update);
        screen()->set_type_gl();
    }
#endif
}

void DisplayChannel::create_surface(int surface_id, int width, int height, uint32_t format)
{
    AutoRef<CreateSurfaceEvent> event(new CreateSurfaceEvent(*this, surface_id, width, height,
                                                             format));
    get_client().push_event(*event);
    (*event)->wait();
    if (!(*event)->success()) {
        THROW("Create surface failed");
    }

#ifdef USE_OPENGL
    Canvas *canvas;

    canvas = _surfaces_cache[surface_id];

    if (canvas->get_pixmap_type() == CANVAS_TYPE_GL) {
        ((GCanvas *)(canvas))->touch_context();
    }
#endif
}

void DisplayChannel::destroy_primary_surface()
{
    if (screen()) {
#ifdef USE_OPENGL
        if (_surfaces_cache.exist(0)) {
            if (_surfaces_cache[0]->get_pixmap_type() == CANVAS_TYPE_GL) {
                screen()->unset_type_gl();
                screen()->untouch_context();
            }
        }
#endif

        reset_screen();
    }

    AutoRef<DestroyPrimarySurfaceEvent> event(new DestroyPrimarySurfaceEvent(*this));
    get_client().push_event(*event);
    (*event)->wait();
    if (!(*event)->success()) {
        THROW("Destroying primary surface failed");
    }
}

void DisplayChannel::destroy_surface(int surface_id)
{
    AutoRef<DestroySurfaceEvent> event(new DestroySurfaceEvent(*this, surface_id));
    get_client().push_event(*event);
    (*event)->wait();
    if (!(*event)->success()) {
        THROW("Destroying surface failed");
    }
}

void DisplayChannel::handle_surface_create(RedPeer::InMessage* message)
{
    SpiceMsgSurfaceCreate* surface_create = (SpiceMsgSurfaceCreate*)message->data();
    if (surface_create->flags == SPICE_SURFACE_FLAGS_PRIMARY) {
        create_primary_surface(surface_create->width, surface_create->height,
                               surface_create->format);
    } else {
        create_surface(surface_create->surface_id, surface_create->width, surface_create->height,
                       surface_create->format);
    }
}

void DisplayChannel::handle_surface_destroy(RedPeer::InMessage* message)
{
    SpiceMsgSurfaceDestroy* surface_destroy = (SpiceMsgSurfaceDestroy*)message->data();
    if (surface_destroy->surface_id == 0) { //fixme
        destroy_primary_surface();
    } else {
        destroy_surface(surface_destroy->surface_id);
    }
}

#define PRE_DRAW
#define POST_DRAW

#define DRAW(type) {                                \
    PRE_DRAW;                                       \
    canvas->draw_##type(*type, message->size());    \
    POST_DRAW;                                      \
    if (type->base.surface_id == 0) {               \
        invalidate(type->base.box, false);          \
    }                                               \
}

void DisplayChannel::handle_copy_bits(RedPeer::InMessage* message)
{
    Canvas *canvas;
    SpiceMsgDisplayCopyBits* copy_bits = (SpiceMsgDisplayCopyBits*)message->data();
    PRE_DRAW;
    canvas = _surfaces_cache[copy_bits->base.surface_id];
    canvas->copy_bits(*copy_bits, message->size());
    POST_DRAW;
    if (copy_bits->base.surface_id == 0) {
        invalidate(copy_bits->base.box, false);
    }
}

void DisplayChannel::handle_draw_fill(RedPeer::InMessage* message)
{
    Canvas *canvas;
    SpiceMsgDisplayDrawFill* fill = (SpiceMsgDisplayDrawFill*)message->data();
    canvas = _surfaces_cache[fill->base.surface_id];
    DRAW(fill);
}

void DisplayChannel::handle_draw_opaque(RedPeer::InMessage* message)
{
    Canvas *canvas;
    SpiceMsgDisplayDrawOpaque* opaque = (SpiceMsgDisplayDrawOpaque*)message->data();
    canvas = _surfaces_cache[opaque->base.surface_id];
    DRAW(opaque);
}

void DisplayChannel::handle_draw_copy(RedPeer::InMessage* message)
{
    Canvas *canvas;
    SpiceMsgDisplayDrawCopy* copy = (SpiceMsgDisplayDrawCopy*)message->data();
    canvas = _surfaces_cache[copy->base.surface_id];
    DRAW(copy);
}

void DisplayChannel::handle_draw_blend(RedPeer::InMessage* message)
{
    Canvas *canvas;
    SpiceMsgDisplayDrawBlend* blend = (SpiceMsgDisplayDrawBlend*)message->data();
    canvas = _surfaces_cache[blend->base.surface_id];
    DRAW(blend);
}

void DisplayChannel::handle_draw_blackness(RedPeer::InMessage* message)
{
    Canvas *canvas;
    SpiceMsgDisplayDrawBlackness* blackness = (SpiceMsgDisplayDrawBlackness*)message->data();
    canvas = _surfaces_cache[blackness->base.surface_id];
    DRAW(blackness);
}

void DisplayChannel::handle_draw_whiteness(RedPeer::InMessage* message)
{
    Canvas *canvas;
    SpiceMsgDisplayDrawWhiteness* whiteness = (SpiceMsgDisplayDrawWhiteness*)message->data();
    canvas = _surfaces_cache[whiteness->base.surface_id];
    DRAW(whiteness);
}

void DisplayChannel::handle_draw_invers(RedPeer::InMessage* message)
{
    Canvas *canvas;
    SpiceMsgDisplayDrawInvers* invers = (SpiceMsgDisplayDrawInvers*)message->data();
    canvas = _surfaces_cache[invers->base.surface_id];
    DRAW(invers);
}

void DisplayChannel::handle_draw_rop3(RedPeer::InMessage* message)
{
    Canvas *canvas;
    SpiceMsgDisplayDrawRop3* rop3 = (SpiceMsgDisplayDrawRop3*)message->data();
    canvas = _surfaces_cache[rop3->base.surface_id];
    DRAW(rop3);
}

void DisplayChannel::handle_draw_stroke(RedPeer::InMessage* message)
{
    Canvas *canvas;
    SpiceMsgDisplayDrawStroke* stroke = (SpiceMsgDisplayDrawStroke*)message->data();
    canvas = _surfaces_cache[stroke->base.surface_id];
    DRAW(stroke);
}

void DisplayChannel::handle_draw_text(RedPeer::InMessage* message)
{
    Canvas *canvas;
    SpiceMsgDisplayDrawText* text = (SpiceMsgDisplayDrawText*)message->data();
    canvas = _surfaces_cache[text->base.surface_id];
    DRAW(text);
}

void DisplayChannel::handle_draw_transparent(RedPeer::InMessage* message)
{
    Canvas *canvas;
    SpiceMsgDisplayDrawTransparent* transparent = (SpiceMsgDisplayDrawTransparent*)message->data();
    canvas = _surfaces_cache[transparent->base.surface_id];
    DRAW(transparent);
}

void DisplayChannel::handle_draw_alpha_blend(RedPeer::InMessage* message)
{
    Canvas *canvas;
    SpiceMsgDisplayDrawAlphaBlend* alpha_blend = (SpiceMsgDisplayDrawAlphaBlend*)message->data();
    canvas = _surfaces_cache[alpha_blend->base.surface_id];
    DRAW(alpha_blend);
}

void DisplayChannel::streams_time()
{
#ifndef USE_GSTREAMER
    _next_timer_time = 0;
    Lock lock(_streams_lock);
    uint32_t mm_time = get_client().get_mm_time();
    uint32_t next_time = 0;
    VideoStream* stream = _active_streams;
    while (stream) {
        uint32_t next_frame_time;
        if ((next_frame_time = stream->handle_timer_update(mm_time))) {
            if (!next_time || int(next_frame_time - next_time) < 0) {
                next_time = next_frame_time;
            }
        }
        stream = stream->next;
    }
    Lock timer_lock(_timer_lock);
    mm_time = get_client().get_mm_time();
    next_time = mm_time + 15;
    if (next_time && (!_next_timer_time || int(next_time - _next_timer_time) < 0)) {
        get_client().activate_interval_timer(*_streams_timer, MAX(int(next_time - mm_time), 0));
        _next_timer_time = next_time;
    } else if (!_next_timer_time) {
        get_client().deactivate_interval_timer(*_streams_timer);
    }
    timer_lock.unlock();
    lock.unlock();
    Platform::yield();
#endif
}

void DisplayChannel::activate_streams_timer()
{
    uint32_t next_time = _next_timer_time;
    if (!next_time) {
        return;
    }

    int delta = next_time - get_client().get_mm_time();
    if (delta <= 0) {
        streams_time();
    } else {
        Lock timer_lock(_timer_lock);
        if (!_next_timer_time) {
            return;
        }
        delta = _next_timer_time - get_client().get_mm_time();
        get_client().activate_interval_timer(*_streams_timer, delta);
    }
}

void DisplayChannel::stream_update_request(uint32_t mm_time)
{
    Lock lock(_timer_lock);
    if (_next_timer_time && int(mm_time - _next_timer_time) > 0) {
        return;
    }
    _next_timer_time = mm_time;
    lock.unlock();
    AutoRef<ActivateTimerEvent> event(new ActivateTimerEvent(*this));
    get_client().push_event(*event);
}

void DisplayChannel::on_update_completion(uint64_t mark)
{
#ifndef RED64
    Lock lock(_mark_lock);
#endif
    _update_mark = mark;
#ifndef RED64
    lock.unlock();
#endif
    _streams_trigger.trigger();
}

void DisplayChannel::on_streams_trigger()
{
#ifndef USE_GSTREAMER

#ifndef RED64
    Lock lock(_mark_lock);
#endif
    uint64_t update_mark = _update_mark;
#ifndef RED64
    lock.unlock();
#endif

    VideoStream* stream = _active_streams;
    while (stream) {
        stream->handle_update_mark(update_mark);
        stream = stream->next;
    }
#endif
}

class DisplayFactory: public ChannelFactory {
public:
    DisplayFactory() : ChannelFactory(SPICE_CHANNEL_DISPLAY) {}
    virtual RedChannel* construct(RedClient& client, uint32_t id)
    {
        return new DisplayChannel(client, id,
                                  client.get_pixmap_cache(), client.get_glz_window());
    }
};

static DisplayFactory factory;

ChannelFactory& DisplayChannel::Factory()
{
    return factory;
}

