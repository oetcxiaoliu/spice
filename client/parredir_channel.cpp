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
/* Code by neo. Apr 19, 2013 15:22 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include "common.h"
#include "utils.h"
#include "red_client.h"
#include "parredir_channel.h"

class ParredirHandler: public MessageHandlerImp<ParredirChannel, SPICE_CHANNEL_PARALLEL> 
{
public:
	ParredirHandler(ParredirChannel& channel) : MessageHandlerImp<ParredirChannel, SPICE_CHANNEL_PARALLEL>(channel) 
	{
	}
};

ParredirChannel::ParredirChannel(RedClient& client, uint32_t id) : RedChannel(client, SPICE_CHANNEL_PARALLEL, id, new ParredirHandler(*this), Platform::PRIORITY_HIGH)
{
	ParredirHandler* handler = static_cast<ParredirHandler*>(get_message_handler());

	handler->set_handler(SPICE_MSG_MIGRATE, &ParredirChannel::handle_migrate);
	handler->set_handler(SPICE_MSG_SET_ACK, &ParredirChannel::handle_set_ack);
	handler->set_handler(SPICE_MSG_PING, &ParredirChannel::handle_ping);
	handler->set_handler(SPICE_MSG_WAIT_FOR_CHANNELS, &ParredirChannel::handle_wait_for_channels);
	handler->set_handler(SPICE_MSG_DISCONNECTING, &ParredirChannel::handle_disconnect);
	handler->set_handler(SPICE_MSG_NOTIFY, &ParredirChannel::handle_notify);

	handler->set_handler(SPICE_MSG_SPICEVMC_DATA, &ParredirChannel::handle_parredir_msg);

	portCount = 0;
	portActive = 0;
	sprintf(portName, "/dev/parport%d", portCount);
	
	parredir_init();
}

ParredirChannel::~ParredirChannel(void)
{
	if (portActive)
	{
		ClosePort();
	}
}

void ParredirChannel::handle_parredir_msg(RedPeer::InMessage* message)
{
	char *buffer = (char *)message->data();
	size_t size = message->size();

	if (!portActive) return;
	parredir_async_wirte_data(hParallel, buffer, size);
}

void ParredirChannel::parredir_init()
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
		pthread_t thread_id;
		hThread = pthread_create(&thread_id, NULL, parredir_read_thread, this);
	}
}

int ParredirChannel::parredir_async_read_data(int hFile, char *buffer, size_t nBufSize)
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

ssize_t ParredirChannel::parredir_async_wirte_data(int hFile, char *buffer, ssize_t nBufSize)
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

void *ParredirChannel::parredir_read_thread(void *read_thread_parameter)
{
	char buffer[1024] = {0};
	int size;

	ParredirChannel* parChannel = (ParredirChannel*)read_thread_parameter;
	
	for (;;)
	{
		size = parChannel->parredir_async_read_data(parChannel->hParallel, buffer, sizeof(buffer));
		if (size > 0)
		{
			Message* message = new Message(SPICE_MSGC_SPICEVMC_DATA);
			spice_marshaller_add_ref_full(message->marshaller(), (uint8_t *)buffer, size, NULL, parChannel);
			parChannel->post_message(message);
		} 
		else if (errno != 0)
		{
			break;
		}
	}
	
	pthread_detach(pthread_self());
	return 0;
}

bool ParredirChannel::OpenPort(char *fileName)
{
	hParallel = ::open(portName, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (hParallel != -1) return true;	
	return false;
}

void ParredirChannel::ClosePort()
{
	::close(hThread);
	::close(hParallel);
	return;
}

class ParredirFactory: public ChannelFactory {
public:
    ParredirFactory() : ChannelFactory(SPICE_CHANNEL_PARALLEL) {}
    virtual RedChannel* construct(RedClient& client, uint32_t id)
    {
        return new ParredirChannel(client, id);
    }
};

static ParredirFactory factory;

ChannelFactory& ParredirChannel::Factory()
{
    return factory;
}