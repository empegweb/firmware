/*
 * Lib/syntaxok.c -- unsn_syntaxok()
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

static int syntaxok_soption(char const **pp)
{
	size_t ngroups = 0;
	char const *p = *pp;
	p += unsn_decode(NULL, p);
	if(*p++ != '=')
		return 0;
	while(1) {
		if(*p == '(' /*)*/) {
			ngroups++;
			p++;
			continue;
		}
		p += unsn_decode(NULL, p);
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
		return 0;
	}
	*pp = p;
	return 1;
}

static int syntaxok_slayer(char const **pp)
{
	size_t ngroups = 0;
	char const *p = *pp;
	size_t protlen = unsn_decode(NULL, p);
	if(!protlen)
		return 0;
	p += protlen;
	if(*p != ',' && *p != '=')
		goto ret;
	if(*p == ',')
		p++;
	while(1) {
		if(*p == '(' /*)*/) {
			ngroups++;
			p++;
			if(*p == ';' || *p == /*(*/ ')')
				goto doalter;
			continue;
		}
		if(!syntaxok_soption(&p))
			return 0;
		endoption:
		if(*p == ',') {
			p++;
			continue;
		}
		if(!ngroups)
			break;
		if(*p != ';' && *p != /*(*/ ')')
			return 0;
		doalter:
		while(*p == ';')
			p++;
		if(*p == /*(*/ ')') {
			p++;
			ngroups--;
			goto endoption;
		}
		continue;
	}
	ret:
	*pp = p;
	return 1;
}

static int syntaxok_layerseq(char const **pp)
{
	size_t ngroups = 0;
	char const *p = *pp;
	while(1) {
		if(*p == '(' /*)*/) {
			ngroups++;
			p++;
			if(*p == ';' || *p == /*(*/ ')')
				goto doalter;
			continue;
		}
		if(!syntaxok_slayer(&p))
			return 0;
		endlayer:
		if(*p == '/') {
			p++;
			continue;
		}
		if(!ngroups)
			break;
		if(*p != ';' && *p != /*(*/ ')')
			return 0;
		doalter:
		while(*p == ';')
			p++;
		if(*p == /*(*/ ')') {
			p++;
			ngroups--;
			goto endlayer;
		}
		continue;
	}
	*pp = p;
	return 1;
}

int unsn_syntaxok(char const *str)
{
	return syntaxok_layerseq(&str) && !*str;
}
