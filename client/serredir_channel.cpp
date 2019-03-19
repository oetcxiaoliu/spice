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
/* Code by neo. Apr 18, 2013 10:54 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include "common.h"
#include "utils.h"
#include "red_client.h"
#include "serredir_channel.h"

class SerredirHandler: public MessageHandlerImp<SerredirChannel, SPICE_CHANNEL_SERIAL> 
{
public:
	SerredirHandler(SerredirChannel& channel) : MessageHandlerImp<SerredirChannel, SPICE_CHANNEL_SERIAL>(channel) 
	{
	}
};

SerredirChannel::SerredirChannel(RedClient& client, uint32_t id) : RedChannel(client, SPICE_CHANNEL_SERIAL, id, new SerredirHandler(*this), Platform::PRIORITY_HIGH)
{
	SerredirHandler* handler = static_cast<SerredirHandler*>(get_message_handler());

	handler->set_handler(SPICE_MSG_MIGRATE, &SerredirChannel::handle_migrate);
	handler->set_handler(SPICE_MSG_SET_ACK, &SerredirChannel::handle_set_ack);
	handler->set_handler(SPICE_MSG_PING, &SerredirChannel::handle_ping);
	handler->set_handler(SPICE_MSG_WAIT_FOR_CHANNELS, &SerredirChannel::handle_wait_for_channels);
	handler->set_handler(SPICE_MSG_DISCONNECTING, &SerredirChannel::handle_disconnect);
	handler->set_handler(SPICE_MSG_NOTIFY, &SerredirChannel::handle_notify);

	handler->set_handler(SPICE_MSG_SPICEVMC_DATA, &SerredirChannel::handle_serredir_msg);

	portCount = 0;
	portActive = 0;
	sprintf(portName, "/dev/ttyS%d", portCount);
	
	serredir_init();
}

SerredirChannel::~SerredirChannel(void)
{
	if (portActive)
	{
		ClosePort();
	}
}

void SerredirChannel::handle_serredir_msg(RedPeer::InMessage* message)
{
	char *buffer = (char *)message->data();
	size_t size = message->size();

	if (!portActive) return;
	serredir_async_wirte_data(hSerial, buffer, size);
}

void SerredirChannel::serredir_init()
{
	if (!OpenPort(portName))
	{
		portActive = 0;
		return;
	}
	else
		portActive = 1;

	if (portActive)
	{
		if (!ConfigurePort(hSerial, 9600))
		{
			portActive = 0;
			ClosePort();
			return;
		}
		pthread_t thread_id;
		hThread = pthread_create(&thread_id, NULL, serredir_read_thread, this);
	}
}

int SerredirChannel::serredir_async_read_data(int hFile, char *buffer, size_t nBufSize)
{
	fd_set fdset;
	int iLen = 0;
	
	struct timeval tv = {0, 100 * 1000};
	
	FD_ZERO(&fdset);
	FD_SET(hFile, &fdset);

	if (select(hFile + 1, &fdset, NULL, NULL, &tv))
	{
		if (FD_ISSET(hFile, &fdset))
		{
			iLen = ::read(hFile, buffer, nBufSize);
		}
	}

	return iLen;
}

ssize_t SerredirChannel::serredir_async_wirte_data(int hFile, char *buffer, ssize_t nBufSize)
{
	struct timeval tv = {2, 0};
	fd_set fdset;
	int iLen = 0, written = 0;

	do{
		FD_ZERO(&fdset);
		FD_SET(hFile, &fdset);
		tv.tv_sec = 2;

		if (select(hFile + 1, NULL, &fdset, NULL, &tv))
		{
			if (FD_ISSET(hFile, &fdset))
			{
				iLen = ::write(hFile, buffer + written, nBufSize - written);
				if (iLen > 0)
					written += iLen;
			}
		}
	}while(written < nBufSize);

	return written;
}

void *SerredirChannel::serredir_read_thread(void *read_thread_parameter)
{
	char buffer[1024] = {0};
	int size;

	SerredirChannel* serChannel = (SerredirChannel*)read_thread_parameter;
	
	for (;;)
	{
		size = serChannel->serredir_async_read_data(serChannel->hSerial, buffer, sizeof(buffer));
		if (size > 0)
		{
			Message* message = new Message(SPICE_MSGC_SPICEVMC_DATA);
			spice_marshaller_add_ref_full(message->marshaller(), (uint8_t *)buffer, size, NULL, serChannel);
			serChannel->post_message(message);
		} 
		else if (errno != 0)
		{
			break;
		}
	}
	
	pthread_detach(pthread_self());
	return 0;
}

bool SerredirChannel::OpenPort(char *fileName)
{
	hSerial = ::open(portName, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (hSerial != -1) return true;	
	return false;
}

bool SerredirChannel::ConfigurePort(int hFile, int iSpeed)
{
	bool bRet = false;
	int speed_arr[] = {B38400, B19200, B9600, B4800, B2400, B1200, B300, B38400, B19200, B9600, B4800, B2400, B1200, B300};
	int name_arr[] = {38400, 19200, 9600, 4800, 2400, 1200, 300, 38400, 19200, 9600, 4800, 2400, 1200, 300};
	int status;
	struct termios Opt;
	
	if (0 == tcgetattr(hFile, &Opt))
	{
		for (int i = 0; i < sizeof(speed_arr) / sizeof(int); i++)
		{
			if (iSpeed == name_arr[i])
			{
				Opt.c_oflag &= ~(INLCR | IGNCR | ICRNL);
				Opt.c_oflag &= ~(ONLCR | OCRNL);
				Opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
				Opt.c_oflag &= ~OPOST;

				tcflush(hFile, TCIOFLUSH);
				cfsetispeed(&Opt, speed_arr[i]);
				cfsetospeed(&Opt, speed_arr[i]);

				status = tcsetattr(hFile, TCSANOW, &Opt);
				if (status != 0) break;
				tcflush(hFile, TCIOFLUSH);
				bRet = true;
				break;
			}
		}
	}

	return bRet;
}

void SerredirChannel::ClosePort()
{
	::close(hThread);
	::close(hSerial);
	return;
}

class SerredirFactory: public ChannelFactory {
public:
    SerredirFactory() : ChannelFactory(SPICE_CHANNEL_SERIAL) {}
    virtual RedChannel* construct(RedClient& client, uint32_t id)
    {
        return new SerredirChannel(client, id);
    }
};

static SerredirFactory factory;

ChannelFactory& SerredirChannel::Factory()
{
    return factory;
}