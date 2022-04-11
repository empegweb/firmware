/*
 * Lib/ilayers.c -- protocol layer interpretation functions
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

#include <ilayers.h>

/* Protocol lookup table */

#include <ilayers.t.ic>

/* Protocol name lookup */

static int protocmp(void const *sp, void const *pp)
{
	struct sstring const *sname = sp;
	char const * const *p = pp;
	char const *tname = *p;
	return cmpsstrstr(sname, tname);
}

static int lookup_proto(struct sstring const *s)
{
	char const * const *p = bsearch(s, protos,
		sizeof(protos)/sizeof(protos[0]), sizeof(protos[0]), protocmp);
	return p ? p-protos : -1;
}

/* Layer set freeing */

void unsn_private_freeilayers(struct ilayer *il)
{
	while(il) {
		struct ilayer *nil = il->next;
		il->free(il);
		il = nil;
	}
}

/* Layer parsing and checking */

struct ilayer *unsn_private_interplayer(struct unsn_a_layer const *l)
{
	/* check for duplicate options */
	if(l->options[0].name) {
		struct unsn_a_option const *op;
		for(op = l->options; op[1].name; op++) {
			if(!cmpsstrings(op[0].name, op[1].name)) {
				errno = UNSN_EUNSNDUPOPTION;
				return NULL;
			}
		}
	}
	/* look up protocol */
	switch(lookup_proto(l->protocol)) {
#include <ilayers.i.ic>
		default: {
			errno = UNSN_EUNSNPROTOUNREC;
			return NULL;
		}
	}
}

/* Layer set parsing and checking */

struct ilayer *unsn_private_interplayerset(
	struct unsn_a_layerset const *ls)
{
	struct ilayer *ret = NULL, **nextp = &ret;
	struct unsn_a_layer * const *lp, *l;
	int err = 0;
	/* check for errors affecting the entire layer set */
	if(ls->nlayers == 0) {
		errno = UNSN_EUNSNEMPTY;
		return NULL;
	}
	/* parse and check each layer */
	for(lp = ls->layers; l = *lp; lp++) {
		struct ilayer *il = unsn_private_interplayer(l);
		if(!il)
			err = unsn_errno_min(err, errno);
		else {
			*nextp = il;
			nextp = &il->next;
		}
	}
	if(err) {
		errno = err;
		unsn_private_freeilayers(ret);
		return NULL;
	}
	/* all OK */
	return ret;
}
