/*
 * Lib/opensfsai.c -- unsn_opensock_fromsai()
 * Copyright (C) 2000  Andrew Main
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <libunsn_pr.h>

#ifdef SUPPORT_SOCKETS

# include <Compat/socket.h>
# include <Compat/errno.h>
# include <Compat/unistd.h>

static inline int intr_socket(int family, int type, int protocol)
{
	int sock, err;
	do {
		sock = socket(family, type, protocol);
	} while(sock == -1 && errno == EINTR);
	do {
		int val = 1;
		err = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
			(char *)&val, sizeof(val));
	} while(err == -1 && errno == EINTR);
	return sock;
}

static inline int intr_bind(int sock, struct sockaddr const *addr,
	socklen_t addrlen)
{
	int ret;
	do {
		ret = bind(sock, addr, addrlen);
	} while(ret == -1 && errno == EINTR);
	return ret;
}

static inline int intr_connect(int sock, struct sockaddr const *addr,
	socklen_t addrlen)
{
	int ret;
	do {
		ret = connect(sock, addr, addrlen);
	} while(ret == -1 && errno == EINTR);
	return ret;
}

int unsn_opensock_fromsai(struct unsn_sockaddrinfo const *local_addr,
	struct unsn_sockaddrinfo const *remote_addr)
{
	struct unsn_sockaddrinfo const *socktype;
	int sock;
	if(!local_addr) {
		if(!remote_addr) {
			errno = EINVAL;
			return -1;
		}
		socktype = remote_addr;
	} else {
		if(remote_addr &&
			(local_addr->sai_family != remote_addr->sai_family ||
			 local_addr->sai_type != remote_addr->sai_type ||
			 local_addr->sai_protocol!=remote_addr->sai_protocol)) {
			errno = UNSN_EADDRINCOMPAT;
			return -1;
		}
		socktype = local_addr;
	}
	sock = intr_socket(socktype->sai_family, socktype->sai_type,
		socktype->sai_protocol);
	if(sock == -1)
		return -1;
	if(local_addr && -1 == intr_bind(sock, local_addr->sai_addr,
						local_addr->sai_addrlen)) {
		close(sock);
		return -1;
	}
	if(remote_addr && -1 == intr_connect(sock, remote_addr->sai_addr,
						remote_addr->sai_addrlen)) {
		close(sock);
		return -1;
	}
	return sock;
}

#endif /* SUPPORT_SOCKETS */
