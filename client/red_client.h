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

#ifndef _H_REDCLIENT
#define _H_REDCLIENT

#include <list>

#include "common.h"
#include "red_peer.h"
#include "red_channel.h"
#include "display_channel.h"
#include "inputs_channel.h"
#include "cursor_channel.h"
#include "audio_channels.h"
/* Code by neo. Apr 17, 2013 11:19 */
#ifdef USE_PORTREDIR
#include "serredir_channel.h"
#include "parredir_channel.h"
#endif
#ifdef USE_USBREDIR
#include "usbredir_channel.h"
#endif
#ifdef USE_DEVREDIR
#include "devredir_printer.h"
#include "devredir_cam.h"
#endif
#include "guestos_autologon.h"
#include "messages.h"
#include <spice/vd_agent.h>
#include "process_loop.h"
#define MAX_FILE_SIZE 1024*16
#define LEFT_BRACE '['
#define RIGHT_BRACE ']'

typedef enum InputState {
    INPUT_ON,
    INPUT_OFF,
    INPUT_UNKOWN
}InputState;

typedef enum ControlState {
    CONTROL_ON,
    CONTROL_OFF,
    CONTROL_UNKOWN
}ControlState;

class Application;

class MigChannel: public RedChannelBase {
public:
    MigChannel(uint32_t type, uint32_t id, const ChannelCaps& common_caps, const ChannelCaps& caps)
        : RedChannelBase(type, id, common_caps, caps)
        , _valid(false) {}
    bool is_valid() { return _valid;}
    void set_valid(bool val) { _valid = val;}

private:
    bool _valid;
};

class Migrate {
public:
    Migrate(RedClient& client);
    ~Migrate();

    void start(const SpiceMsgMainMigrationBegin* migrate);
    bool abort();
    void add_channel(MigChannel* channel);
    void clear_channels();
    void swap_peer(RedChannelBase& other);

private:
    void connect_one(MigChannel& channel, const RedPeer::ConnectionOptions& options,
                     uint32_t connection_id);
    void disconnect_channels();
    void close_channels();
    void delete_channels();
    void run();
    static void* worker_main(void *data);

private:
    RedClient& _client;
    typedef std::list<MigChannel*> MigChannels;
    MigChannels _channels;
    bool _running;
    bool _aborting;
    bool _connected;
    std::string _password;
    std::string _host;
    int _port;
    int _sport;
    RedPeer::HostAuthOptions _auth_options;
    std::string _con_ciphers;
    Thread* _thread;
    Mutex _lock;
    Condition _cond;
    int _pending_con;
    int _protocol;
};

class ChannelFactory {
public:
    ChannelFactory(uint32_t type) : _type (type) {}
    virtual ~ChannelFactory() {}

    uint32_t type() { return _type;}
    virtual RedChannel* construct(RedClient& client, uint32_t id) = 0;

private:
    uint32_t _type;
};

class GlzDecoderWindowDebug: public GlzDecoderDebug {
public:
    virtual void error(const std::string& str)
    {
        throw Exception(str);
    }

    virtual void warn(const std::string& str)
    {
        LOG_WARN("%s", str.c_str());
    }

    virtual void info(const std::string& str)
    {
        LOG_INFO("%s", str.c_str());
    }
};

class RedClient;

class AgentTimer: public Timer {
public:
    virtual void response(AbstractProcessLoop& events_loop);
    AgentTimer(RedClient *client) : _client(client) {};
private:
    RedClient *_client;
};

typedef std::map< int, RedPeer::ConnectionOptions::Type> PeerConnectionOptMap;

class ForEachChannelFunc {
public:
    virtual bool operator() (RedChannel& channel) = 0;
};

class DisplaySetting {
public:
    DisplaySetting() : _disable_wallpaper (false)
                     , _disable_font_smooth (false)
                     , _disable_animation (false)
                     , _set_color_depth (false)
                     {}

    bool is_empty() {return !(_disable_wallpaper || _disable_font_smooth ||
                              _disable_animation || _set_color_depth);}

public:
    bool _disable_wallpaper;
    bool _disable_font_smooth;
    bool _disable_animation;
    bool _set_color_depth;
    uint32_t _color_depth;
};

class DisplaySetParams {
public:

    DisplaySetParams() 
        : _enable_jpeg (1)
        , _jpeg_quality (50)
        , _video_codec_type (1)
        , _mjpeg_quality (50)
        , _h264_preset (1)
        , _h264_keyint (20)
#ifdef OFFLINE_VIDEO_REDIR
        , _video_native_redirect (0)
#endif
    {
        _h264_keyint = read_profile_int("video", "video-h264-keyint", 0, "/etc/spicec.conf");
        char cfg_preset[1024] = {0};
        read_profile_string("video", "video-h264-preset", cfg_preset, 255, "", "/etc/spicec.conf" );
        if (strcmp("ultrafast", cfg_preset)==0)
        {
            _h264_preset = 1;
        }
        else if (strcmp("superfast", cfg_preset)==0)
        {
            _h264_preset = 2;
        }
        else if (strcmp("veryfast", cfg_preset)==0)
        {
            _h264_preset = 3;
        }
        else if (strcmp("faster", cfg_preset)==0)
        {
            _h264_preset = 4;
        }
#ifdef OFFLINE_VIDEO_REDIR
        _video_native_redirect = read_profile_int("video", "video-native-redirect", 0, "/etc/spicec.conf");
#endif
    }

public:
    uint32_t _enable_jpeg;
    uint32_t _jpeg_quality;
    uint32_t _video_codec_type;
    uint32_t _mjpeg_quality;
    uint32_t _h264_preset;
    uint32_t _h264_keyint;
#ifdef OFFLINE_VIDEO_REDIR
    uint32_t _video_native_redirect;
#endif

static int load_ini_file(const char *file, char *buf,int *file_size)
{
    FILE *in = NULL;
    int i=0;
    *file_size =0;

    in = fopen(file,"r");
    if( NULL == in) {
        return 0;
    }

    buf[i]=fgetc(in);
    
    //load initialization file
    while( buf[i]!= (char)EOF) {
        i++;
        buf[i]=fgetc(in);
    }
    
    buf[i]='\0';
    *file_size = i;

    fclose(in);
    return 1;
}

static int newline(char c)
{
    return ('\n' == c ||  '\r' == c )? 1 : 0;
}
static int end_of_string(char c)
{
    return '\0'==c? 1 : 0;
}
static int left_barce(char c)
{
    return LEFT_BRACE == c? 1 : 0;
}
static int isright_brace(char c )
{
    return RIGHT_BRACE == c? 1 : 0;
}
static int parse_file(const char *section, const char *key, const char *buf,int *sec_s,int *sec_e,
                      int *key_s,int *key_e, int *value_s, int *value_e)
{
    const char *p = buf;
    int i=0;

    *sec_e = *sec_s = *key_e = *key_s = *value_s = *value_e = -1;

    while( !end_of_string(p[i]) ) {
        //find the section
        if( ( 0==i ||  newline(p[i-1]) ) && left_barce(p[i]) )
        {
            int section_start=i+1;

            //find the ']'
            do {
                i++;
            } while( !isright_brace(p[i]) && !end_of_string(p[i]));

            if( 0 == strncmp(p+section_start,section, i-section_start)) {
                int newline_start=0;

                i++;

                //Skip over space char after ']'
                while(isspace(p[i])) {
                    i++;
                }

                //find the section
                *sec_s = section_start;
                *sec_e = i;

                while( ! (newline(p[i-1]) && left_barce(p[i])) 
                && !end_of_string(p[i]) ) {
                    int j=0;
                    //get a new line
                    newline_start = i;

                    while( !newline(p[i]) &&  !end_of_string(p[i]) ) {
                        i++;
                    }
                    
                    //now i  is equal to end of the line
                    j = newline_start;

                    if(';' != p[j]) //skip over comment
                    {
                        while(j < i && p[j]!='=') {
                            j++;
                            if('=' == p[j]) {
                                if(strncmp(key,p+newline_start,j-newline_start)==0)
                                {
                                    //find the key ok
                                    *key_s = newline_start;
                                    *key_e = j-1;

                                    *value_s = j+1;
                                    *value_e = i;

                                    return 1;
                                }
                            }
                        }
                    }

                    i++;
                }
            }
        }
        else
        {
            i++;
        }
    }
    return 0;
}

int read_profile_string( const char *section, const char *key,char *value, 
         int size, const char *default_value, const char *file)
{
    char buf[MAX_FILE_SIZE]={0};
    int file_size;
    int sec_s,sec_e,key_s,key_e, value_s, value_e;



    if(!load_ini_file(file,buf,&file_size))
    {
        if(default_value!=NULL)
        {
            strncpy(value,default_value, size);
        }
        return 0;
    }

    if(!parse_file(section,key,buf,&sec_s,&sec_e,&key_s,&key_e,&value_s,&value_e))
    {
        if(default_value!=NULL)
        {
            strncpy(value,default_value, size);
        }
        return 0; //not find the key
    }
    else
    {
        int cpcount = value_e -value_s;

        if( size-1 < cpcount)
        {
            cpcount =  size-1;
        }
    
        memset(value, 0, size);
        memcpy(value,buf+value_s, cpcount );
        value[cpcount] = '\0';

        return 1;
    }
}

int read_profile_int( const char *section, const char *key,int default_value, 
                const char *file)
{
    char value[32] = {0};
    if(!read_profile_string(section,key,value, sizeof(value),NULL,file))
    {
        return default_value;
    }
    else
    {
        return atoi(value);
    }
}

int write_profile_string(const char *section, const char *key,
                    const char *value, const char *file)
{
    char buf[MAX_FILE_SIZE]={0};
    char w_buf[MAX_FILE_SIZE]={0};
    int sec_s,sec_e,key_s,key_e, value_s, value_e;
    int value_len = (int)strlen(value);
    int file_size;
    FILE *out;


    if(!load_ini_file(file,buf,&file_size))
    {
        sec_s = -1;
    }
    else
    {
        parse_file(section,key,buf,&sec_s,&sec_e,&key_s,&key_e,&value_s,&value_e);
    }

    if( -1 == sec_s)
    {
        if(0==file_size)
        {
            sprintf(w_buf+file_size,"[%s]\n%s=%s\n",section,key,value);
        }
        else
        {
            //not find the section, then add the new section at end of the file
            memcpy(w_buf,buf,file_size);
            sprintf(w_buf+file_size,"\n[%s]\n%s=%s\n",section,key,value);
        }
    }
    else if(-1 == key_s)
    {
        //not find the key, then add the new key=value at end of the section
        memcpy(w_buf,buf,sec_e);
        sprintf(w_buf+sec_e,"%s=%s\n",key,value);
        sprintf(w_buf+sec_e+strlen(key)+strlen(value)+2,buf+sec_e, file_size - sec_e);
    }
    else
    {
        //update value with new value
        memcpy(w_buf,buf,value_s);
        memcpy(w_buf+value_s,value, value_len);
        memcpy(w_buf+value_s+value_len, buf+value_e, file_size - value_e);
    }
    
    out = fopen(file,"w");
    if(NULL == out)
    {
        return 0;
    }
    
    if(-1 == fputs(w_buf,out) )
    {
        fclose(out);
        return 0;
    }

    fclose(out);
    return 1;
}


};

class ClipboardGrabEvent : public Event {
public:
    ClipboardGrabEvent(uint32_t *types, uint32_t type_count)
    {
        _types = new uint32_t [type_count];
        memcpy(_types, types, type_count * sizeof(uint32_t));
        _type_count = type_count;
    }
    ~ClipboardGrabEvent()
    {
        delete[] _types;
    }

    virtual void response(AbstractProcessLoop& events_loop);

private:
    uint32_t *_types;
    uint32_t _type_count;
};

class ClipboardRequestEvent : public Event {
public:
    ClipboardRequestEvent(uint32_t type) : _type (type) {}
    virtual void response(AbstractProcessLoop& events_loop);

private:
    uint32_t _type;
};

class ClipboardNotifyEvent : public Event {
public:
    ClipboardNotifyEvent(uint32_t type, uint8_t *data, uint32_t size)
    {
        _type = type;
        _data = new uint8_t [size];
        memcpy(_data, data, size);
        _size = size;
    }
    ~ClipboardNotifyEvent()
    {
        delete[] _data;
    }

    virtual void response(AbstractProcessLoop& events_loop);

private:
    uint32_t _type;
    uint8_t *_data;
    uint32_t _size;
};

class ClipboardReleaseEvent : public Event {
public:
    ClipboardReleaseEvent() {}
    virtual void response(AbstractProcessLoop& events_loop);
};

class MigrateEndEvent: public Event {
public:
    virtual void response(AbstractProcessLoop& events_loop);
};

/* Code by neo. Jul 23, 2013 14:20 */
class RedClient: public RedChannel,
                 public Platform::ClipboardListener {
public:
    friend class RedChannel;
    friend class Migrate;
    friend class ClipboardGrabEvent;
    friend class ClipboardRequestEvent;
    friend class ClipboardNotifyEvent;
    friend class ClipboardReleaseEvent;
    friend class MigrateEndEvent;
    friend class DelVideoMessage;
    static RedClient* get(Application& application);
    static RedClient* get(){return _singleton;};
    RedClient(Application& application);
    ~RedClient();

    void register_channel_factory(ChannelFactory& factory);

    virtual void connect();
    virtual void disconnect();
    virtual bool abort();

    void connect(bool wait_main_disconnect);

    void push_event(Event* event);
    void activate_interval_timer(Timer* timer, unsigned int millisec);
    void deactivate_interval_timer(Timer* timer);

    void set_target(const std::string& host, int port, int sport, int protocol = 0);
#ifdef USE_DEVREDIR
    void send_msg_to_device(uint32_t msg_source_flag, uint32_t msg_type, int opaque,uint8_t *data, uint32_t data_size);
#endif
    void set_password(const std::string& password) { _password = password;}
    void set_auto_display_res(bool auto_display_res) { _auto_display_res = auto_display_res;}
    void set_display_setting(DisplaySetting& setting) { _display_setting = setting;}
    const std::string& get_password() { return _password;}
    const std::string& get_host() { return _host;}
    int get_port() { return _port;}
    int get_sport() { return _sport;}
    int get_protocol() { return _protocol;}
    void set_protocol(int protocol) { _protocol = protocol;}
    virtual uint32_t get_connection_id() { return _connection_id;}
    uint32_t get_mouse_mode() { return _mouse_mode;}
    Application& get_application() { return _application;}
    bool is_auto_display_res() { return _auto_display_res;}
    RedPeer::ConnectionOptions::Type get_connection_options(uint32_t channel_type);
    RedPeer::HostAuthOptions& get_host_auth_options() { return _host_auth_opt;}
    const std::string& get_connection_ciphers() { return _con_ciphers;}
    void get_sync_info(uint8_t channel_type, uint8_t channel_id, SyncInfo& info);
    void wait_for_channels(int wait_list_size, SpiceWaitForChannel* wait_list);
    PixmapCache& get_pixmap_cache() {return _pixmap_cache;}
    uint64_t get_pixmap_cache_size() { return _pixmap_cache_size;}
    void on_display_mode_change();
    void on_clipboard_grab(uint32_t *types, uint32_t type_count);
    void on_clipboard_request(uint32_t type);
    void on_clipboard_notify(uint32_t type, uint8_t* data, int32_t size);
    void on_clipboard_release();

    void for_each_channel(ForEachChannelFunc& func);
    void on_mouse_capture_trigger(RedScreen& screen);

    GlzDecoderWindow& get_glz_window() {return _glz_window;}
    int get_glz_window_size() { return _glz_window_size;}

    void set_mm_time(uint32_t time);
    uint32_t get_mm_time();
    void send_main_attach_channels(void);

    DisplaySetParams& get_set_params() { return _display_set_params; }
    void set_display_params(DisplaySetParams& setparams) { _display_set_params = setparams; }
    InputState get_input_state();
    ControlState get_control_state();
    void set_operation_timer(int minutes);
    int get_operation_timer();
    void quit();
    void set_agent_mon_config_sent(bool b) { _agent_mon_config_sent = b; }
	void set_agent_guest_config_sent(bool b) { _agent_guest_config_sent = b; }
protected:
    virtual void on_connecting();
    virtual void on_connect();
    virtual void on_disconnect();
    virtual void on_connect_mig_target() {}
    virtual void on_disconnect_mig_src();

private:
    void on_channel_disconnected(RedChannel& channel);
    void on_channel_disconnect_mig_src_completed(RedChannel& channel);
    void send_migrate_end();
    void migrate_channel(RedChannel& channel);
    void send_agent_announce_capabilities(bool request);
    void send_agent_monitors_config();
    void send_agent_display_config();
	void send_agent_guest_config(int flags);
    void calc_pixmap_cach_and_glz_window_size(uint32_t display_channels_hint,
                                              uint32_t pci_mem_hint);
    void set_mouse_mode(uint32_t supported_modes, uint32_t current_mode);

    void handle_migrate_begin(RedPeer::InMessage* message);
    void handle_migrate_cancel(RedPeer::InMessage* message);
    void handle_migrate_end(RedPeer::InMessage* message);
    void handle_init(RedPeer::InMessage* message);
    void handle_channels(RedPeer::InMessage* message);
    void handle_mouse_mode(RedPeer::InMessage* message);
    void handle_mm_time(RedPeer::InMessage* message);
    void handle_agent_connected(RedPeer::InMessage* message);
    void handle_agent_disconnected(RedPeer::InMessage* message);
    void handle_agent_data(RedPeer::InMessage* message);
    void handle_agent_tokens(RedPeer::InMessage* message);
    void handle_migrate_switch_host(RedPeer::InMessage* message);
    void dispatch_agent_message(VDAgentMessage* msg, void* data);

    bool init_guest_display();
    void on_agent_reply(VDAgentReply* reply);
    void on_agent_announce_capabilities(VDAgentAnnounceCapabilities* caps,
                                        uint32_t msg_size);
    void do_send_agent_clipboard();
    void send_agent_clipboard_message(uint32_t message_type, uint32_t size = 0, void* data = NULL);
    void send_agent_clipboard_notify_message(uint32_t type, uint8_t *data, uint32_t size);

    ChannelFactory* find_factory(uint32_t type);
    void create_channel(uint32_t type, uint32_t id);
    void disconnect_channels();
    void delete_channels();
    bool abort_channels();
    void get_current_resolution();

/* Code by neo. Jun 20, 2014 16:36:42 */
#ifdef USE_DEVREDIR
public:
    GuestOSAutoLogon *guestOSAutoLogon;
#endif
public:
    GUESTOS_AUTOLOGN_INFO pGuestOSAutoLogonInfo;


public:
    int _operation_timer;

private:
    Application& _application;
    static RedClient* _singleton;
#ifdef USE_DEVREDIR
    DevredirPrinter *devredirPrinter;
    DelVideoMessage *devredirCam;
#endif

    std::string _host;
    int _port;
    int _sport;
    int _protocol;
    std::string _password;
    uint32_t _connection_id;
    uint32_t _mouse_mode;
    Mutex _notify_lock;
    bool _notify_disconnect;
    bool _auto_display_res;
    DisplaySetting _display_setting;
    uint32_t _agent_reply_wait_type;

    bool _aborting;
    bool _msg_attach_channels_sent;

    bool _agent_connected;
    bool _agent_mon_config_sent;
    bool _agent_disp_config_sent;
    bool _agent_guest_config_sent;
    bool _agent_autologon_sent;
    int  _agent_autologon_sent_count;
    //FIXME: rename to in/out, extract all agent stuff?
    VDAgentMessage* _agent_msg;
    uint8_t* _agent_msg_data;
    uint32_t _agent_msg_pos;
    VDAgentMessage* _agent_out_msg;
    uint32_t _agent_out_msg_size;
    uint32_t _agent_out_msg_pos;
    uint32_t _agent_tokens;
    AutoRef<AgentTimer> _agent_timer;
    uint32_t _agent_caps_size;
    uint32_t *_agent_caps;

    PeerConnectionOptMap _con_opt_map;
    RedPeer::HostAuthOptions _host_auth_opt;
    std::string _con_ciphers;
    Migrate _migrate;
    Mutex _channels_lock;
    typedef std::list<ChannelFactory*> Factorys;
    Factorys _factorys;
    typedef std::list<RedChannel*> Channels;
    Channels _channels;
    Channels _pending_mig_disconnect_channels;
    PixmapCache _pixmap_cache;
    uint64_t _pixmap_cache_size;
    Mutex _sync_lock;
    Condition _sync_condition;
    uint64_t _sync_info[SPICE_END_CHANNEL][256];

    GlzDecoderWindowDebug _glz_debug;
    GlzDecoderWindow _glz_window;
    int _glz_window_size; // in pixels

    Mutex _mm_clock_lock;
    uint64_t _mm_clock_last_update;
    uint32_t _mm_time;

    bool _during_migration;

    VDAgentMonConfig _current_screen_resolution;
    
    DisplaySetParams _display_set_params;
};

#endif
