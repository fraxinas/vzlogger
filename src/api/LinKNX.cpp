/***********************************************************************/
/** @file LinKNX.cpp
 * Header file for volkszaehler.org API calls
 *
 *  @author Andreas Frisch
 *  @date   2018-12-07
 *  @email  fraxinas@dreambox.guru
 * @copyright Copyright (c) 2011, The volkszaehler.org project
 * @package vzlogger
 * @license http://opensource.org/licenses/gpl-license.php GNU Public License
 *
 * (C) Fraunhofer ITWM
 **/
/*---------------------------------------------------------------------*/

/*
 * This file is part of volkzaehler.org
 *
 * volkzaehler.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * volkzaehler.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with volkszaehler.org. If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#include <VZException.hpp>
#include "Config_Options.hpp"
#include <api/LinKNX.hpp>
#include <sstream>

extern Config_Options options;

vz::api::LinKNX::LinKNX(
	Channel::Ptr ch,
	std::list<Option> pOptions
)
: ApiIF(ch)
, _host("")
, _port(9999)
, _knx_group("")
, _previous_val(0)
, _first_ts(0)
, _first_counter(0)
, _last_counter(0)

{
	OptionList optlist;
	print(log_debug, "===> Create LinKNX-API", channel()->name());

	/* parse required options */
	try {
		_host      = optlist.lookup_string(pOptions, "host");
		_port      = optlist.lookup_int(pOptions, "port");
		_knx_group = optlist.lookup_string(pOptions, "knx_group");
	} catch (vz::OptionNotFoundException &e) {
		throw;
	} catch (vz::VZException &e) {
		throw;
	}

	pthread_mutex_init(&_send_mutex, NULL);
	print(log_debug, "===> Created LinKNX-API %s:%i", channel()->name(), _host.c_str(), _port);
}

vz::api::LinKNX::~LinKNX()
{
	pthread_mutex_destroy(&_send_mutex);
}

void vz::api::LinKNX::send()
{
	int value = 0;
	bool do_send = false;

	Buffer::Ptr buf = channel()->buffer();
	buf->lock();  

	if (_sendstream.tellp() <= 0) {
		print(log_finest, "_sendstream.tellp = %ld", channel()->name(), _sendstream.tellp());
		_sendstream << "<write>‌";
		do_send = true;
	}

	for (Buffer::iterator it = buf->begin(); it != buf->end(); it++) {
		print(log_finest, "Reading buffer: timestamp %lld value %f", channel()->name(), it->time_ms(), it->value());
		value = it->value();
		_sendstream << "<object id=\"" << _knx_group << "\" value=\"" << std::to_string(value) << "\"/>";
		it->mark_delete();
	}

	if (value != _previous_val && do_send)
	{
		int sockfd = 0;
		struct sockaddr_in serv_addr;
		struct hostent *he;
		char recvbuffer[1024] = {0};
		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			print(log_warning, "LinKNX socket creation error", NULL);
			goto out;
		}

		memset(&serv_addr, '0', sizeof(serv_addr));

		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(_port);

		if ((he = gethostbyname(_host.c_str())) == NULL ) {
			print(log_warning, "Can't resolve host %s", channel()->name(), _host.c_str());
			goto out;
		}

		memcpy(&serv_addr.sin_addr, he->h_addr_list[0], he->h_length);

		if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
		{
			print(log_warning, "Connection Failed", channel()->name());
			close(sockfd);
			goto out;
		}

		print(log_debug, "...sleeping 5s...", channel()->name());
		pthread_mutex_t fakeMutex = PTHREAD_MUTEX_INITIALIZER;
		pthread_cond_t fakeCond = PTHREAD_COND_INITIALIZER;
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += 5;
		pthread_mutex_lock(&fakeMutex);
		pthread_cond_timedwait(&fakeCond, &fakeMutex, &ts);
		pthread_mutex_unlock(&fakeMutex);

		pthread_mutex_lock(&_send_mutex);
		_sendstream << "</write>\n\x04";
		const std::string& tmp = _sendstream.str();
		const char* sendbuf = tmp.c_str();
		print(log_debug, "do send: %s", channel()->name(), sendbuf);
		int n = write(sockfd, sendbuf, strlen(sendbuf));
		if (n < 0) {
			print(log_warning, "Error writing to socket", channel()->name()); 
			close(sockfd);
			goto out;
		}
		ssize_t valread = read(sockfd, recvbuffer, 1024);
		if (valread > 0) {
			if (strstr(recvbuffer, "<write status='success'/>") != NULL) {
				print(log_debug, "successfully written. keep value %i", channel()->name(), value);
				_previous_val = value;
			} else {
				print(log_warning, "error writing value. reply: %s", channel()->name(), recvbuffer);
			}
		}
		close(sockfd);
		_sendstream.clear();
		pthread_mutex_unlock(&_send_mutex);
	}
	goto out;

out:
	buf->unlock();
	buf->clean();
}

void vz::api::LinKNX::register_device() {
}
