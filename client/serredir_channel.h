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
/* Code by neo. Apr 18, 2013 10:53 */
#ifndef _H_SERREDIR_CHANNEL
#define _H_SERREDIR_CHANNEL

#include "red_channel.h"
#include "debug.h"

class ChannelFactory;

class SerredirChannel: public RedChannel {
public:
    SerredirChannel(RedClient& client, uint32_t id);
    ~SerredirChannel(void);

    static ChannelFactory& Factory();

private:
	void handle_serredir_msg(RedPeer::InMessage* message);
	void serredir_init();
	int  serredir_async_read_data(int hFile, char *buffer, size_t nBufSize);
	ssize_t serredir_async_wirte_data(int hFile, char *buffer, ssize_t nBufSize);
	static void *serredir_read_thread(void *read_thread_parameter);

private:
	bool OpenPort(char *fileName);
	bool ConfigurePort(int hFile, int iSpeed);
	void ClosePort();

private:
	int hSerial;
	int hThread;
	char portName[16];
	int portCount;
	int portActive;

};

#endif