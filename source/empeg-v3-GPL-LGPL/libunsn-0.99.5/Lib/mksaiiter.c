/*
 * Lib/mksaiiter.c -- unsn_mksaiiterator()
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

struct unsn_sai_iterator *unsn_mksaiiterator(char const *str)
{
	struct unsn_alt_iterator *ai = unsn_parse_unsn(str);
	if(!ai)
		return NULL;
	return unsn_private_mksaiiterator(ai);
}

#endif /* SUPPORT_SOCKETS */
