/*
 * Lib/Compat/inet_pton.c -- inet_pton()
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

#ifdef NEED_COMPAT_INET_PTON

# include <Compat/string.h>
# include <stdio.h>
# include <Compat/errno.h>

# if defined(AF_INET) || defined(AF_INET6)

static int pton_inet(char const *src, void *dst)
{
	unsigned char buf[4];
	int octets = 0;
	while(1) {
		char c = *src++;
		unsigned val;
		if(c < '0' || c > '9')
			return 0;
		val = c - '0';
		if((c = *src) >= '0' && c <= '9') {
			val = val*10 + c-'0';
			c = *++src;
			if(c >= '0' && c <= '9') {
				val = val*10 + c-'0';
				c = *++src;
			}
		}
		if(val > 255)
			return 0;
		buf[octets++] = val;
		if(octets == 4) {
			if(c)
				return 0;
			memcpy(dst, buf, 4);
			return 1;
		}
		if(c != '.')
			return 0;
		src++;
	}
}

# endif /* AF_INET || AF_INET6 */

# ifdef AF_INET6

static inline int xdigitval(char c)
{
	if(c >= '0' && c <= '9')
		return c - '0';
	if(c >= 'a' && c <= 'f')
		return 10 + c - 'a';
	if(c >= 'A' && c <= 'F')
		return 10 + c - 'A';
	return -1;
}

#  define xdigitp(c) (xdigitval((c)) != -1)

static inline int pton_inet6(char const *src, void *dst)
{
	unsigned char buf[16];
	int nwords = 0;
	int gapstart = -1;
	if(src[0] == ':' && src[1] == ':') {
		src += 2;
		goto aftergap;
	}
	while(1) {
		char const *wstart = src;
		unsigned val;
		if(!xdigitp(src[0]))
			return 0;
		val = xdigitval(*src++);
		if(xdigitp(*src)) {
			val = (val<<4) | xdigitval(*src++);
			if(xdigitp(*src)) {
				val = (val<<4) | xdigitval(*src++);
				if(xdigitp(*src))
					val = (val<<4) | xdigitval(*src++);
			}
		}
		if(*src == '.') {
			if(nwords > 6)
				return 0;
			val = pton_inet(wstart, buf+nwords*2);
			if(!val)
				return 0;
			nwords += 2;
			break;
		}
		if(*src && *src != ':')
			return 0;
		if(nwords == 8)
			return 0;
		buf[nwords<<1] = val>>8;
		buf[(nwords<<1)+1] = val & 0xff;
		nwords++;
		if(!*src++)
			break;
		if(*src == ':') {
			if(gapstart != -1)
				return 0;
			src++;
			aftergap:
			gapstart = nwords;
			if(!*src)
				break;
		}
	}
	if(gapstart == -1) {
		if(nwords != 8)
			return 0;
	} else {
		int gaplen = 8-nwords;
		int gapend = gapstart+gaplen;
		memmove(buf+gapend*2, buf+gapstart*2,
			(nwords-gapstart)*2);
		bzero(buf+gapstart*2, gaplen*2);
	}
	memcpy(dst, buf, 16);
	return 1;
}

# endif /* AF_INET6 */

int inet_pton(int af, char const *src, void *dst)
{
	switch(af) {
# ifdef AF_INET
		case AF_INET:
			return pton_inet(src, dst);
# endif
# ifdef AF_INET6
		case AF_INET6:
			return pton_inet6(src, dst);
# endif
		default:
			errno = EAFNOSUPPORT;
			return -1;
	}
}

#endif /* NEED_COMPAT_INET_PTON */
