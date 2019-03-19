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
/* Code by neo. Apr 19, 2013 16:16 */
#ifndef _H_USBREDIR_CHANNEL
#define _H_USBREDIR_CHANNEL

#include "red_channel.h"
#include "debug.h"

#define PACKAGE_STR "UNKNOWN"

typedef struct _SpiceUsbredirChannelPrivate SpiceUsbredirChannelPrivate;
struct libusb_device;

class ChannelFactory;

class UsbredirChannel: public RedChannel {
public:
    UsbredirChannel(RedClient& client, uint32_t id);
    ~UsbredirChannel(void);

    static ChannelFactory& Factory();

public:
	void handle_usbredir_msg(RedPeer::InMessage* message);
	void spice_usbredir_channel_set_context();
	bool spice_usbredir_channel_open_device();
	void spice_usbredir_channel_disconnect_device();
	void spice_usbredir_channel_finalize();
	bool spice_usbredir_channel_connect_device_async(libusb_device *device);
	void spice_usbredir_channel_get_guest_filter();
	void spice_usbredir_channel_up();
	libusb_device * spice_usbredir_channel_get_device();
	SpiceUsbredirChannelPrivate * spice_usbredir_channel_get_priv() {return priv;}

private:
	static void usbredir_log(void *user_data, int level, const char *msg);
	static int usbredir_read_callback(void *user_data, uint8_t *data, int count);
	static int usbredir_write_callback(void *user_data, uint8_t *data, int count);
	static void usbredir_write_flush_callback(void *user_data);
	static void *usbredir_alloc_lock(void);
	static void usbredir_lock_lock(void *user_data);
	static void usbredir_unlock_lock(void *user_data);
	static void usbredir_free_lock(void *user_data);
	static void usbredir_free_write_cb_data(uint8_t *data, void *user_data);

private:
	SpiceUsbredirChannelPrivate *priv;

};

#endif