/*
 * Lib/unparse1.c -- unsn_unparse1()
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

#include <Compat/errno.h>
#include <Compat/stdlib.h>

static size_t unparsestr(char **pp, struct sstring const *s)
{
	char *p = *pp;
	size_t len = unsn_encode(p, s->string, s->length);
	if(p)
		*pp = p + len;
	return len;
}

static size_t unparse1(struct unsn_alt_iterator const *i, char *p)
{
	size_t len = 0;
	struct it_elem const *e = i->elements;
	struct unsn_s_layer const *l;
	if(!(l = e->u.l->next)) {
		if(p)
			strcpy(p, "()");
		return 3;
	}
	while(1) {
		struct unsn_s_option const *o;
		len += unparsestr(&p, l->protocol);
		while(o = (++e)->u.o->next) {
			if(p)
				*p++ = ',';
			len++;
			len += unparsestr(&p, o->name);
			if(p)
				*p++ = '=';
			len++;
			len += unparsestr(&p, (++e)->u.v->value);
		}
		len++;
		if(!(l = (++e)->u.l->next))
			break;
		if(p)
			*p++ = '/';
	}
	return len;
}

char *unsn_unparse1(struct unsn_alt_iterator const *i)
{
	size_t len = unparse1(i, NULL);
	char *ret = malloc(len);
	if(!ret) {
		errno = ENOMEM;
		return NULL;
	}
	unparse1(i, ret);
	return unsn_canonize(ret, ret);
}
