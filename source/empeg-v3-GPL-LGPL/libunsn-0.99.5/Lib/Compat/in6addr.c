/*
 * Lib/Compat/in6addr.c -- IPv6 address constants
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

#ifdef NEED_COMPAT_VAR_IN6ADDR_ANY
struct in6_addr const in6addr_any = IN6ADDR_ANY_INIT;
#endif /* NEED_COMPAT_VAR_IN6ADDR_ANY */

#ifdef NEED_COMPAT_VAR_IN6ADDR_LOOPBACK
struct in6_addr const in6addr_loopback = IN6ADDR_LOOPBACK_INIT;
#endif /* NEED_COMPAT_VAR_IN6ADDR_LOOPBACK */
