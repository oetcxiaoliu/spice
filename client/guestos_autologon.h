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

#ifndef _H_GUESTOS_AUTOLOGON
#define _H_GUESTOS_AUTOLOGON

#include "red_channel.h"
#include "debug.h"

typedef struct _GUESTOS_AUTOLOGON_INFO{
	int iGuestOSAutoLogon;
	char szGuestOSAutoLogonUsername[50];
	char szGuestOSAutoLogonPassword[50];
	char szGuestOSAutoLogonDomain[50];
}GUESTOS_AUTOLOGN_INFO, *PGUESTOS_AUTOLOGON_INFO, *LPGUESTOS_AUTOLOGON_INFO;

class GuestOSAutoLogon
{
public:
	GuestOSAutoLogon(RedClient* client);
	~GuestOSAutoLogon(void);
	void msg_deal(void *pHead,void *pData);

private:
	int send_autologon_msg();

private:
	RedClient* _client;
	GUESTOS_AUTOLOGN_INFO pGuestOSAutoLogonInfo;

};

#endif