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
#ifndef _H_USB_DEVICE_MANAGER
#define _H_USB_DEVICE_MAANGER

#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <pthread.h>
#include "libusb.h"

#define USB_DEVICE_EVENT_ADD 		0x00000000
#define USB_DEVICE_EVENT_REMOVE 	0x00000001

typedef struct _SpiceUsbDeviceManagerPrivate SpiceUsbDeviceManagerPrivate;
typedef struct _SpiceUsbDeviceManager SpiceUsbDeviceManager;
typedef struct _SpiceUsbDevice UsbDevice;

struct _SpiceUsbDeviceManagerPrivate{
	std::string usbredir_class_filter;
	std::string usbredir_enable_devices;
	std::string usbredir_disable_devices;
	std::string usbredir_auto;
	libusb_context *context;
	int event_listeners;
	pthread_t listen_thread;
	bool event_thread_run;
	libusb_device **coldplug_list;
	struct usbredirfilter_rule *auto_conn_filter_rules;
	int auto_conn_filter_rules_count;
};

struct _SpiceUsbDeviceManager{
	SpiceUsbDeviceManagerPrivate *priv;
	bool bUsbDeviceRedirChannelEnabled;
	bool bUsbDeviceAutoConnect;
	std::wstring wstrUsbDeviceAutoConnectDeviceClassFilter;
	std::wstring wstrUsbDeviceAutoConnectDeviceFilter;
};

struct _SpiceUsbDevice{
	uint8_t iUsbDeviceClass;
	uint8_t iUsbDeviceSubClass;
	uint16_t iUsbDeviceVenderID;
	uint16_t iUsbDeviceProductID;
	uint8_t iUsbDeviceManufacturer;
	uint8_t iUsbDeviceProduct;
	uint8_t iUsbDeviceSerialNumber;
	uint8_t iUsbDeviceBusNumber;
	uint8_t iUsbDeviceAddress;
	std::wstring wstrUsbDeviceName;
	bool bUsbDeviceEnabled;
	bool bUsbDeviceAutoRedir;
	bool bUsbDeviceConnectState;
	std::wstring wstrDeviceError;
	libusb_device *libusb_dev;
};

class RedChannel;
class UsbredirChannel;

class UsbDeviceManager
{
public:
	class HLock;
	static UsbDeviceManager * getInstance();

public:
	UsbDeviceManager(void);
	UsbDeviceManager(const UsbDeviceManager &);
	UsbDeviceManager & operator = (const UsbDeviceManager &);
	~UsbDeviceManager(void);

public:
	void spice_usb_device_manager_hibernate(){_bUsbDeviceManagerWorkEnable = false;}
	void spice_usb_device_manager_init();
	void spice_usb_device_manager_do_get_devices();
	std::vector<UsbDevice> &spice_usb_device_manager_get_devices();
	int spice_usb_device_manager_get_device_count();
	void spice_usb_device_manager_click_device(wchar_t *wstrDevId);
	void spice_usb_device_manager_destroy();
	void spice_usb_device_manager_device_add();
	void spice_usb_device_manager_new_channel_cb(RedChannel *channel);
	void spice_usb_device_manager_start();

public:
	std::string spice_usb_device_manager_libusb_strerror(enum libusb_error error_code);
	bool spice_usb_device_manager_set_filters(SpiceUsbDeviceManager *manager);
	bool spice_usb_device_manager_can_redirect(){return _bUsbDeviceManagerWorkEnable;}
	bool spice_usb_device_manager_can_redirect_device(UsbDevice &dev);
	void spice_usb_device_manager_register_channels(RedChannel *channel);
	void spice_usb_device_mananger_do_remove_device(uint16_t iProd, uint16_t iVend);
	void spice_usb_device_manager_device_event(uint32_t event, uint16_t iProd, uint16_t iVend);
	bool spice_usb_device_manager_start_event_listening();
	void spice_usb_device_manager_stop_event_listening();

private:
	void spice_usb_device_manager_do_device_connect(UsbDevice &device);
	void spice_usb_device_manager_do_device_disconnect(UsbDevice &device);
	int spice_usb_device_manager_class_filter_and_enabled(std::string &filter, std::string &usb_enable_filter, char *devName, int iOffValue);
	void spice_usb_device_manager_device_data_provision(UsbDevice &device, libusb_device *lib_dev);
	//void spice_usb_device_manager_refresh_device_list(std::vector<UsbDevice> &newList);
	void spice_usb_device_manager_add_device(UsbDevice &device);
	UsbredirChannel* spice_usb_device_manager_get_channel_for_dev(UsbDevice &dev);
	bool spice_usb_device_manager_is_device_connected(UsbDevice &dev);
	static void* spice_usb_device_manager_usb_ev_thread(void *threadArg);
	
public:
	SpiceUsbDeviceManagerPrivate *_priv;
	class Gard
	{
	public:
		Gard();
		~Gard()
		{
			if (UsbDeviceManager::_pMgr != NULL)
			{
				delete UsbDeviceManager::_pMgr;
			}
		}
	};
	
private:
	static UsbDeviceManager * _pMgr;
	static Gard gd;
	static HLock _lock;
	std::vector<UsbDevice> _UsbDeviceList;
	libusb_device **devs;
	std::list<RedChannel*> _usbChannels;
	bool _bUsbDeviceManagerWorkEnable;
	uint16_t _pDevId[256];

};

class UsbDeviceManager::HLock
{
public:
	HLock()
	{
		pthread_mutex_init(&_mutex, NULL);
	}
	int Lock()
	{
		return pthread_mutex_lock(&_mutex);
	}
	int UnLock()
	{
		return pthread_mutex_unlock(&_mutex);
	}
	~HLock()
	{
		pthread_mutex_destroy(&_mutex);
	}

private:
	pthread_mutex_t _mutex;

};

#endif