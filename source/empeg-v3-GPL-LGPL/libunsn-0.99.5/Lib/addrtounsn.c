/*
 * Lib/addrtounsn.c -- unsn_addrtounsn()
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

char *unsn_addrtounsn(int sock_fd,
	struct sockaddr const *addr, socklen_t addrlen, unsigned opts)
{
	struct unsn_sockaddrinfo sai;
	int sock_af;
	char *ret = NULL;
	sai.sai_addr = (struct sockaddr *)addr;
	sai.sai_addrlen = addrlen;
	{
		socklen_t len = sizeof(sai.sai_type);
		if(-1 == getsockopt(sock_fd, SOL_SOCKET, SO_TYPE,
				(char *)&sai.sai_type, &len))
			return NULL;
	}
	/* what happens next depends on the address family */
	sock_af = sai.sai_addr->sa_family;
	switch(sock_af) {
# if defined(SUPPORT_SOCK_INET) || defined(SUPPORT_SOCK_INET6)
#  ifdef SUPPORT_SOCK_INET
		case AF_INET:
			sai.sai_family = PF_INET;
			goto ip;
#  endif
#  ifdef SUPPORT_SOCK_INET6
		case AF_INET6:
			sai.sai_family = PF_INET6;
			goto ip;
#  endif
		ip: {
			/* For stream and datagram type sockets, we must be
			   using TCP or UDP, and have the address.  For raw
			   IP sockets, we have no way to get the protocol
			   number, and so can't form a complete address. */
			if(sai.sai_type == SOCK_RAW) {
				errno = UNSN_ESOCKNOADDR;
			} else if(sai.sai_type == SOCK_STREAM ||
					sai.sai_type == SOCK_DGRAM) {
				sai.sai_protocol = 0;
				ret = unsn_saitounsn(&sai, opts);
			} else
				errno = ESOCKTNOSUPPORT;
			break;
		}
# endif /* SUPPORT_SOCK_INET || SUPPORT_SOCK_INET6 */
# ifdef SUPPORT_SOCK_LOCAL
		case AF_LOCAL: {
			sai.sai_type = PF_LOCAL;
			sai.sai_protocol = 0;
			ret = unsn_saitounsn(&sai, opts);
			break;
		}
# endif /* SUPPORT_SOCK_LOCAL */
		default: {
			errno = EPFNOSUPPORT;
			break;
		}
	}
	return ret;
}

#endif /* SUPPORT_SOCKETS */
