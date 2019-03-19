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
#include "red_client.h"
#include "guestos_autologon.h"
#include "utils.h"

GuestOSAutoLogon::GuestOSAutoLogon(RedClient* client)
{	
	_client = client;
	pGuestOSAutoLogonInfo = _client->pGuestOSAutoLogonInfo;
	send_autologon_msg();
}

GuestOSAutoLogon::~GuestOSAutoLogon(void)
{
}

void GuestOSAutoLogon::msg_deal(void *pHead,void *pData)
{
	VDAgentMessage *msg = (VDAgentMessage *)pHead;

	//uint32_t RecSize = msg->size;
	//char *pchData = (char *)pData;

	switch (msg->type)
	{
	case AGENT_MSG_DEVICE_START:
		break;
	case AGENT_MSG_DEVICE_DATA:
		break;
	case AGENT_MSG_DEVICE_COMPLATE:
		break;
	default:
		break;
	}

	return;
}

int GuestOSAutoLogon::send_autologon_msg()
{
	_client->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_HID, VD_AGENT_DEVICE, 0, (uint8_t*)&pGuestOSAutoLogonInfo, sizeof(pGuestOSAutoLogonInfo));

	return 0;
}