/*
 * Lib/Compat/sock_inet.h -- system compatibility hacks -- Internet sockets
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

#ifndef UNSN_COMPAT_SOCK_INET_H
#define UNSN_COMPAT_SOCK_INET_H 1

#include <Compat/base.h>

/* system headers */

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

/* IPv6 socket address structure */

#if defined(HAVE_STRUCT_SOCKADDR) && defined(AF_INET6) && !defined(HAVE_STRUCT_SOCKADDR_IN6)

/* If AF_INET6 is defined but struct sockaddr_in6 isn't, then we have a system
   where the kernel may or may not support IPv6, but the library hasn't
   caught up.  (E.g., Linux 2.2 with libc5.)  In this case, we can fabricate
   the socket address structure, and then at run time we can use IPv6 if the
   kernel in use actually supports it.  The structure layout is the same
   everywhere, except for the presence/absence of sin6_scope_id, and the whole
   sa_len thing.  On the whole, this *should* work, but it's not guaranteed;
   it's a hack for systems in an intermediate state.  On systems that fully
   support IPv6, we have no problem.  If AF_INET6 is missing, we don't even
   try. */

# include <Compat/ip.h>

struct sockaddr_in6 {
# ifdef HAVE_STRUCTMEM_SOCKADDR_SA_LEN
	uint8_t sin6_len;
	uint8_t sin6_family;
# else /* !HAVE_STRUCTMEM_SOCKADDR_SA_LEN */
	sa_family_t sin6_family;
# endif /* !HAVE_STRUCTMEM_SOCKADDR_SA_LEN */
	in_port_t sin6_port;
	uint32_t sin6_flowinfo;
	struct in6_addr sin6_addr;
	/* no sin6_scope_id: in cases where the kernel supports IPv6 but the
	   headers lack this structure, it's probably an early implementation,
	   based on RFC 2133 rather than RFC 2553.  If the kernel actually
	   is based on RFC 2553, and doesn't accept the older structure, the
	   headers will just have to be right. */
};

# define HAVE_STRUCT_SOCKADDR_IN6 1

#endif /* HAVE_STRUCT_SOCKADDR && AF_INET6 && !HAVE_STRUCT_SOCKADDR_IN6 */

/* IP pseudo-protocol number */

#ifndef IPPROTO_RAW
# define IPPROTO_RAW 0xff
#endif

#endif
