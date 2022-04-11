/*
 * Lib/encode.c -- unsn_encode()
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

#include <Compat/string.h>

size_t unsn_encode(char *buffer, char const *string, ssize_t string_len)
{
	size_t olen = 0;
	char const *string_end;
	if(string_len == -1)
		string_end = strchr(string, 0);
	else
		string_end = string + string_len;
	while(string != string_end) {
		char c = *string++;
		int cval = (unsigned char)c;
		if(unsn_isoctet1(cval)) {
			if(buffer)
				*buffer++ = c;
			olen++;
		} else {
			if(buffer) {
				static char const *xdigits = "0123456789ABCDEF";
				*buffer++ = '%';
				*buffer++ = xdigits[(cval >> 4) & 0xf];
				*buffer++ = xdigits[cval & 0xf];
			}
			olen += 3;
		}
	}
	if(buffer)
		*buffer = 0;
	return olen;
}
