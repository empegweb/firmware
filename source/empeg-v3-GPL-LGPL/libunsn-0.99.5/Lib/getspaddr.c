/*
 * Lib/getspaddr.c -- unsn_private_getspaddr()
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
# include <Compat/stdlib.h>
# include <Compat/string.h>

int unsn_private_getspaddr(int sock_fd,
	int (*getname)(int /*sock_fd*/,
		struct sockaddr * /*addr*/, socklen_t * /*addrlen*/),
	struct unsn_sockaddrinfo *addr_ret)
{
	socklen_t len;
	struct sockaddr sa, *addr;
	len = sizeof(sa);
	if(-1 == getname(sock_fd, &sa, &len))
		return -1;
	addr = malloc(len);
	if(!addr) {
		errno = ENOMEM;
		return -1;
	}
	if(len <= sizeof(sa)) {
		memcpy(addr, &sa, len);
	} else {
		if(-1 == getname(sock_fd, addr, &len)) {
			free(addr);
			return -1;
		}
	}
	addr_ret->sai_addr = addr;
	addr_ret->sai_addrlen = len;
	return 0;
}

#endif /* SUPPORT_SOCKETS */
