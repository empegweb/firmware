/*
 * Lib/Compat/inet_ntop.c -- inet_ntop()
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

#include <Compat/ip.h>

#ifdef NEED_COMPAT_INET_NTOP

# include <stdio.h>
# include <Compat/string.h>
# include <Compat/errno.h>

# if defined(AF_INET) || defined(AF_INET6)

static inline void ntop_inet(void const *cp, char *buf)
{
	unsigned char const *addr = cp;
	sprintf(buf, "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
}

# endif /* AF_INET || AF_INET6 */

# ifdef AF_INET6

static inline void ntop_inet6(void const *cp, char *buf)
{
	struct in6_addr i6a;
	unsigned char *addr = (unsigned char *)i6a.s6_addr;
	unsigned short words[8];
	int embedv4, i, ew;
	char *bptr = buf;
	int bestbase = -1, bestend = -1, bestlen = 0, thisbase = 0;
	memcpy(i6a.s6_addr, cp, 16);
	for(i = 0; i != 8; i++)
		words[i] = (addr[i<<1] << 8) | addr[(i<<1)+1];
	embedv4 = IN6_IS_ADDR_V4MAPPED(&i6a) || IN6_IS_ADDR_V4COMPAT(&i6a);
	ew = embedv4 ? 6 : 8;
	while(thisbase != ew) {
		if(words[thisbase]) {
			thisbase++;
			continue;
		}
		for(i = thisbase+1; i!=ew && !words[i]; i++) ;
		if(i-thisbase > bestlen ||
			(i-thisbase == bestlen &&
				bestbase==0 && i!=8)) {
			bestbase = thisbase;
			bestend = i;
			bestlen = i-thisbase;
		}
		thisbase = i;
	}
	if(bestbase == 0)
		*bptr++ = ':';
	for(i = 0; ; ) {
		if(i == bestbase) {
			*bptr++ = ':';
			i = bestend;
			if(i == 8)
				return;
		}
		if(i == 6 && embedv4) {
			ntop_inet(addr+12, bptr);
			return;
		}
		sprintf(bptr, "%x", (unsigned)words[i]);
		bptr = strchr(bptr, 0);
		if(++i == 8)
			return;
		*bptr++ = ':';
	}
}

# endif /* AF_INET6 */

char const *inet_ntop(int af, void const *cp, char *dst, size_t len)
{
	char buf[40];
	switch(af) {
# ifdef AF_INET
		case AF_INET:
			ntop_inet(cp, buf);
			break;
# endif
# ifdef AF_INET6
		case AF_INET6:
			ntop_inet6(cp, buf);
			break;
# endif
		default:
			errno = EAFNOSUPPORT;
			return NULL;
	}
	if(len < strlen(buf)+1) {
		errno = ENOSPC;
		return NULL;
	}
	strcpy(dst, buf);
	return dst;
}

#endif /* NEED_COMPAT_INET_NTOP */
