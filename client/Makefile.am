HI_CFLAGS = -I../include
HI_LIBS = -L../lib -lhigo 
 
NULL =

bin_PROGRAMS = spicec

BUILT_SOURCES = 			\
	generated_demarshallers.cpp	\
	generated_marshallers.cpp	\
	generated_demarshallers1.cpp	\
	generated_marshallers1.cpp	\
	$(NULL)

generated_demarshallers.cpp: $(top_srcdir)/spice.proto
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/spice_codegen.py --generate-demarshallers --client --include common.h --include messages.h $(top_srcdir)/spice.proto generated_demarshallers.cpp

generated_demarshallers1.cpp: $(top_srcdir)/spice1.proto
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/spice_codegen.py --generate-demarshallers --client --include common.h --include messages.h --prefix 1 --ptrsize 8 $(top_srcdir)/spice1.proto generated_demarshallers1.cpp

generated_marshallers.cpp: $(top_srcdir)/spice.proto
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/spice_codegen.py --generate-marshallers -P  --include "common.h" --include messages.h --include marshallers.h --client $(top_srcdir)/spice.proto generated_marshallers.cpp

generated_marshallers1.cpp: $(top_srcdir)/spice1.proto
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/spice_codegen.py --generate-marshallers -P  --include "common.h" --include messages.h --include marshallers.h --client --prefix 1 --ptrsize 8 $(top_srcdir)/spice1.proto generated_marshallers1.cpp

MAINTAINERCLEANFILES = $(BUILT_SOURCES)

spicec_SOURCES =		\
	application.cpp		\
	application.h		\
	audio_channels.h	\
	audio_devices.h		\
	cache.hpp		\
	canvas.cpp		\
	canvas.h		\
	client_net_socket.cpp	\
	client_net_socket.h	\
	cmd_line_parser.cpp	\
	cmd_line_parser.h	\
	common.h		\
	controller.cpp		\
	controller.h		\
	cursor.cpp		\
	cursor.h		\
	cursor_channel.cpp	\
	cursor_channel.h	\
	debug.h			\
	demarshallers.h		\
	display_channel.cpp	\
	display_channel.h	\
	event_sources.h		\
	foreign_menu.cpp	\
	foreign_menu.h		\
	glz_decoded_image.h	\
	glz_decoder.cpp		\
	glz_decoder.h		\
	glz_decoder_config.h	\
	glz_decoder_window.cpp	\
	glz_decoder_window.h	\
	hot_keys.cpp		\
	hot_keys.h		\
	icon.h			\
	inputs_channel.cpp	\
	inputs_channel.h	\
	inputs_handler.h	\
	jpeg_decoder.cpp	\
	jpeg_decoder.h		\
	marshallers.h		\
	menu.cpp		\
	menu.h			\
	monitor.cpp		\
	monitor.h		\
	pixels_source.h		\
	platform.h		\
	playback_channel.cpp	\
	process_loop.cpp	\
	process_loop.h		\
	read_write_mutex.h	\
	record_channel.cpp	\
	red_canvas_base.h	\
	red_channel.cpp		\
	red_channel.h		\
	red_client.cpp		\
	red_client.h		\
	red_drawable.h		\
	red_key.h		\
	red_peer.cpp		\
	red_peer.h		\
	red_pixmap.h		\
	red_pixmap_sw.h		\
	red_sw_canvas.cpp	\
	red_sw_canvas.h		\
	red_types.h		\
	red_window.h		\
	screen.cpp		\
	screen.h		\
	screen_layer.cpp	\
	screen_layer.h		\
	shared_cache.hpp	\
	threads.cpp		\
	threads.h		\
	utils.cpp		\
	utils.h			\
	zlib_decoder.cpp	\
	zlib_decoder.h		\
	JSON.cpp				\
	JSON.h					\
	JSONValue.cpp			\
	JSONValue.h				\
	b64_decode.c			\
	b64_encode.c			\
	b64.h					\
	$(BUILT_SOURCES)	\
	$(NULL)

if SUPPORT_OFFLINE_VIDEO_REDIR
spicec_SOURCES +=			\
	video_play.c			\
	function.h				\
	$(NULL)
endif

if OS_WIN32
spicec_SOURCES +=			\
	red_gdi_canvas.cpp		\
	red_gdi_canvas.h		\
	red_pixmap_gdi.h		\
	windows/atomic_count.h		\
	windows/event_sources_p.cpp	\
	windows/main.cpp		\
	windows/my_getopt.cpp		\
	windows/named_pipe.cpp		\
	windows/named_pipe.h		\
	windows/pixels_source.cpp	\
	windows/pixels_source_p.h	\
	windows/platform.cpp		\
	windows/platform_utils.cpp	\
	windows/platform_utils.h	\
	windows/playback.cpp		\
	windows/playback.h		\
	windows/record.cpp		\
	windows/record.h		\
	windows/red_drawable.cpp	\
	windows/red_pixmap.cpp		\
	windows/red_pixmap_gdi.cpp	\
	windows/red_pixmap_sw.cpp	\
	windows/red_window.cpp		\
	windows/red_window_p.h		\
	windows/resource.h		\
	windows/stdint.h		\
	windows/win_platform.h		\
	$(NULL)
else
spicec_SOURCES +=			\
	x11/atomic_count.h		\
	x11/event_sources_p.cpp		\
	x11/event_sources_p.h		\
	x11/main.cpp			\
	x11/named_pipe.cpp		\
	x11/named_pipe.h		\
	x11/pixels_source.cpp		\
	x11/pixels_source_p.h		\
	x11/platform.cpp		\
	x11/platform_utils.cpp		\
	x11/platform_utils.h		\
	x11/playback.cpp		\
	x11/playback.h			\
	x11/record.cpp			\
	x11/record.h			\
	x11/red_drawable.cpp		\
	x11/red_pixmap.cpp		\
	x11/red_pixmap_sw.cpp		\
	x11/red_window.cpp		\
	x11/red_window_p.h		\
	x11/res.cpp			\
	x11/res.h			\
	x11/resource.h			\
	x11/x_icon.cpp			\
	x11/x_icon.h			\
	x11/x_platform.h		\
	$(NULL)
endif

if SUPPORT_TUNNEL
spicec_SOURCES +=		\
	tunnel_channel.cpp	\
	tunnel_channel.h	\
	$(NULL)
endif

if SUPPORT_GUI
spicec_SOURCES +=			\
	gui/gui.cpp			\
	gui/gui.h			\
	gui/resource_provider.cpp	\
	gui/resource_provider.h		\
	gui/softrenderer.cpp		\
	gui/softrenderer.h		\
	gui/softtexture.cpp		\
	gui/softtexture.h		\
	$(NULL)
endif

if SUPPORT_GL
spicec_SOURCES +=		\
	red_gl_canvas.cpp	\
	red_gl_canvas.h		\
	red_pixmap_gl.h		\
	$(NULL)

if !OS_WIN32
spicec_SOURCES += x11/red_pixmap_gl.cpp
endif

endif

if SUPPORT_SMARTCARD
spicec_SOURCES +=		\
	smartcard_channel.cpp	\
	smartcard_channel.h	\
	$(NULL)
endif

if SUPPORT_PORTREDIR
spicec_SOURCES +=		\
	serredir_channel.cpp	\
	serredir_channel.h	\
	parredir_channel.cpp	\
	parredir_channel.h	\
	$(NULL)
endif

if SUPPORT_USBREDIR
spicec_SOURCES +=		\
	usbredir_channel.cpp	\
	usbredir_channel.h	\
	usb_device_manager.cpp	\
	usb_device_manager.h	\
	$(NULL)
endif

if SUPPORT_DEVREDIR
spicec_SOURCES +=		\
	devredir_printer.cpp	\
	devredir_printer.h	\
	devredir_cam.cpp \
	devredir_cam.h \
	linux_video.cpp \
	linux_video.h \
	x264_encode.cpp \
	x264_encode.h \
	guestos_autologon.cpp \
	guestos_autologon.h \
	$(NULL)
endif

if SUPPORT_GSTREAMER
spicec_SOURCES +=		\
	gstreamer.cpp \
	gstreamer.h \
	$(NULL)
else
spicec_SOURCES +=		\
	video_stream.cpp\
	video_stream.h \
	decoder.cpp \
	decoder.h \
	$(NULL)
endif

if SUPPORT_DEVREDIR
DEVREDIR_CFLAGS = -I ${prefix}
DEVREDIR_LIBS = -L${prefix}/ -lcups -lx264
#DEVREDIR_CFLAGS = `cups-config --cflags`
#DEVREDIR_LIBS = `cups-config --libs`

else
DEVREDIR_CFLAGS = 
DEVREDIR_LIBS = 
endif

if SUPPORT_GSTREAMER
STREAM_CFLAGS =  `pkg-config --cflags gstreamer-app-1.0 gstreamer-1.0 gstreamer-video-1.0`
STREAM_LIBS = -lgstapp-1.0 -lgstvideo-1.0 -lgstbase-1.0 -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0
else
STREAM_CFLAGS =  -I /usr/ffmpeg/include
STREAM_LIBS = -L/usr/ffmpeg/lib -lavcodec -lavutil -lswscale
endif

if SUPPORT_CLOUD_CLASS
STREAM_CFLAGS += -DCLOULDCLASS \
    $(NULL)
endif

if SUPPORT_OFFLINE_VIDEO_REDIR
STREAM_CFLAGS +=                    \
	-I/usr/local/include        \
	$(NULL)
STREAM_LIBS +=                      \
	-L/usr/local/lib -lavdevice -lavfilter  \
	-lavformat -lavcodec -ldl -lXv -lX11    \
	-lXext -ljack -lasound -lSDL -lbz2 -lz  \
	-lswresample -lswscale -lavutil -lm     \
	$(NULL)
endif

INCLUDES = \
	-D__STDC_LIMIT_MACROS			\
	-I$(top_srcdir)/client/x11		\
	-I$(top_srcdir)/common			\
	$(ALSA_CFLAGS)					\
	$(CEGUI_CFLAGS)					\
	$(CEGUI06_CFLAGS)				\
	$(CELT051_CFLAGS)				\
	$(GL_CFLAGS)					\
	$(MISC_X_CFLAGS)				\
	$(PIXMAN_CFLAGS)				\
	$(PROTOCOL_CFLAGS)				\
	$(USBREDIR_CFLAGS)				\
	$(DEVREDIR_CFLAGS)				\
	$(SPICE_NONPKGCONFIG_CFLAGS)	\
	$(SMARTCARD_CFLAGS)				\
	$(SSL_CFLAGS)					\
	$(XRANDR_CFLAGS)				\
	$(XFIXES_CFLAGS)				\
	$(WARN_CFLAGS)					\
	$(STREAM_CFLAGS)					\
	$(NULL)

spicec_LDFLAGS = $(SPICEC_STATIC_LINKAGE_BSTATIC)

spicec_LDADD =						\
	$(top_builddir)/common/libspice-common.la	\
	$(ALSA_LIBS)					\
	$(CEGUI_LIBS)					\
	$(CEGUI06_LIBS)					\
	$(CELT051_LIBS)					\
	$(GL_LIBS)						\
	$(JPEG_LIBS)					\
	$(MISC_X_LIBS)					\
	$(PIXMAN_LIBS)					\
	$(SMARTCARD_LIBS)				\
	$(USBREDIR_LIBS)				\
	$(DEVREDIR_LIBS)				\
	$(SPICE_NONPKGCONFIG_LIBS)		\
	$(SSL_LIBS)						\
	$(XFIXES_LIBS)					\
	$(XRANDR_LIBS)					\
	$(STREAM_LIBS) 					\
	$(Z_LIBS)						\
	$(NULL)

EXTRA_DIST =				\
	glz_decode_tmpl.c		\
	x11/images/red_icon.c		\
	x11/images/alt_image.c		\
	gui/commonv2c.ttf.c		\
	gui/commonwealth-10.font.c	\
	gui/dejavu_sans-10.font.c	\
	gui/dejavu_sans.ttf.c		\
	gui/taharez_look.imageset.c	\
	gui/taharez_look.looknfeel.c	\
	gui/taharez_look.scheme.c	\
	gui/taharez_look.tga.c		\
	$(NULL)
