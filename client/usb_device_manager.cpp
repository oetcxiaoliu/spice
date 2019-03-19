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

#include "utils.h"
#include "usbredir_channel.h"
#include "devredir_cam.h"
#include "usb_device_manager.h"

enum {
	DEVICE_ADDED,
	DEVICE_REMOVED,
	AUTO_CONNECT_FAILED,
	DEVICE_ERROR,
	LAST_SIGNAL,
};

UsbDeviceManager * UsbDeviceManager::_pMgr = NULL;
UsbDeviceManager::HLock UsbDeviceManager::_lock;

UsbDeviceManager *UsbDeviceManager::getInstance()
{
	if (!_pMgr)
	{
		_lock.Lock();
		if (!_pMgr)
		{
			_pMgr = new UsbDeviceManager();
		}
		_lock.UnLock();
	}
	return _pMgr;
}

UsbDeviceManager::UsbDeviceManager(void)
{
	_bUsbDeviceManagerWorkEnable = true;
	_priv = NULL;
	
	spice_usb_device_manager_init();
}

UsbDeviceManager::~UsbDeviceManager(void)
{
	spice_usb_device_manager_destroy();
}

void UsbDeviceManager::spice_usb_device_manager_start()
{
	if (_bUsbDeviceManagerWorkEnable && (_priv->usbredir_auto == "yes"))
	{
		spice_usb_device_manager_do_get_devices();
	}
}

void UsbDeviceManager::spice_usb_device_manager_register_channels(RedChannel *channel)
{
	ASSERT(channel);
	_usbChannels.push_back(channel);
}

void UsbDeviceManager::spice_usb_device_manager_init()
{
	int rc = libusb_init(NULL);
	if (rc < 0)
	{
		_bUsbDeviceManagerWorkEnable = false;
		std::string desc = spice_usb_device_manager_libusb_strerror((enum libusb_error)rc);
		printf("function: %s, linusb_init Error: %s \n", __FUNCTION__, desc.c_str());
	}

	_priv = new SpiceUsbDeviceManagerPrivate;
	_priv->usbredir_class_filter = "";
	_priv->usbredir_disable_devices = "";
	_priv->usbredir_enable_devices = "";
	_priv->usbredir_auto = "no";
	_priv->auto_conn_filter_rules = NULL;
	_priv->auto_conn_filter_rules_count = 0;
	_priv->coldplug_list = NULL;
	_priv->context = NULL;
	_priv->event_listeners = 0;
	_priv->event_thread_run = false;
	_priv->listen_thread = NULL;

	for (int i = 0; i < 256; i++)
	{
		_pDevId[i] = 0;
	}
}

void UsbDeviceManager::spice_usb_device_mananger_do_remove_device(uint16_t iProd, uint16_t iVend)
{
	std::vector<UsbDevice>::iterator it;
	for (it = _UsbDeviceList.begin(); it != _UsbDeviceList.end(); ++it)
	{
		if (((*it).iUsbDeviceProductID == iProd ) && ((*it).iUsbDeviceVenderID == iVend))
		{
			spice_usb_device_manager_do_device_disconnect(*it);
			_UsbDeviceList.erase(it);
			break;
		}
	}
}

void UsbDeviceManager::spice_usb_device_manager_device_event(uint32_t event, uint16_t iProd, uint16_t iVend)
{
	switch (event)
	{
	case USB_DEVICE_EVENT_ADD:
		spice_usb_device_manager_do_get_devices();
		break;
	case USB_DEVICE_EVENT_REMOVE:
		spice_usb_device_mananger_do_remove_device(iProd, iVend);
		break;
	}
}

int UsbDeviceManager::spice_usb_device_manager_class_filter_and_enabled(std::string &filter, std::string &usb_enable_filter, char *devName, int iOffValue)
{
	int iRet = 0;

	if (!filter.empty())
	{
		if (filter.at(iOffValue) == '1') iRet = 1;
		if (filter.at(iOffValue) == '0')
		{
			if (usb_enable_filter.find(devName) != std::string::npos) iRet = 1;
		}
	}

	return iRet;
}

void UsbDeviceManager::spice_usb_device_manager_device_data_provision(UsbDevice &device, libusb_device *lib_dev)
{
	ASSERT(lib_dev);
	libusb_device_descriptor desc;
	
	libusb_get_device_descriptor(lib_dev, &desc);


	wchar_t devName[256] = {0};
	swprintf(devName, 255, L"%d:%d:%d:%d", 
					desc.idVendor, desc.idProduct, libusb_get_bus_number(lib_dev), libusb_get_device_address(lib_dev));

	std::wstring wstrDevName = devName;
	printf("%s: devName: %ls \n", __FUNCTION__, wstrDevName.c_str());
	
	device.iUsbDeviceClass = desc.bDeviceClass;
	device.iUsbDeviceSubClass = desc.bDeviceSubClass;
	device.iUsbDeviceVenderID = desc.idVendor;
	device.iUsbDeviceProductID = desc.idProduct;
	device.iUsbDeviceManufacturer = desc.iManufacturer;
	device.iUsbDeviceProduct = desc.iProduct;
	device.iUsbDeviceSerialNumber = desc.iSerialNumber;
	device.iUsbDeviceBusNumber = libusb_get_bus_number(lib_dev);
	device.iUsbDeviceAddress = libusb_get_device_address(lib_dev);
	device.wstrUsbDeviceName = wstrDevName;
	device.bUsbDeviceEnabled = false;
	device.bUsbDeviceAutoRedir = (_priv->usbredir_auto == "yes"); 
	device.bUsbDeviceConnectState = false;
	device.libusb_dev = lib_dev;
}

bool UsbDeviceManager::spice_usb_device_manager_can_redirect_device(UsbDevice &dev)
{
	printf("++++++++++++++%s entry+++++++++++++++\n", __FUNCTION__);

	if (!_bUsbDeviceManagerWorkEnable || _usbChannels.empty())
	{
		printf("%s: Disable USB Device Manager and usb channels is empty(%d:%d)! \n", 
			__FUNCTION__, _bUsbDeviceManagerWorkEnable, _usbChannels.size());
		return false;
	}
	
	if (spice_usb_device_manager_is_device_connected(dev))
	{
		printf("%s: The usb device aleay redir. \n", __FUNCTION__);
		return true;
	}

	printf("%s: The usb device not redir. \n", __FUNCTION__);
	
	UsbredirChannel *channel = NULL;
	std::list<RedChannel*>::iterator it;
	for (it = _usbChannels.begin(); it!=_usbChannels.end(); ++it)
	{
		channel = static_cast<UsbredirChannel*> (*it);
		if (!channel->spice_usbredir_channel_get_device())
		{
			printf("%s: Not found device for usbredir channel. \n", __FUNCTION__);
			break;
		}
	}

	printf("++++++++++++++%s leave+++++++++++++++\n", __FUNCTION__);
	
	return (it != _usbChannels.end());
}

void* UsbDeviceManager::spice_usb_device_manager_usb_ev_thread(void *threadArg)
{
	UsbDeviceManager *pDevMgr = static_cast<UsbDeviceManager *> (threadArg);
	int rc;
	
	while (pDevMgr->_priv->event_thread_run)
	{
		rc = libusb_handle_events(NULL);
		if (rc)
		{
			std::string desc = pDevMgr->spice_usb_device_manager_libusb_strerror((enum libusb_error)rc);
            printf("Error handling USB events: %s [%i] \n", desc.c_str(), rc);
		}
	}
	
	return (void*)0;
}

bool UsbDeviceManager::spice_usb_device_manager_start_event_listening()
{
	_priv->event_listeners++;
	if (_priv->event_listeners > 1)
	{
		return true;
	}
	if (_priv->listen_thread)
	{
		pthread_join(_priv->listen_thread, NULL);
		_priv->listen_thread = NULL;
	}

	_priv->event_thread_run = true;
	int rc = pthread_create(&_priv->listen_thread, NULL, UsbDeviceManager::spice_usb_device_manager_usb_ev_thread, this);
	if (rc != 0)
	{
		printf("function: %s, pthread_create Error: %d \n", __FUNCTION__, rc);
	}

	return (_priv->listen_thread);
}

void UsbDeviceManager::spice_usb_device_manager_stop_event_listening()
{
	if (_priv->event_listeners > 0)
	{
		_priv->event_listeners--;
		if (_priv->event_listeners == 0)
		{
			_priv->event_thread_run = false;
		}
	}
}

bool UsbDeviceManager::spice_usb_device_manager_is_device_connected(UsbDevice &dev)
{
	if (spice_usb_device_manager_get_channel_for_dev(dev))
	{
		return true;
	}
	return false;
}

UsbredirChannel * UsbDeviceManager::spice_usb_device_manager_get_channel_for_dev(UsbDevice &dev)
{
	if (dev.libusb_dev == NULL)
	{
		return NULL;
	}
	UsbredirChannel *channel = NULL;
	for (std::list<RedChannel*>::iterator it = _usbChannels.begin(); it != _usbChannels.end(); ++it)
	{
		channel = static_cast<UsbredirChannel*> (*it);
		if (channel->spice_usbredir_channel_get_device() == dev.libusb_dev) 
		{
			return channel;
		}
	}
	return NULL;
}

void UsbDeviceManager::spice_usb_device_manager_do_get_devices()
{
	UsbDevice device;
	libusb_device *dev;
	int i = 0;
	int m = 0;
	ssize_t cnt;
	int iDeviceEnable = 0;

	if (!_bUsbDeviceManagerWorkEnable) return;

	cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0)
	{
		libusb_free_device_list(devs, 1);
		_bUsbDeviceManagerWorkEnable = false;
		THROW("retrive usb device list error. \n");
	}

	std::vector<UsbDevice> newUsbDeviceList;
	char devName[200] = {0};
	
	while ((dev = devs[i++]) != NULL)
	{
		struct libusb_device_descriptor desc;
		struct libusb_config_descriptor *conf;
		int rValue = libusb_get_device_descriptor(dev, &desc);
		if (rValue < 0) return;
		m = libusb_get_config_descriptor(dev, 0, &conf);
		if (m < 0)
		{
			LOG_INFO("failed to get config descirptor \n");
		} 
		else
		{
			memset(devName, 0, sizeof(devName));
			sprintf(devName, "%d:%d", desc.idProduct, desc.idVendor);
			printf("bInterfaceClass: %d, bInterfaceProtocol: %d, idProduct: %d, idVendor: %d \n", conf->interface->altsetting->bInterfaceClass, conf->interface->altsetting->bInterfaceProtocol,	desc.idProduct, desc.idVendor);
			
			if (conf->interface->altsetting->bInterfaceClass == 9) continue;

			if (conf->interface->altsetting->bInterfaceClass == 3)
			{
				if (conf->interface->altsetting->bInterfaceProtocol == 0 ||
					conf->interface->altsetting->bInterfaceProtocol == 1 ||
					conf->interface->altsetting->bInterfaceProtocol == 2)
				{
					if (_priv->usbredir_enable_devices.find(devName) == std::string::npos)
					{
						continue;
					}
				}
			}

			if (_priv->usbredir_disable_devices.find(devName) != std::string::npos)
			{
				continue;
			}

			uint8_t idev_class = conf->interface->altsetting->bInterfaceClass;
			switch(idev_class)
			{
			case 7:
				iDeviceEnable = spice_usb_device_manager_class_filter_and_enabled(_priv->usbredir_class_filter, _priv->usbredir_enable_devices, devName, 0);
				break;
			case 3:
				iDeviceEnable = spice_usb_device_manager_class_filter_and_enabled(_priv->usbredir_class_filter, _priv->usbredir_enable_devices, devName, 2);
				break;
			case 8:
				iDeviceEnable = spice_usb_device_manager_class_filter_and_enabled(_priv->usbredir_class_filter, _priv->usbredir_enable_devices, devName, 4);
				break;
			case 1:
				iDeviceEnable = spice_usb_device_manager_class_filter_and_enabled(_priv->usbredir_class_filter, _priv->usbredir_enable_devices, devName, 6);
				break;
			case 0x0e:

#ifdef  USE_DEVREDIR
				DelVideoMessage::getInstance()->SendMesgAgentAddDevice(desc.idProduct,desc.idVendor);
				//break;
#endif

				iDeviceEnable = spice_usb_device_manager_class_filter_and_enabled(_priv->usbredir_class_filter, _priv->usbredir_enable_devices, devName, 8);
				break;
			case 0x0b:
				iDeviceEnable = spice_usb_device_manager_class_filter_and_enabled(_priv->usbredir_class_filter, _priv->usbredir_enable_devices, devName, 10);
				break;
			default:
				iDeviceEnable = spice_usb_device_manager_class_filter_and_enabled(_priv->usbredir_class_filter, _priv->usbredir_enable_devices, devName, 12);
				break;
			}
			
			if (iDeviceEnable == 0) continue;

			printf("new usb device insert. devName: %s \n", devName);

			spice_usb_device_manager_device_data_provision(device, dev);

			printf("Get new insert usb device of data provision.\n");

			if (device.bUsbDeviceAutoRedir)
			{
				printf("%s device.bUsbDeviceAutoRedir= true.\n", __FUNCTION__);	
			}
			
			if (device.bUsbDeviceAutoRedir && spice_usb_device_manager_can_redirect_device(device))
			{
				printf("New Insert USB device is auto redirction.\n");
				
				spice_usb_device_manager_do_device_connect(device);

				printf("Connect new usb device success.\n");
			}
			spice_usb_device_manager_add_device(device);
			
			printf("Add new usb device to device manager.\n");
		}
	}
}

void UsbDeviceManager::spice_usb_device_manager_add_device(UsbDevice &device)
{
	if (_UsbDeviceList.empty())
	{
		_UsbDeviceList.push_back(device);
		return;
	}

	for (std::vector<UsbDevice>::iterator it = _UsbDeviceList.begin(); it != _UsbDeviceList.end(); ++it)
	{
		if (((*it).iUsbDeviceBusNumber == device.iUsbDeviceBusNumber) &&
			((*it).iUsbDeviceAddress == device.iUsbDeviceAddress) &&
			((*it).iUsbDeviceProductID == device.iUsbDeviceProductID) &&
			((*it).iUsbDeviceVenderID == device.iUsbDeviceVenderID))
		{
			(*it).bUsbDeviceEnabled  = device.bUsbDeviceEnabled;
			(*it).bUsbDeviceAutoRedir = device.bUsbDeviceAutoRedir;
			(*it).bUsbDeviceConnectState = device.bUsbDeviceConnectState;
			return;
		}
	}
	_UsbDeviceList.push_back(device);
}

std::vector<UsbDevice> &UsbDeviceManager::spice_usb_device_manager_get_devices()
{
	return _UsbDeviceList;
}

int UsbDeviceManager::spice_usb_device_manager_get_device_count()
{
	return _UsbDeviceList.size();
}

void UsbDeviceManager::spice_usb_device_manager_do_device_connect(UsbDevice &device)
{
	if (!device.libusb_dev)
	{
		THROW("invalid target device \n");
	}
	UsbredirChannel *usbChannel = NULL;
	if (spice_usb_device_manager_is_device_connected(device)) 
	{
		return;
	}

	for (std::list<RedChannel*>::iterator it = _usbChannels.begin(); it != _usbChannels.end(); ++it) 
	{
		usbChannel = static_cast<UsbredirChannel*>(*it);
		if (usbChannel->spice_usbredir_channel_get_device())
		{
			continue;
		}
		else
		{ 
			device.bUsbDeviceConnectState = usbChannel->spice_usbredir_channel_connect_device_async(device.libusb_dev);
			break;
		}
	}
}

std::vector<std::string> split(const std::string& src, std::string delimit, std::string null_subst = "")
{
	typedef std::basic_string<char>::size_type s_t;
	static const s_t npos = -1;
	
	if (src.empty() || delimit.empty())
	{
		THROW("split:empty string \n");
	}
	
	std::vector<std::string> v;
	s_t deli_len = delimit.size();
	s_t index = npos, last_search_position = 0;
	
	while ((index = src.find(delimit, last_search_position)) != npos)
	{
		if (index == last_search_position)
		{
			v.push_back(null_subst);
		}
		else
		{
			v.push_back(src.substr(last_search_position, index - last_search_position));
		}
		last_search_position = index + deli_len;
	}

	std::string last_one = src.substr(last_search_position);
	v.push_back(last_one.empty() ? null_subst : last_one);
	
	return v;
}

void UsbDeviceManager::spice_usb_device_manager_click_device(wchar_t *wstrDevId)
{
	if (_UsbDeviceList.empty())
	{
		return;
	}

	uint8_t idevBus, idevAddr;
	uint16_t idevPID, idevVID;
	std::vector<std::string> box;
	char buffer[512] = {0};
	wcstombs(buffer, wstrDevId, sizeof(buffer));
	std::string del = ":";
	box = split(buffer, del);
	idevBus = (uint8_t)atoi(box[0].c_str());
	idevAddr = (uint8_t)atoi(box[1].c_str());
	idevPID = (uint16_t)atoi(box[2].c_str());
	idevVID = (uint16_t)atoi(box[3].c_str());

	std::vector<UsbDevice>::iterator it;
	for (it = _UsbDeviceList.begin(); it != _UsbDeviceList.end(); ++it)
	{
		if (((*it).iUsbDeviceBusNumber == idevBus) && ((*it).iUsbDeviceAddress == idevAddr) &&
			((*it).iUsbDeviceProductID == idevPID ) && ((*it).iUsbDeviceVenderID == idevVID))
		{
			(*it).bUsbDeviceConnectState = !((*it).bUsbDeviceConnectState);
			if ((*it).bUsbDeviceConnectState)
			{
				spice_usb_device_manager_do_device_connect(*it);
			}
			else
			{
				spice_usb_device_manager_do_device_disconnect(*it);
			}
			break;
		}
	}
}

void UsbDeviceManager::spice_usb_device_manager_do_device_disconnect(UsbDevice &device)
{
	UsbredirChannel *usbChannel = NULL;
	usbChannel = spice_usb_device_manager_get_channel_for_dev(device);

	if (usbChannel)
	{
		usbChannel->spice_usbredir_channel_disconnect_device();
	}
}

void UsbDeviceManager::spice_usb_device_manager_destroy()
{
	if (_priv)
	{
		delete _priv;
		_priv = NULL;
	}
	libusb_free_device_list(devs, 1);
	libusb_exit(NULL);
}

std::string UsbDeviceManager::spice_usb_device_manager_libusb_strerror(enum libusb_error error_code)
{
	switch (error_code)
	{
	case LIBUSB_SUCCESS:
		return "Success";
	case LIBUSB_ERROR_IO:
		return "Input/output error";
	case LIBUSB_ERROR_INVALID_PARAM:
		return "Invalid parameter";
	case LIBUSB_ERROR_ACCESS:
		return "Access denied (insufficient permissions)";
	case LIBUSB_ERROR_NO_DEVICE:
		return "No such device (it may have been disconnected)";
	case LIBUSB_ERROR_NOT_FOUND:
		return "Entity not found";
	case LIBUSB_ERROR_BUSY:
		return "Resource busy";
	case LIBUSB_ERROR_TIMEOUT:
		return "Operation timed out";
	case LIBUSB_ERROR_OVERFLOW:
		return "Overflow";
	case LIBUSB_ERROR_PIPE:
		return "Pipe error";
	case LIBUSB_ERROR_INTERRUPTED:
		return "System call interrupted (perhaps due to signal)";
	case LIBUSB_ERROR_NO_MEM:
		return "Insufficient memory";
	case LIBUSB_ERROR_NOT_SUPPORTED:
		return "Operation not supported or unimplemented on this platform";
	case LIBUSB_ERROR_OTHER:
		return "Other error";
	}
	return "Unknown error";
}

bool UsbDeviceManager::spice_usb_device_manager_set_filters(SpiceUsbDeviceManager *manager)
{
	return true;
}

void UsbDeviceManager::spice_usb_device_manager_new_channel_cb(RedChannel *channel)
{
	UsbredirChannel *usbredir_channel = dynamic_cast<UsbredirChannel *>(channel);

	usbredir_channel->spice_usbredir_channel_set_context();
	channel->connect();
}