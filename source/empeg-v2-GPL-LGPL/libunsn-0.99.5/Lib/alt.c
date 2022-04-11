/*
 * Lib/alt.c -- single-alternative structures
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

void unsn_private_freealt(struct unsn_a_layerset *a)
{
	struct unsn_a_layer **lp;
	if(!a)
		return;
	for(lp = a->layers; *lp; lp++)
		free(*lp);
	free(a);
}

static int optioncmp(void const *p1, void const *p2)
{
	struct unsn_a_option const *o1 = p1, *o2 = p2;
	return cmpsstrings(o1->name, o2->name);
}

struct unsn_a_layerset *unsn_private_getalt(
	struct unsn_alt_iterator const *i)
{
	struct unsn_a_layerset *ret;
	struct unsn_a_layer **lp;
	struct unsn_s_layer const *l;
	struct it_elem const *e = i->elements;
	size_t nlayers = 0;
	while(e++->u.l->next) {
		nlayers++;
		while(e++->u.o->next)
			e++;
	}
	ret = malloc(sizeof(struct unsn_a_layerset) +
		nlayers * sizeof(struct unsn_a_layer *));
	if(!ret) {
		errno = ENOMEM;
		return NULL;
	}
	ret->nlayers = nlayers;
	lp = ret->layers;
	for(e = i->elements; l = e++->u.l->next; ) {
		struct unsn_a_layer *lyr;
		struct unsn_a_option *op;
		struct unsn_s_option const *o;
		size_t noptions = 0;
		struct it_elem const *ee = e;
		for(ee = e; ee++->u.o->next; ee++)
			noptions++;
		lyr = malloc(sizeof(struct unsn_a_layer) +
			noptions * sizeof(struct unsn_a_option));
		if(!lyr) {
			while(lp != ret->layers)
				free(*--lp);
			errno = ENOMEM;
			return NULL;
		}
		*lp++ = lyr;
		lyr->protocol = l->protocol;
		lyr->noptions = noptions;
		op = lyr->options;
		while(o = e++->u.o->next) {
			op->name = o->name;
			op->value = e++->u.v->value;
			op++;
		}
		op->name = NULL;
		op->value = NULL;
		qsort(lyr->options, noptions, sizeof(struct unsn_a_option),
			optioncmp);
	}
	*lp = NULL;
	return ret;
}
