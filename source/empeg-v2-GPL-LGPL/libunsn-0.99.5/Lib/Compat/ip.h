/*
 * Lib/Compat/ip.h -- system compatibility hacks -- IP protocols
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

#ifndef UNSN_COMPAT_IP_H
#define UNSN_COMPAT_IP_H 1

#include <Compat/base.h>

/* system headers */

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

/* Internet byte order functions */

#if !defined(HAVE_HTONL) && !defined(htonl)
# define htonl unsn_compat_htonl
uint32_t htonl(uint32_t /*hl*/);
# define NEED_COMPAT_HTONL 1
# define HAVE_HTONL 1
#endif /* !HAVE_HTONL && !htonl */

#if !defined(HAVE_HTONS) && !defined(htons)
# define htons unsn_compat_htons
uint16_t htons(uint16_t /*hs*/);
# define NEED_COMPAT_HTONS 1
# define HAVE_HTONS 1
#endif /* !HAVE_HTONS && !htons */

#if !defined(HAVE_NTOHL) && !defined(ntohl)
# define ntohl unsn_compat_ntohl
uint32_t ntohl(uint32_t /*nl*/);
# define NEED_COMPAT_NTOHL 1
# define HAVE_NTOHL 1
#endif /* !HAVE_NTOHL && !ntohl */

#if !defined(HAVE_NTOHS) && !defined(ntohs)
# define ntohs unsn_compat_ntohs
uint16_t ntohs(uint16_t /*ns*/);
# define NEED_COMPAT_NTOHS 1
# define HAVE_NTOHS 1
#endif /* !HAVE_NTOHS && !ntohs */

/* IPv4 address definitions */

#ifndef HAVE_STRUCT_IN_ADDR
struct in_addr {
	in_addr_t s_addr;
};
# define HAVE_STRUCT_IN_ADDR 1
#endif /* !HAVE_STRUCT_IN_ADDR */

#ifndef INADDR_ANY
# define INADDR_ANY ((uint32_t) 0)
#endif

#ifndef INADDR_LOOPBACK
# define INADDR_LOOPBACK ((uint32_t) 0x7f000001)
#endif

/* IPv6 address definitions */

#ifndef HAVE_STRUCT_IN6_ADDR

struct in6_addr {
	union {
		uint8_t s6_u_8[16];
		uint16_t s6_u_16[8];
		uint32_t s6_u_32[4];
	} s6_u;
};
# define s6_addr s6_u.s6_u_8

# ifndef IN6ADDR_ANY_INIT
#  define IN6ADDR_ANY_INIT {{ {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} }}
# endif

# ifndef IN6ADDR_LOOPBACK_INIT
#  define IN6ADDR_LOOPBACK_INIT {{ {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1} }}
# endif

# define HAVE_STRUCT_IN6_ADDR 1

#endif /* !HAVE_STRUCT_IN6_ADDR */

#ifdef HAVE_VAR_IN6ADDR_ANY
# ifndef HAVE_DECLARATION_IN6ADDR_ANY
extern struct in6_addr const in6addr_any;
# endif
#else /* !HAVE_VAR_IN6ADDR_ANY */
# define in6addr_any unsn_compat_in6addr_any
extern struct in6_addr const in6addr_any;
# define NEED_COMPAT_VAR_IN6ADDR_ANY 1
# define HAVE_VAR_IN6ADDR_ANY 1
#endif /* !HAVE_VAR_IN6ADDR_ANY */

#ifdef HAVE_VAR_IN6ADDR_LOOPBACK
# ifndef HAVE_DECLARATION_IN6ADDR_LOOPBACK
extern struct in6_addr const in6addr_loopback;
# endif
#else /* !HAVE_VAR_IN6ADDR_LOOPBACK */
# define in6addr_loopback unsn_compat_in6addr_loopback
extern struct in6_addr const in6addr_loopback;
# define NEED_COMPAT_VAR_IN6ADDR_LOOPBACK 1
# define HAVE_VAR_IN6ADDR_LOOPBACK 1
#endif /* !HAVE_VAR_IN6ADDR_LOOPBACK */

#ifndef IN6ADDR_ANY_INIT
# define IN6ADDR_ANY_INIT {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
#endif

#ifndef IN6ADDR_LOOPBACK_INIT
# define IN6ADDR_LOOPBACK_INIT {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}
#endif

#ifndef IN6_IS_ADDR_UNSPECIFIED
# define IN6_IS_ADDR_UNSPECIFIED(a) \
	((a)->s6_addr[0] == 0 && (a)->s6_addr[1] == 0 && \
	 (a)->s6_addr[2] == 0 && (a)->s6_addr[3] == 0 && \
	 (a)->s6_addr[4] == 0 && (a)->s6_addr[5] == 0 && \
	 (a)->s6_addr[6] == 0 && (a)->s6_addr[7] == 0 && \
	 (a)->s6_addr[8] == 0 && (a)->s6_addr[9] == 0 && \
	 (a)->s6_addr[10] == 0 && (a)->s6_addr[11] == 0 && \
	 (a)->s6_addr[12] == 0 && (a)->s6_addr[13] == 0 && \
	 (a)->s6_addr[14] == 0 && (a)->s6_addr[15] == 0)
#endif

#ifndef IN6_IS_ADDR_LOOPBACK
# define IN6_IS_ADDR_LOOPBACK(a) \
	((a)->s6_addr[0] == 0 && (a)->s6_addr[1] == 0 && \
	 (a)->s6_addr[2] == 0 && (a)->s6_addr[3] == 0 && \
	 (a)->s6_addr[4] == 0 && (a)->s6_addr[5] == 0 && \
	 (a)->s6_addr[6] == 0 && (a)->s6_addr[7] == 0 && \
	 (a)->s6_addr[8] == 0 && (a)->s6_addr[9] == 0 && \
	 (a)->s6_addr[10] == 0 && (a)->s6_addr[11] == 0 && \
	 (a)->s6_addr[12] == 0 && (a)->s6_addr[13] == 0 && \
	 (a)->s6_addr[14] == 0 && (a)->s6_addr[15] == 1)
#endif

#ifndef IN6_IS_ADDR_MULTICAST
# define IN6_IS_ADDR_MULTICAST(a) ((a)->s6_addr[0] == 0xff)
#endif

#ifndef IN6_IS_ADDR_LINKLOCAL
# define IN6_IS_ADDR_LINKLOCAL(a) \
	((a)->s6_addr[0] == 0xfe && ((a)->s6_addr[1] & 0xc0) == 0x80)
#endif

#ifndef IN6_IS_ADDR_SITELOCAL
# define IN6_IS_ADDR_SITELOCAL(a) \
	((a)->s6_addr[0] == 0xfe && ((a)->s6_addr[1] & 0xc0) == 0xc0)
#endif

#ifndef IN6_IS_ADDR_V4MAPPED
# define IN6_IS_ADDR_V4MAPPED(a) \
	((a)->s6_addr[0] == 0 && (a)->s6_addr[1] == 0 && \
	 (a)->s6_addr[2] == 0 && (a)->s6_addr[3] == 0 && \
	 (a)->s6_addr[4] == 0 && (a)->s6_addr[5] == 0 && \
	 (a)->s6_addr[6] == 0 && (a)->s6_addr[7] == 0 && \
	 (a)->s6_addr[8] == 0 && (a)->s6_addr[9] == 0 && \
	 (a)->s6_addr[10] == 0xff && (a)->s6_addr[11] == 0xff)
#endif

#ifndef IN6_IS_ADDR_V4COMPAT
# define IN6_IS_ADDR_V4COMPAT(a) \
	((a)->s6_addr[0] == 0 && (a)->s6_addr[1] == 0 && \
	 (a)->s6_addr[2] == 0 && (a)->s6_addr[3] == 0 && \
	 (a)->s6_addr[4] == 0 && (a)->s6_addr[5] == 0 && \
	 (a)->s6_addr[6] == 0 && (a)->s6_addr[7] == 0 && \
	 (a)->s6_addr[8] == 0 && (a)->s6_addr[9] == 0 && \
	 (a)->s6_addr[10] == 0 && (a)->s6_addr[11] == 0 && \
	 ((a)->s6_addr[12] != 0 || (a)->s6_addr[13] != 0 || \
	  (a)->s6_addr[14] != 0 || (a)->s6_addr[15] > 1))
#endif

#ifndef IN6_IS_ADDR_MC_NODELOCAL
# define IN6_IS_ADDR_MC_NODELOCAL(a) \
	(IN6_IS_ADDR_MULTICAST((a)) && ((a)->s6_addr[1] & 0xf) == 0x1)
#endif

#ifndef IN6_IS_ADDR_MC_LINKLOCAL
# define IN6_IS_ADDR_MC_LINKLOCAL(a) \
	(IN6_IS_ADDR_MULTICAST((a)) && ((a)->s6_addr[1] & 0xf) == 0x2)
#endif

#ifndef IN6_IS_ADDR_MC_SITELOCAL
# define IN6_IS_ADDR_MC_SITELOCAL(a) \
	(IN6_IS_ADDR_MULTICAST((a)) && ((a)->s6_addr[1] & 0xf) == 0x5)
#endif

#ifndef IN6_IS_ADDR_MC_ORGLOCAL
# define IN6_IS_ADDR_MC_ORGLOCAL(a) \
	(IN6_IS_ADDR_MULTICAST((a)) && ((a)->s6_addr[1] & 0xf) == 0x8)
#endif

#ifndef IN6_IS_ADDR_MC_GLOBAL
# define IN6_IS_ADDR_MC_GLOBAL(a) \
	(IN6_IS_ADDR_MULTICAST((a)) && ((a)->s6_addr[1] & 0xf) == 0xe)
#endif

/* RFC 2553 numerical/presentation IP address conversion functions */

#if defined(AF_INET) || defined(AF_INET6)

# if defined(AF_INET) && !defined(INET_ADDRSTRLEN)
#  define INET_ADDRSTRLEN 16
# endif

# if defined(AF_INET6) && !defined(INET6_ADDRSTRLEN)
#  define INET6_ADDRSTRLEN 46
# endif

# ifndef HAVE_INET_PTON
#  define inet_pton unsn_compat_inet_pton
int inet_pton(int /*af*/, char const * /*src*/, void * /*dst*/);
#  define NEED_COMPAT_INET_PTON 1
#  define HAVE_INET_PTON 1
# endif /* !HAVE_INET_PTON */

# ifndef HAVE_INET_NTOP
#  define inet_ntop unsn_compat_inet_ntop
char const *inet_ntop(int /*af*/, void const * /*cp*/,
	char * /*dst*/, size_t /*len*/);
#  define NEED_COMPAT_INET_NTOP 1
#  define HAVE_INET_NTOP 1
# endif /* !HAVE_INET_NTOP */

#endif /* AF_INET || AF_INET6 */

/* IP protocol numbers */

#ifndef IPPROTO_TCP
# define IPPROTO_TCP 6
#endif

#ifndef IPPROTO_UDP
# define IPPROTO_UDP 17
#endif

#endif
