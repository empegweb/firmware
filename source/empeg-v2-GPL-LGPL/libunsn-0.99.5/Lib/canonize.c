/*
 * Lib/canonize.c -- unsn_canonize()
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

/* utility functions */

static inline int xdigitval(char c)
{
	if(c >= '0' && c <= '9')
		return c - '0';
	if(c >= 'a' && c <= 'f')
		return 10 + c - 'a';
	if(c >= 'A' && c <= 'F')
		return 10 + c - 'A';
	return -1;
}

static char *skipparen(char const *s)
{
	size_t parens = 0;
	while(1) {
		char c = *s++;
		if(c == '(') {
			parens++;
		} else if(c == ')') {
			if(!parens--)
				return (char *)s;
		}
	}
}

static char *nextdelim(char const *s, char const *delims)
{
	while(1) {
		char c = *s++;
		if(c == '(')
			s = skipparen(s);
		else if(strchr(delims, c))
			return (char *)s-1;
	}
}

/* lexical comparison */

static int lexcmp_list(char const *s, char const *t,
	int (*compare)(char const *s, char const *t),
	char const *delims, char ldelim)
{
	while(1) {
		int c = compare(s, t);
		if(c)
			return c;
		s = nextdelim(s, delims);
		t = nextdelim(t, delims);
		if(*s != ldelim && *t != ldelim)
			return 0;
		if(*s != ldelim)
			return -1;
		if(*t != ldelim)
			return 1;
		s++;
		t++;
	}
}

static inline int lexcmp_altern(char const *s, char const *t,
	int (*compare)(char const *s, char const *t))
{
	return lexcmp_list(s+1, t+1, compare, ";)", ';');
}

static int lexcmp_string(char const *s, char const *t)
{
	while(1) {
		unsigned char sc = (unsigned char)*s++;
		unsigned char tc = (unsigned char)*t++;
		if(!unsn_isoctet(sc) && !unsn_isoctet(tc))
			return 0;
		if(!unsn_isoctet(sc))
			return -1;
		if(!unsn_isoctet(tc))
			return 1;
		if(sc == '%') {
			sc = (xdigitval(s[0]) << 4) | xdigitval(s[1]);
			s += 2;
		}
		if(tc == '%') {
			tc = (xdigitval(t[0]) << 4) | xdigitval(t[1]);
			t += 2;
		}
		if(sc < tc)
			return -1;
		if(sc > tc)
			return 1;
	}
}

static int lexcmp_value(char const *s, char const *t)
{
	int ssolid = *s != '(';
	int tsolid = *t != '(';
	if(ssolid && tsolid)
		return lexcmp_string(s, t);
	if(ssolid)
		return -1;
	if(tsolid)
		return 1;
	return lexcmp_altern(s, t, lexcmp_string);
}

static inline int lexcmp_soption(char const *s, char const *t)
{
	int c = lexcmp_string(s, t);
	if(c)
		return c;
	s = strchr(s, '=') + 1;
	t = strchr(t, '=') + 1;
	return lexcmp_value(s, t);
}

static int lexcmp_optionseq(char const *s, char const *t);

static int lexcmp_option(char const *s, char const *t)
{
	int ssolid = *s != '(';
	int tsolid = *t != '(';
	if(ssolid && tsolid)
		return lexcmp_soption(s, t);
	if(ssolid)
		return -1;
	if(tsolid)
		return 1;
	return lexcmp_altern(s, t, lexcmp_optionseq);
}

static int lexcmp_optionseq(char const *s, char const *t)
{
	int sempty = !!strchr("/;)", *s);
	int tempty = !!strchr("/;)", *t);
	if(sempty && tempty)
		return 0;
	if(sempty)
		return -1;
	if(tempty)
		return 1;
	return lexcmp_list(s, t, lexcmp_option, ",/;)", ',');
}

static inline int lexcmp_slayer(char const *s, char const *t)
{
	int c = lexcmp_string(s, t);
	if(c)
		return c;
	s += unsn_decode(NULL, s);
	t += unsn_decode(NULL, t);
	if(*s == ',')
		s++;
	if(*t == ',')
		t++;
	return lexcmp_optionseq(s, t);
}

static int lexcmp_layerseq(char const *s, char const *t);

static int lexcmp_layer(char const *s, char const *t)
{
	int ssolid = *s != '(';
	int tsolid = *t != '(';
	if(ssolid && tsolid)
		return lexcmp_slayer(s, t);
	if(ssolid)
		return -1;
	if(tsolid)
		return 1;
	return lexcmp_altern(s, t, lexcmp_layerseq);
}

static int lexcmp_layerseq(char const *s, char const *t)
{
	int sempty = !!strchr(";)", *s);
	int tempty = !!strchr(";)", *t);
	if(sempty && tempty)
		return 0;
	if(sempty)
		return -1;
	if(tempty)
		return 1;
	return lexcmp_list(s, t, lexcmp_layer, "/;)", '/');
}

/* sort in place: sort_in_place() takes a set of NUL-terminated strings,
   adjacent in a buffer, and sorts them in place according to the
   comparison function provided.  Duplicate strings are optionally removed.
   The number of strings remaining after duplicate removal is returned.
   We expect to have only small numbers of items to sort, so
   the trivial O(n^2) algorithm is used. */

static void reverse(char *p, char *end)
{
	while(p != end && (p+1) != end) {
		char c = *p;
		*p++ = *--end;
		*end = c;
	}
}

static size_t sort_in_place(char *buf, size_t nitems,
	int (*compare)(char const *s, char const *t), int uniq)
{
	size_t itemsleft, i;
	char *endbuf;
	if(nitems < 2)
		return nitems;
	for(endbuf = buf, i = nitems; i--; )
		endbuf = strchr(endbuf, 0) + 1;
	for(itemsleft = nitems; itemsleft--; ) {
		char *lowest = buf, *p = buf;
		for(i = itemsleft; i--; ) {
			int c;
			p = strchr(p, 0) + 1;
			c = compare(p, lowest);
			if(!c && uniq) {
				/* duplicate: remove this one */
				char *e = strchr(p, 0) + 1;
				memmove(p, e, endbuf-e);
				endbuf -= e-p;
				p--;
				nitems--;
				itemsleft--;
				continue;
			}
			if(c < 0)
				lowest = p;
		}
		if(lowest != buf) {
			char *lend = strchr(lowest, 0) + 1;
			reverse(buf, lowest);
			reverse(lowest, lend);
			reverse(buf, lend);
		}
		buf = strchr(buf, 0) + 1;
	}
	return nitems;
}

/* canonization */

static void canon_altern(char **qp, char const **pp,
	void (*canon_branch)(char **qp, char const **pp),
	int (*lexcmp_branch)(char const *s, char const *t))
{
	char const *p = *pp;
	char *q = *qp;
	char *qstart, *qend;
	char c;
	size_t nalternatives = 0, i;
	p++;
	qstart = q;
	q++;
	/* first collect the alternatives */
	do {
		nalternatives++;
		canon_branch(&q, &p);
		c = *p++;
		*q++ = 0;
	} while(c == ';');
	qend = q;
	/* flatten alternations */
	for(q = qstart+1, i = nalternatives; i--; ) {
		if(*q == '(') {
			char *end = skipparen(q+1);
			if(!*end) {
				size_t len = (end-1) - (q+1);
				memmove(q, q+1, len);
				memmove(end-2, end, qend-end);
				qend -= 2;
				while(*(q = nextdelim(q, ";"))) {
					*q++ = 0;
					nalternatives++;
				}
			}
		}
		q = strchr(q, 0) + 1;
	}
	/* sort and uniquize the alternatives */
	nalternatives =
		sort_in_place(qstart+1, nalternatives, lexcmp_branch, 1);
	if(nalternatives == 1) {
		/* only one alternative: drop the grouping */
		size_t len = strlen(qstart+1);
		memmove(qstart, qstart+1, len);
		q = qstart + len;
	} else {
		/* fix up the alternation characters */
		*qstart = '(';
		for(q = qstart+1; --nalternatives; ) {
			q = strchr(q, 0);
			*q++ = ';';
		}
		q = strchr(q, 0);
		*q++ = /*(*/ ')';
	}
	*pp = p;
	*qp = q;
}

static void canon_string(char **qp, char const **pp)
{
	char const *p = *pp;
	char *q = *qp;
	while(1) {
		if(unsn_isoctet1((unsigned char)*p)) {
			*q++ = *p++;
		} else if(*p == '%') {
			int c = (xdigitval(p[1]) << 4) | xdigitval(p[2]);
			p += 3;
			if(unsn_isoctet1(c)) {
				*q++ = c;
			} else {
				static char const *xdigits = "0123456789ABCDEF";
				*q++ = '%';
				*q++ = xdigits[(c >> 4) & 0xf];
				*q++ = xdigits[c & 0xf];
			}
		} else
			break;
	}
	*pp = p;
	*qp = q;
}

static void canon_value(char **qp, char const **pp)
{
	char const *p = *pp;
	char *q = *qp;
	char *qstart;
	size_t ngroups = 0;
	size_t nalternatives = 0;
	/* simple case: a plain value */
	if(*p != '(' /*)*/) {
		canon_string(qp, pp);
		return;
	}
	/* alternation: */
	p++;
	qstart = q;
	q++;
	/* first collect the alternatives, flattening the alternation */
	while(1) {
		char c;
		if(*p == '(' /*)*/) {
			ngroups++;
			p++;
			continue;
		}
		nalternatives++;
		canon_string(&q, &p);
		c = *p++;
		*q++ = 0;
		while(c == /*(*/ ')') {
			if(!ngroups--)
				goto gotall;
			c = *p++;
		}
	}
	gotall:
	/* sort and uniquize the alternatives */
	nalternatives =
		sort_in_place(qstart+1, nalternatives, lexcmp_string, 1);
	if(nalternatives == 1) {
		/* only one alternative: drop the grouping */
		size_t len = strlen(qstart+1);
		memmove(qstart, qstart+1, len);
		q = qstart + len;
	} else {
		/* fix up the alternation characters */
		*qstart = '(';
		for(q = qstart+1; --nalternatives; ) {
			q = strchr(q, 0);
			*q++ = ';';
		}
		q = strchr(q, 0);
		*q++ = /*(*/ ')';
	}
	*pp = p;
	*qp = q;
}

static inline void canon_soption(char **qp, char const **pp)
{
	char const *p = *pp;
	char *q = *qp;
	canon_string(&q, &p);
	*q++ = '=';
	p++;
	canon_value(&q, &p);
	*pp = p;
	*qp = q;
}

static void canon_optionseq(char **qp, char const **pp);

static void canon_option(char **qp, char const **pp)
{
	if(**pp == '(')
		canon_altern(qp, pp, canon_optionseq, lexcmp_optionseq);
	else
		canon_soption(qp, pp);
}

static void canon_optionseq(char **qp, char const **pp)
{
	char const *p = *pp;
	char *q = *qp;
	char *qstart = q;
	char c;
	size_t noptions = 0;
	if(strchr(";)", *p))
		return;
	/* first collect the options, omitting empty options */
	do {
		char *oq = q;
		canon_option(&q, &p);
		c = *p++;
		if(oq != q) {
			noptions++;
			*q++ = 0;
			while(*(oq = nextdelim(oq, ","))) {
				*oq++ = 0;
				noptions++;
			}
		}
	} while(c == ',');
	p--;
	if(noptions) {
		/* sort the options */
		noptions = sort_in_place(qstart, noptions, lexcmp_option, 0);
		/* fix up the delimiter characters */
		for(q = qstart; --noptions; ) {
			q = strchr(q, 0);
			*q++ = ',';
		}
		q = strchr(q, 0);
		*q = c;
	}
	*pp = p;
	*qp = q;
}

static inline void canon_slayer(char **qp, char const **pp)
{
	char const *p = *pp;
	char *q = *qp;
	int havecomma;
	canon_string(&q, &p);
	havecomma = *p == ',';
	if(havecomma || *p == '=') {
		char *qstart = q;
		if(havecomma) {
			*q++ = ',';
			p++;
		}
		canon_optionseq(&q, &p);
		if(q == qstart+havecomma)
			q = qstart;
		else if(havecomma && qstart[1] == '=')
			memmove(qstart, qstart+1, --q - qstart);
	}
	*pp = p;
	*qp = q;
}

static void canon_layerseq(char **qp, char const **pp);

static void canon_layer(char **qp, char const **pp)
{
	if(**pp == '(')
		canon_altern(qp, pp, canon_layerseq, lexcmp_layerseq);
	else
		canon_slayer(qp, pp);
}

static void canon_layerseq(char **qp, char const **pp)
{
	char const *p = *pp;
	char *q = *qp;
	char c;
	int havelayer = 0;
	if(strchr(";)", *p))
		return;
	do {
		char *oq = q;
		canon_layer(&q, &p);
		c = *p++;
		if(oq != q) {
			havelayer = 1;
			*q++ = '/';
		}
	} while(c == '/');
	p--;
	if(havelayer)
		*--q = c;
	*pp = p;
	*qp = q;
}

/* driver function */

char *unsn_canonize(char *dst, char const *src)
{
	char *d = dst;
	/* first check syntax, for the error return */
	if(!unsn_syntaxok(src))
		return NULL;
	/* canonize the protocol layer sequence */
	canon_layerseq(&d, &src);
	/* special case representation */
	if(d == dst) {
		*d++ = '(';
		*d++ = ')';
	}
	/* NUL terminate */
	*d = 0;
	return dst;
}
