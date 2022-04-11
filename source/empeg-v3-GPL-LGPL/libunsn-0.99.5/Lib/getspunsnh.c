/*
 * Lib/getspunsnh.c -- unsn_private_getspunsn_withhints()
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

# include <Compat/stdlib.h>

char *unsn_private_getspunsn_withhints(int sock_fd,
	int (*getname)(int /*sock_fd*/,
		struct sockaddr * /*addr*/, socklen_t * /*addrlen*/),
	struct unsn_sockaddrinfo const *hints, unsigned opts)
{
	struct unsn_sockaddrinfo sai = *hints;
	char *ret;
	if(unsn_private_getspaddr(sock_fd, getname, &sai))
		return NULL;
	ret = unsn_saitounsn(&sai, opts);
	free(sai.sai_addr);
	return ret;
}

#endif /* SUPPORT_SOCKETS */
