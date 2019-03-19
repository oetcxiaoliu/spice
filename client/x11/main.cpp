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
#include "application.h"

#ifdef USE_GSTREAMER
#include "gst/gst.h"
GST_DEBUG_CATEGORY (spicec_video_debug);
#define GST_CAT_DEFAULT spicec_video_debug
#endif

static void cleanup()
{
    spice_log_cleanup();
}

const char * version_str = VERSION;

int main(int argc, char** argv)
{
    int exit_val;

    atexit(cleanup);
    try {
#ifdef USE_GSTREAMER
        gst_init(NULL, NULL);
        GST_DEBUG_CATEGORY_INIT (spicec_video_debug, "spicec-video", 0,  "spicec video player");
        DBG(0, "Spice client use gstreamer to play video");
#endif
        exit_val = Application::main(argc, argv, version_str);
        LOG_INFO("Spice client terminated (exitcode = %d)", exit_val);
    } catch (Exception& e) {
        LOG_ERROR("unhandled exception: %s", e.what());
        exit_val = e.get_error_code();
    } catch (std::exception& e) {
        LOG_ERROR("unhandled exception: %s", e.what());
        exit_val = SPICEC_ERROR_CODE_ERROR;
    } catch (...) {
        LOG_ERROR("unhandled exception");
        exit_val = SPICEC_ERROR_CODE_ERROR;
    }

    return exit_val;
}

