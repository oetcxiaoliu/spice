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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "common.h"
#include "red_client.h"
#include "usbredir_channel.h"
extern "C" {
	#include "usbredirhost.h"
}
#include <libusb.h>
#include "utils.h"
#include "usb_device_manager.h"

enum {
	DEVICE_ERROR,
};

struct DEVICE_ERROR {
	libusb_device *device;
	char *error;
};

enum SpiceUsbredirChannelState{
	STATE_DISCONNECTED,
	STATE_WAITING_FOR_ACL_HELPER,
	STATE_CONNECTED,
	STATE_DISCONNECTING,
};

struct _SpiceUsbredirChannelPrivate{
	libusb_device *device;
	struct usbredirhost *host;
	const char *catch_error;
	const uint8_t *read_buf;
	int read_buf_size;
	enum SpiceUsbredirChannelState state;
};

class UsbredirHandler: public MessageHandlerImp<UsbredirChannel, SPICE_CHANNEL_USBREDIR> 
{
public:
	UsbredirHandler(UsbredirChannel& channel) : MessageHandlerImp<UsbredirChannel, SPICE_CHANNEL_USBREDIR>(channel) 
	{
	}
};

UsbredirChannel::UsbredirChannel(RedClient& client, uint32_t id) : RedChannel(client, SPICE_CHANNEL_USBREDIR, id, new UsbredirHandler(*this), Platform::PRIORITY_HIGH)
{
	LOG_INFO("channelID: %d", id);

	UsbredirHandler* handler = static_cast<UsbredirHandler*>(get_message_handler());

	priv = new SpiceUsbredirChannelPrivate;
	priv->device = NULL;
	priv->host = NULL;
	priv->state = STATE_DISCONNECTED;

	handler->set_handler(SPICE_MSG_MIGRATE, &UsbredirChannel::handle_migrate);
	handler->set_handler(SPICE_MSG_SET_ACK, &UsbredirChannel::handle_set_ack);
	handler->set_handler(SPICE_MSG_PING, &UsbredirChannel::handle_ping);
	handler->set_handler(SPICE_MSG_WAIT_FOR_CHANNELS, &UsbredirChannel::handle_wait_for_channels);
	handler->set_handler(SPICE_MSG_DISCONNECTING, &UsbredirChannel::handle_disconnect);
	handler->set_handler(SPICE_MSG_NOTIFY, &UsbredirChannel::handle_notify);

	handler->set_handler(SPICE_MSG_SPICEVMC_DATA, &UsbredirChannel::handle_usbredir_msg);
}

UsbredirChannel::~UsbredirChannel(void)
{
	spice_usbredir_channel_finalize();
	if (priv)
	{
		delete priv;
		priv = NULL;
	}
}

void UsbredirChannel::handle_usbredir_msg(RedPeer::InMessage* message)
{
	SpiceUsbredirChannelPrivate *priv = this->priv;
	int r;
	
	if (priv->host == NULL) return;
	if (priv->read_buf != NULL) return;
	if (priv->state == STATE_DISCONNECTING) return;
	
	priv->read_buf = message->data();
	priv->read_buf_size = message->size();
	
	r = usbredirhost_read_guest_data(priv->host);
	if (r != 0)
	{
		libusb_device *device = priv->device;
		const char *err;
		
		if (device == NULL) return;
		switch (r)
		{
		case usbredirhost_read_parse_error:
			err = "usbredir protocol parse error";
			break;
		case usbredirhost_read_device_rejected:
			err = "rejected by host";
			break;
		case usbredirhost_read_device_lost:
			err = "disconnected (fatal IO error)";
			break;
		default:
			err = "Unknown error";
		}
		
		printf("function: %s, usbredirhost_read_guest_data Error: %s \n", __FUNCTION__, err);
	}
}

void UsbredirChannel::spice_usbredir_channel_set_context()
{
	SpiceUsbredirChannelPrivate *priv = this->priv;
	memset(priv, 0, sizeof(SpiceUsbredirChannelPrivate));
	priv->state = STATE_DISCONNECTED;
	priv->host = usbredirhost_open_full(NULL,
										NULL,
										usbredir_log,
										usbredir_read_callback,
										usbredir_write_callback,
										usbredir_write_flush_callback,
										usbredir_alloc_lock,
										usbredir_lock_lock,
										usbredir_unlock_lock,
										usbredir_free_lock,
										this,
										PACKAGE_STR,
										usbredirparser_warning,
										usbredirhost_fl_write_cb_owns_buffer);
	if (!priv->host)
	{
		printf("funtion: %s, usbredirhost_open_full Error: Out of memory allocating usbredirhost \n", __FUNCTION__);
	}
	//spice_usbredir_channel_up();
}

bool UsbredirChannel::spice_usbredir_channel_open_device()
{
	SpiceUsbredirChannelPrivate *priv = this->priv;
	libusb_device_handle *handle = NULL;
	int rc, status;
	
	if (!priv->state == STATE_DISCONNECTED) return false;
	rc = libusb_open(priv->device, &handle);
	if (rc != 0)
	{
		printf("function: %s, libusb_open Error: %s [%i]", __FUNCTION__, UsbDeviceManager::getInstance()->spice_usb_device_manager_libusb_strerror((enum libusb_error)rc).c_str(), rc);
		priv->catch_error = "Could not open usb device.";
		return false;
	}

	priv->catch_error = NULL;
	status = usbredirhost_set_device(priv->host, handle);

	if (status != usb_redir_success) return false;
	if (!UsbDeviceManager::getInstance()->spice_usb_device_manager_start_event_listening())
	{
		usbredirhost_set_device(priv->host, NULL);
		return false;
	}
	return true;
}

void UsbredirChannel::spice_usbredir_channel_disconnect_device()
{
	SpiceUsbredirChannelPrivate *priv = this->priv;
	switch (priv->state)
	{
	case STATE_DISCONNECTED:
	case STATE_DISCONNECTING:
		break;
	case STATE_WAITING_FOR_ACL_HELPER:
		priv->state = STATE_DISCONNECTING;
		break;
	case STATE_CONNECTED:
		UsbDeviceManager::getInstance()->spice_usb_device_manager_stop_event_listening();
		priv->state = STATE_DISCONNECTING;
		usbredirhost_set_device(priv->host, NULL);
		libusb_unref_device(priv->device);
		priv->device = NULL;
		priv->state = STATE_DISCONNECTED;
		break;
	}
}

void UsbredirChannel::spice_usbredir_channel_finalize()
{
	if (this->priv->host)
	{
		usbredirhost_close(this->priv->host);
	}
}

bool UsbredirChannel::spice_usbredir_channel_connect_device_async(libusb_device *device)
{
	SpiceUsbredirChannelPrivate *priv = this->priv;
	if (device == NULL) return false;
	
	priv->device = libusb_ref_device(device);
	if (!spice_usbredir_channel_open_device())
	{
		libusb_unref_device(priv->device);
		priv->device = NULL;
		return false;
	}
	priv->state = STATE_CONNECTED;
	return true;
}

void UsbredirChannel::spice_usbredir_channel_get_guest_filter()
{
	SpiceUsbredirChannelPrivate *priv = this->priv;
	if (priv->host) return;
}

void UsbredirChannel::spice_usbredir_channel_up()
{
	SpiceUsbredirChannelPrivate *priv = this->priv;
	usbredirhost_write_guest_data(priv->host);
}

libusb_device * UsbredirChannel::spice_usbredir_channel_get_device()
{
	return priv->device;
}

void UsbredirChannel::usbredir_log(void *user_data, int level, const char *msg)
{
	SpiceUsbredirChannelPrivate *priv = ((UsbredirChannel *)user_data)->priv;
	
	if (priv->catch_error && level == usbredirparser_error)
	{
		printf("function: %s, msg: %s \n", __FUNCTION__, msg);
		return;
	}
	
	switch (level)
	{
	case usbredirparser_error:
		printf("function: %s, msg(usbredirparser_error): %s \n", __FUNCTION__, msg);
		break;
	case usbredirparser_warning:
		printf("function: %s, msg(usbredirparser_warning): %s \n", __FUNCTION__, msg);
		break;
	default:
		printf("function: %s, msg(default): %s \n", __FUNCTION__, msg);
		break;
	}
}

int UsbredirChannel::usbredir_read_callback(void *user_data, uint8_t *data, int count)
{
	SpiceUsbredirChannelPrivate *priv = ((UsbredirChannel *)user_data)->priv;

	if (priv->read_buf_size < count) 
	{
		count = priv->read_buf_size;
	}
	
	memcpy(data, priv->read_buf, count);
	
	priv->read_buf_size -= count;
	if (priv->read_buf_size)
		priv->read_buf += count;
	else
		priv->read_buf = NULL;
	
	return count;
}

int UsbredirChannel::usbredir_write_callback(void *user_data, uint8_t *data, int count)
{
	Message* message = new Message(SPICE_MSGC_SPICEVMC_DATA);
	spice_marshaller_add_ref_full(message->marshaller(), data, count, usbredir_free_write_cb_data, user_data);
	((UsbredirChannel *)user_data)->post_message(message);
	return count;
}

void UsbredirChannel::usbredir_write_flush_callback(void *user_data)
{
	SpiceUsbredirChannelPrivate *priv = ((UsbredirChannel *)user_data)->priv;
	if (priv->host == NULL) return;
	usbredirhost_write_guest_data(priv->host);
}

void *UsbredirChannel::usbredir_alloc_lock(void)
{
	pthread_mutex_t *mutex =  new pthread_mutex_t;
	pthread_mutex_init(mutex, NULL);
	return mutex;
}

void UsbredirChannel::usbredir_lock_lock(void *user_data)
{
	pthread_mutex_lock((pthread_mutex_t *)user_data);
}

void UsbredirChannel::usbredir_unlock_lock(void *user_data)
{
	pthread_mutex_unlock((pthread_mutex_t *)user_data);
}

void UsbredirChannel::usbredir_free_lock(void *user_data)
{
	pthread_mutex_destroy((pthread_mutex_t *)user_data);
	delete (pthread_mutex_t *)user_data;
}

void UsbredirChannel::usbredir_free_write_cb_data(uint8_t *data, void *user_data)
{
	SpiceUsbredirChannelPrivate *priv = ((UsbredirChannel *)user_data)->priv;
	usbredirhost_free_write_buffer(priv->host, data);
}

class UsbredirFactory: public ChannelFactory {
public:
    UsbredirFactory() : ChannelFactory(SPICE_CHANNEL_USBREDIR) {}
    virtual RedChannel* construct(RedClient& client, uint32_t id)
    {
        return new UsbredirChannel(client, id);
    }
};

static UsbredirFactory factory;

ChannelFactory& UsbredirChannel::Factory()
{
    return factory;
}