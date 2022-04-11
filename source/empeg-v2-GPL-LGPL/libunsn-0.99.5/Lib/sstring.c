/*
 * Lib/sstring.c -- sstring functions
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

int cmpsstrings(struct sstring const *s1, struct sstring const *s2)
{
	size_t l1 = s1->length, l2 = s2->length;
	size_t l = l1 < l2 ? l1 : l2;
	int ret = memcmp(s1->string, s2->string, l);
	if(ret)
		return ret;
	return l1 < l2 ? -1 : l1 > l2 ? 1 : 0;
}

int cmpsstrstr(struct sstring const *s1, char const *s2)
{
	size_t l1 = s1->length, l2 = strlen(s2);
	size_t l = l1 < l2 ? l1 : l2;
	int ret = memcmp(s1->string, s2, l);
	if(ret)
		return ret;
	return l1 < l2 ? -1 : l1 > l2 ? 1 : 0;
}
