/*
 * Lib/ctype_funcs.c -- UNSN ctype functions
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

#define MAKE_CTYPE_FN(fn) \
	int (fn)(int c) \
	{ \
		return fn(c); \
	}

MAKE_CTYPE_FN(unsn_isoctet1)
MAKE_CTYPE_FN(unsn_isoctet)
MAKE_CTYPE_FN(unsn_isxdigit)
MAKE_CTYPE_FN(unsn_isbasic)
MAKE_CTYPE_FN(unsn_isunsn)
