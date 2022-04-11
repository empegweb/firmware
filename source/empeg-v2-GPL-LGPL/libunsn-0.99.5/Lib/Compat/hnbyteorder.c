/*
 * Lib/Compat/hnbyteorder.c -- network byte order functions
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

#ifdef NEED_COMPAT_HTONL

uint32_t htonl(uint32_t hl)
{
	union {
		uint32_t nl;
		uint8_t octets[4];
	} u;
	u.octets[0] = (hl >> 24) & 0xff;
	u.octets[1] = (hl >> 16) & 0xff;
	u.octets[2] = (hl >> 8) & 0xff;
	u.octets[3] = hl & 0xff;
	return u.nl;
}

#endif /* NEED_COMPAT_HTONL */

#ifdef NEED_COMPAT_HTONS

uint16_t htons(uint16_t hs)
{
	union {
		uint16_t ns;
		uint8_t octets[2];
	} u;
	u.octets[0] = (hs >> 8) & 0xff;
	u.octets[1] = hs & 0xff;
	return u.ns;
}

#endif /* NEED_COMPAT_HTONS */

#ifdef NEED_COMPAT_NTOHL

uint32_t ntohl(uint32_t nl)
{
	union {
		uint32_t nl;
		uint8_t octets[4];
	} u;
	u.nl = nl;
	return (((uint32_t)u.octets[0]) << 24) |
		(((uint32_t)u.octets[1]) << 16) |
		(((uint32_t)u.octets[2]) << 8) |
		((uint32_t)u.octets[3]);
}

#endif /* NEED_COMPAT_NTOHL */

#ifdef NEED_COMPAT_NTOHS

uint16_t ntohs(uint16_t ns)
{
	union {
		uint16_t ns;
		uint8_t octets[2];
	} u;
	u.ns = ns;
	return (((uint16_t)u.octets[0]) << 8) | ((uint16_t)u.octets[1]);
}

#endif /* NEED_COMPAT_NTOHS */
