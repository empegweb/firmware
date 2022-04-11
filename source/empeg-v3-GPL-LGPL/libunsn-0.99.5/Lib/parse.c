/*
 * Lib/parse.c -- unsn_parse_unsn()
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

/* Memory block chaining */

static inline void *chainalloc(struct chain **head, size_t sz)
{
	void *ret = malloc(sz);
	struct chain *ch = ret;
	if(ret) {
		ch->next = *head;
		*head = ch;
	}
	return ret;
}

/* Option groups are represented in intermediate interfaces by the following
   structure.  `alt' points to the node starting the option set.  The nodes
   hanging off this are normal, representing the paths through the option
   set.  The option set ends at a set of nodes chained though their `next'
   members starting at `final'.  If there is an empty path, its node is
   pointed to by `nullpath'. */

struct optionset {
	struct unsn_s_option_alt *alt;
	struct unsn_s_option_alt **lastalt;
	struct unsn_s_option_alt *final;
	struct unsn_s_option **lastfinal;
	size_t length;
	struct unsn_s_option_alt *nullpath;
};

static struct optionset null_optionset = { NULL, NULL, NULL, NULL, 0 };

static struct optionset merge_os(struct optionset os1, struct optionset os2)
{
	if(os1.nullpath) {
		struct unsn_s_option_alt *final, *nfinal;
		if(!os2.nullpath) {
			struct optionset ost = os1;
			os1 = os2;
			os2 = ost;
			goto normalmerge;
		}
		/* both option sets have empty paths */
		if(os1.final == os1.nullpath)
			return os2;
		if(os2.final == os2.nullpath)
			return os1;
		/* unusual case: both option sets have an empty alternative
		   plus at least one non-empty alternative; remove one of
		   the empties from the final list, then merge */
		final = os1.final;
		while(1) {
			nfinal = (struct unsn_s_option_alt *)final->next;
			if(nfinal == os1.nullpath)
				break;
			final = nfinal;
		}
		if(!(final->next = nfinal->next))
			os1.lastfinal = &final->next;
		os1.nullpath->alt = os2.alt->alt;
		os1.nullpath->next = os2.alt->next;
	} else {
		normalmerge:
		*os1.lastalt = os2.alt;
		os1.lastalt = os2.lastalt;
	}
	os1.nullpath = os2.nullpath;
	*os1.lastfinal = (struct unsn_s_option *)os2.final;
	os1.lastfinal = os2.lastfinal;
	if(os2.length > os1.length)
		os1.length = os2.length;
	return os1;
}

static struct optionset concat_os(struct optionset os1, struct optionset os2)
{
	struct unsn_s_option_alt *final = os1.final;
	while(final) {
		struct unsn_s_option_alt *nfinal =
			(struct unsn_s_option_alt *)final->next;
		final->alt = os2.alt->alt;
		final->next = os2.alt->next;
		final = nfinal;
	}
	os1.final = os2.final;
	os1.lastfinal = os2.lastfinal;
	os1.length += os2.length - 1;
	if(os1.nullpath && os2.nullpath != os2.alt)
		os1.nullpath = os2.nullpath;
	return os1;
}

/* Layer groups are handled the same way as option groups. */

struct layerset {
	struct unsn_s_layer_alt *alt;
	struct unsn_s_layer_alt **lastalt;
	struct unsn_s_layer_alt *final;
	struct unsn_s_layer **lastfinal;
	size_t length;
	struct unsn_s_layer_alt *nullpath;
};

static struct layerset null_layerset = { NULL, NULL, NULL, NULL, 0 };

static struct layerset merge_ls(struct layerset ls1, struct layerset ls2)
{
	if(ls1.nullpath) {
		struct unsn_s_layer_alt *final, *nfinal;
		if(!ls2.nullpath) {
			struct layerset lst = ls1;
			ls1 = ls2;
			ls2 = lst;
			goto normalmerge;
		}
		/* both layer sets have empty paths */
		if(ls1.final == ls1.nullpath)
			return ls2;
		if(ls2.final == ls2.nullpath)
			return ls1;
		/* unusual case: both layer sets have an empty alternative
		   plus at least one non-empty alternative; remove one of
		   the empties from the final list, then merge */
		final = ls1.final;
		while(1) {
			nfinal = (struct unsn_s_layer_alt *)final->next;
			if(nfinal == ls1.nullpath)
				break;
			final = nfinal;
		}
		if(!(final->next = nfinal->next))
			ls1.lastfinal = &final->next;
		ls1.nullpath->alt = ls2.alt->alt;
		ls1.nullpath->next = ls2.alt->next;
	} else {
		normalmerge:
		*ls1.lastalt = ls2.alt;
		ls1.lastalt = ls2.lastalt;
	}
	ls1.nullpath = ls2.nullpath;
	*ls1.lastfinal = (struct unsn_s_layer *)ls2.final;
	ls1.lastfinal = ls2.lastfinal;
	if(ls2.length > ls1.length)
		ls1.length = ls2.length;
	return ls1;
}

static struct layerset concat_ls(struct layerset ls1, struct layerset ls2)
{
	struct unsn_s_layer_alt *final = ls1.final;
	while(final) {
		struct unsn_s_layer_alt *nfinal =
			(struct unsn_s_layer_alt *)final->next;
		final->alt = ls2.alt->alt;
		final->next = ls2.alt->next;
		final = nfinal;
	}
	ls1.final = ls2.final;
	ls1.lastfinal = ls2.lastfinal;
	ls1.length += ls2.length - 1;
	if(ls1.nullpath && ls2.nullpath != ls2.alt)
		ls1.nullpath = ls2.nullpath;
	return ls1;
}

/* Parsing */

static struct sstring *parse_string(char const **pp, struct chain **head)
{
	char *bufp;
	char const *p = *pp;
	size_t len = unsn_decode(NULL, p);
	size_t allocsz = offsetof(struct sstring, string) + len + 1;
	struct sstring *ret;
	if(allocsz < sizeof(struct sstring))
		allocsz = sizeof(struct sstring);
	ret = chainalloc(head, allocsz);
	if(!ret) {
		errno = ENOMEM;
		return NULL;
	}
	bufp = ret->string;
	unsn_decode(&bufp, p);
	*pp = p + len;
	*bufp = 0;
	len = bufp - ret->string;
	ret->length = len;
	return ret;
}

static struct optionset parse_soption(char const **pp, struct chain **head)
{
	size_t ngroups = 0;
	char const *p = *pp;
	struct unsn_s_value_alt **vp, *v;
	struct optionset ret;
	struct unsn_s_option *opt = chainalloc(head, sizeof(*opt));
	ret.alt = chainalloc(head, sizeof(*ret.alt));
	ret.final = chainalloc(head, sizeof(*ret.final));
	if(!ret.alt || !opt || !ret.final) {
		errno = ENOMEM;
		return null_optionset;
	}
	ret.alt->alt = NULL;
	ret.alt->next = opt;
	opt->next = ret.final;
	ret.final->alt = NULL;
	ret.final->next = NULL;
	ret.lastalt = &ret.alt->alt;
	ret.lastfinal = &ret.final->next;
	ret.length = 3;
	ret.nullpath = NULL;
	vp = &opt->values;
	if(!(opt->name = parse_string(&p, head)))
		return null_optionset;
	if(*p++ != '=')
		goto badsyntax;
	while(1) {
		if(*p == '(' /*)*/) {
			ngroups++;
			p++;
			continue;
		}
		v = chainalloc(head, sizeof(*v));
		if(!v) {
			errno = ENOMEM;
			return null_optionset;
		}
		v->alt = NULL;
		if(!(v->value = parse_string(&p, head)))
			return null_optionset;
		*vp = v;
		vp = &v->alt;
		endvalue:
		if(!ngroups)
			break;
		if(*p == ';') {
			p++;
			continue;
		}
		if(*p == /*(*/ ')') {
			p++;
			ngroups--;
			goto endvalue;
		}
		badsyntax:
		errno = UNSN_EUNSNBADSYNTAX;
		return null_optionset;
	}
	*pp = p;
	return ret;
}

static struct optionset parse_optionseq(char const **pp, struct chain **head);

static struct optionset parse_optionnseq(char const **pp, struct chain **head)
{
	if(**pp == ';' || **pp == ')') {
		struct optionset os;
		os.alt = chainalloc(head, sizeof(*os.alt));
		if(!os.alt) {
			errno = ENOMEM;
			return null_optionset;
		}
		os.lastalt = &os.alt->alt;
		os.final = os.alt;
		os.lastfinal = &os.alt->next;
		os.length = 1;
		os.nullpath = os.alt;
		os.alt->alt = NULL;
		os.alt->next = NULL;
		return os;
	}
	return parse_optionseq(pp, head);
}

static struct optionset parse_aoption(char const **pp, struct chain **head)
{
	char const *p = *pp + 1;
	struct optionset ret = parse_optionnseq(&p, head);
	if(!ret.alt)
		return ret;
	while(*p == ';') {
		struct optionset os;
		p++;
		os = parse_optionnseq(&p, head);
		if(!os.alt)
			return os;
		ret = merge_os(ret, os);
	}
	if(*p != ')')
		return null_optionset;
	*pp = p+1;
	return ret;
}

static struct optionset parse_option(char const **pp, struct chain **head)
{
	return (**pp == '(' ? parse_aoption : parse_soption)(pp, head);
}

static struct optionset parse_optionseq(char const **pp, struct chain **head)
{
	char const *p = *pp;
	struct optionset ret = parse_option(&p, head);
	if(!ret.alt)
		return ret;
	while(*p == ',') {
		struct optionset os;
		p++;
		os = parse_option(&p, head);
		if(!os.alt)
			return os;
		ret = concat_os(ret, os);
	}
	*pp = p;
	return ret;
}

static struct layerset parse_slayer(char const **pp, struct chain **head)
{
	char const *p = *pp;
	struct unsn_s_option_alt *opt;
	struct layerset ret;
	struct unsn_s_layer *lyr = chainalloc(head, sizeof(*lyr));
	ret.alt = chainalloc(head, sizeof(*ret.alt));
	ret.final = chainalloc(head, sizeof(*ret.final));
	if(!ret.alt || !lyr || !ret.final) {
		errno = ENOMEM;
		return null_layerset;
	}
	ret.alt->alt = NULL;
	ret.alt->next = lyr;
	lyr->next = ret.final;
	ret.final->alt = NULL;
	ret.final->next = NULL;
	ret.lastalt = &ret.alt->alt;
	ret.lastfinal = &ret.final->next;
	ret.nullpath = NULL;
	if(!(lyr->protocol = parse_string(&p, head)))
		return null_layerset;
	if(!lyr->protocol->length) {
		errno = UNSN_EUNSNBADSYNTAX;
		return null_layerset;
	}
	if(*p == '=' || *p == ',') {
		struct optionset os;
		struct unsn_s_option_alt *final;
		p += *p == ',';
		os = parse_optionseq(&p, head);
		if(!(opt = os.alt))
			return null_layerset;
		ret.length = 2 + os.length;
		for(final = os.final; final; ) {
			struct unsn_s_option_alt *nfinal =
				(struct unsn_s_option_alt *)final->next;
			final->next = NULL;
			final = nfinal;
		}
	} else {
		opt = chainalloc(head, sizeof(*opt));
		if(!opt) {
			errno = ENOMEM;
			return null_layerset;
		}
		opt->alt = NULL;
		opt->next = NULL;
		ret.length = 3;
	}
	lyr->options = opt;
	*pp = p;
	return ret;
}

static struct layerset parse_layerseq(char const **pp, struct chain **head);

static struct layerset parse_layernseq(char const **pp, struct chain **head)
{
	if(**pp == ';' || **pp == ')') {
		struct layerset ls;
		ls.alt = chainalloc(head, sizeof(*ls.alt));
		if(!ls.alt) {
			errno = ENOMEM;
			return null_layerset;
		}
		ls.lastalt = &ls.alt->alt;
		ls.final = ls.alt;
		ls.lastfinal = &ls.alt->next;
		ls.length = 1;
		ls.nullpath = ls.alt;
		ls.alt->alt = NULL;
		ls.alt->next = NULL;
		return ls;
	}
	return parse_layerseq(pp, head);
}

static struct layerset parse_alayer(char const **pp, struct chain **head)
{
	char const *p = *pp + 1;
	struct layerset ret = parse_layernseq(&p, head);
	if(!ret.alt)
		return ret;
	while(*p == ';') {
		struct layerset ls;
		p++;
		ls = parse_layernseq(&p, head);
		if(!ls.alt)
			return ls;
		ret = merge_ls(ret, ls);
	}
	if(*p != ')')
		return null_layerset;
	*pp = p+1;
	return ret;
}

static struct layerset parse_layer(char const **pp, struct chain **head)
{
	return (**pp == '(' ? parse_alayer : parse_slayer)(pp, head);
}

static struct layerset parse_layerseq(char const **pp, struct chain **head)
{
	char const *p = *pp;
	struct layerset ret = parse_layer(&p, head);
	if(!ret.alt)
		return ret;
	while(*p == '/') {
		struct layerset ls;
		p++;
		ls = parse_layer(&p, head);
		if(!ls.alt)
			return ls;
		ret = concat_ls(ret, ls);
	}
	*pp = p;
	return ret;
}

struct unsn_alt_iterator *unsn_parse_unsn(char const *str)
{
	struct chain *chain = NULL;
	struct unsn_s_layer_alt *lyr, *final;
	struct layerset ls;
	ls = parse_layerseq(&str, &chain);
	if(!(lyr = ls.alt)) {
		retnull:
		freechain(chain);
		return NULL;
	}
	if(*str) {
		errno = UNSN_EUNSNBADSYNTAX;
		goto retnull;
	}
	for(final = ls.final; final; ) {
		struct unsn_s_layer_alt *nfinal =
			(struct unsn_s_layer_alt *)final->next;
		final->next = NULL;
		final = nfinal;
	}
	return unsn_private_mkaltiterator(ls.length, chain, lyr);
}
