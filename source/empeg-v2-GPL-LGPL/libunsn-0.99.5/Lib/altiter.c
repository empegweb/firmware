/*
 * Lib/altiter.c -- unsn_alt_iterator functions
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

#include <Compat/stdlib.h>
#include <Compat/errno.h>

void unsn_alt_iterator_free(struct unsn_alt_iterator *i)
{
	if(!i)
		return;
	freechain(i->chain);
	free(i);
}

int unsn_alt_iterator_nonnull(struct unsn_alt_iterator const *i)
{
	return i->endelems != NULL;
}

void unsn_alt_iterator_nullify(struct unsn_alt_iterator *i)
{
	i->endelems = NULL;
}

int unsn_alt_iterator_advance(struct unsn_alt_iterator *i)
{
	struct it_elem *first_elem = i->elements;
	struct it_elem *e = i->endelems, *ee;
	struct unsn_s_layer *current_layer;
	struct unsn_s_option *current_option;
	/* if null, set to first alternative */
	if(!e) {
		e = first_elem;
		e->type = 'l';
		e->u.l = i->start;
		goto ok_layer;
	}
	/* search backwards until we can increment */
	while(1) {
		if(e == first_elem) {
			i->endelems = NULL;
			return 0;
		}
		e--;
		switch(e->type) {
			case 'l':
				if(e->u.l = e->u.l->alt)
					goto ok_layer;
				break;
			case 'o':
				if(e->u.o = e->u.o->alt)
					goto ok_option;
				break;
			case 'v':
				if(e->u.v = e->u.v->alt)
					goto ok_value;
				break;
		}
	}
	/* choose the first alternative of each thing that follows */
	ok_layer:
	current_layer = e->u.l->next;
	dolayer:
	if(!current_layer) {
		i->endelems = e+1;
		return 1;
	}
	(++e)->type = 'o';
	e->u.o = current_layer->options;
	current_option = e->u.o->next;
	goto dooption;
	ok_option:
	current_option = e->u.o->next;
	for(ee = e-1; ee->type != 'l'; ee--) ;
	current_layer = ee->u.l->next;
	dooption:
	if(!current_option) {
		(++e)->type = 'l';
		e->u.l = current_layer->next;
		current_layer = e->u.l->next;
		goto dolayer;
	}
	(++e)->type = 'v';
	e->u.v = current_option->values;
	goto dovalue;
	ok_value:
	current_option = (e-1)->u.o->next;
	for(ee = e-2; ee->type != 'l'; ee--) ;
	current_layer = ee->u.l->next;
	dovalue:
	(++e)->type = 'o';
	e->u.o = current_option->next;
	current_option = e->u.o->next;
	goto dooption;
}

struct unsn_alt_iterator *unsn_private_mkaltiterator(size_t length,
	struct chain *chain, struct unsn_s_layer_alt *start)
{
	struct unsn_alt_iterator *i =
		malloc(offsetof(struct unsn_alt_iterator, elements) +
			length * sizeof(struct it_elem));
	if(!i) {
		freechain(chain);
		errno = ENOMEM;
		return NULL;
	}
	i->chain = chain;
	i->start = start;
	i->endelems = NULL;
	return i;
}
