/*
 * Lib/saitounsn.c -- unsn_saitounsn()
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

# include <Compat/errno.h>

char *unsn_saitounsn(struct unsn_sockaddrinfo const *sai,
	unsigned opts)
{
	if(!sai->sai_addr) {
		errno = sai->sai_family;
		return NULL;
	}
	return unsn_aitounsn(sai->sai_family, sai->sai_type,
		sai->sai_protocol, sai->sai_addr, sai->sai_addrlen, opts);
}

#endif /* SUPPORT_SOCKETS */
