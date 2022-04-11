/*
 * Lib/copysai.c -- unsn_copysai()
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
# include <Compat/stdlib.h>
# include <Compat/string.h>
# include <Compat/errno.h>

struct block {
	struct unsn_sockaddrinfo sai;
	struct sockaddr sa;
};

struct unsn_sockaddrinfo *unsn_copysai(
	struct unsn_sockaddrinfo const *src)
{
	if(src->sai_addr) {
		struct block *ret;
		size_t blocksz = offsetof(struct block, sa) + src->sai_addrlen;
		if(blocksz < sizeof(struct block))
			blocksz = sizeof(struct block);
		ret = malloc(blocksz);
		if(!ret) {
			errno = ENOMEM;
			return NULL;
		}
		ret->sai = *src;
		memcpy(&ret->sa, ret->sai.sai_addr, ret->sai.sai_addrlen);
		ret->sai.sai_addr = &ret->sa;
		return &ret->sai;
	} else {
		struct unsn_sockaddrinfo *ret = malloc(sizeof(*ret));
		if(!ret) {
			errno = ENOMEM;
			return NULL;
		}
		*ret = *src;
		return ret;
	}
}

#endif /* SUPPORT_SOCKETS */
