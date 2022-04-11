/*
 * Lib/decode.c -- unsn_decode()
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

size_t unsn_decode(char **pbuffer, char const *string)
{
	char const *sptr = string;
	char *bptr = pbuffer ? *pbuffer : NULL;
	while(1) {
		int c = *sptr, d;
		if(unsn_isoctet1((unsigned char)c)) {
			if(bptr)
				*bptr++ = c;
			sptr++;
			continue;
		}
		if(c != '%')
			break;
		if(!(c = sptr[1]) || -1 == (c = xdigitval(c)))
			break;
		if(!(d = sptr[2]) || -1 == (d = xdigitval(d)))
			break;
		if(bptr)
			*bptr++ = (c << 4) | d;
		sptr += 3;
	}
	if(bptr)
		*pbuffer = bptr;
	return sptr - string;
}
