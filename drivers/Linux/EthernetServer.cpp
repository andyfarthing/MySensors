/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Marcelo Aquino <marceloaqno@gmail.org>
 * Copyright (C) 2016 Marcelo Aquino
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * Based on Arduino ethernet library, Copyright (c) 2010 Arduino LLC. All right reserved.
 */

#include <cstdio>
#include <sys/socket.h>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "EthernetClient.h"
#include "EthernetServer.h"

EthernetServer::EthernetServer(uint16_t port, uint16_t max_clients) : port(port), max_clients(max_clients)
{
	clients.reserve(max_clients);
}

void EthernetServer::begin()
{
	begin(IPAddress(0,0,0,0));
}

void EthernetServer::begin(IPAddress address)
{
	struct addrinfo hints, *servinfo, *p;
	int yes=1;
	int rv;
	char ipstr[INET_ADDRSTRLEN];
	char portstr[6];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	sprintf(portstr, "%d", port);
	if ((rv = getaddrinfo(address.toString().c_str(), portstr, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return;
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			freeaddrinfo(servinfo);
			return;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "failed to bind\n");
		freeaddrinfo(servinfo);
		return;
	}

	if (listen(sockfd, ETHERNETSERVER_BACKLOG) == -1) {
		perror("listen");
		freeaddrinfo(servinfo);
		return;
	}

	freeaddrinfo(servinfo);

	fcntl(sockfd, F_SETFL, O_NONBLOCK);

	struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
	void *addr = &(ipv4->sin_addr);
	inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
	ETHERNETSERVER_DEBUG("Listening for connections on %s:%s\n", ipstr, portstr);
}

bool EthernetServer::hasClient()
{
	_accept();

	return !new_clients.empty();
}

EthernetClient EthernetServer::available()
{
	if (new_clients.empty()) {
		return EthernetClient();
	} else {
		int sock = new_clients.front();
		new_clients.pop_front();
		return EthernetClient(sock);
	}
}

size_t EthernetServer::write(uint8_t b)
{
	return write(&b, 1);
}

size_t EthernetServer::write(const uint8_t *buffer, size_t size)
{
	size_t n = 0;
	size_t i = 0;

	while (i < clients.size()) {
		EthernetClient client(clients[i]);
		if (client.connected()) {
			n += client.write(buffer, size);
			i++;
		} else if (!client.available()) {
			client.stop();
			clients[i] = clients.back();
			clients.pop_back();
			ETHERNETSERVER_DEBUG("Client disconnected.");
		}
	}

	return n;
}

size_t EthernetServer::write(const char *str)
{
	if (str == NULL) return 0;
	return write((const uint8_t *)str, strlen(str));
}

size_t EthernetServer::write(const char *buffer, size_t size)
{
	return write((const uint8_t *)buffer, size);
}

void EthernetServer::_accept()
{
	int new_fd;
	socklen_t sin_size;
	struct sockaddr_storage client_addr;
	char ipstr[INET_ADDRSTRLEN];

	if (new_clients.size() + clients.size() == max_clients) {
		// no free slots, search for a dead client
		bool no_free_slots = true;
		for (size_t i = 0; i < clients.size();) {
			EthernetClient client(clients[i]);
			if (client.connected() || client.available()) {
				i++;
			} else {
				clients[i] = clients.back();
				clients.pop_back();
				no_free_slots = false;
				break;
			}
		}
		if (no_free_slots) {
			for (std::list<int>::iterator it = new_clients.begin(); it != new_clients.end(); ++it) {
				EthernetClient client(*it);
				if (!client.connected() && !client.available()) {
					new_clients.erase(it);
					no_free_slots = false;
					break;
				}
			}
		}
		if (no_free_slots) {
			ETHERNETSERVER_DEBUG("Max number of ethernet clients reached.");
			return;
		}
	}

	sin_size = sizeof client_addr;
	new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
	if (new_fd == -1) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			perror("accept");
		}
		return;
	}

	new_clients.push_back(new_fd);
	clients.push_back(new_fd);

	void *addr = &(((struct sockaddr_in*)&client_addr)->sin_addr);
	inet_ntop(client_addr.ss_family, addr, ipstr, sizeof ipstr);
	ETHERNETSERVER_DEBUG("New connection from %s\n", ipstr);
}
