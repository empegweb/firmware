/*
 * Lib/ctype_table.c -- UNSN ctype table
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

#define UNSN	(UNSN_CTYPE_UNSN)
#define ALTER	(UNSN)
#define BASIC	(UNSN_CTYPE_BASIC | UNSN)
#define OCTET	(UNSN_CTYPE_OCTET | BASIC)
#define OCTET1	(UNSN_CTYPE_OCTET1 | OCTET)
#define XDIGIT	(UNSN_CTYPE_XDIGIT | OCTET1)

unsigned char unsn_private_ctype_table[2+(int)((unsigned char)-1)] = {
	0,	/* EOF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x00 to 0x0f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x10 to 0x1f */
	0,	/* space */
	0,	/* ! */
	0,	/* " */
	0,	/* # */
	0,	/* $ */
	OCTET,	/* % */
	0,	/* & */
	0,	/* ' */
	ALTER,	/* ( */
	ALTER,	/* ) */
	0,	/* * */
	0,	/* + */
	BASIC,	/* , */
	OCTET1,	/* - */
	OCTET1,	/* . */
	BASIC,	/* / */
	XDIGIT, XDIGIT, XDIGIT, XDIGIT, XDIGIT, /* 0 to 4 */
	XDIGIT, XDIGIT, XDIGIT, XDIGIT, XDIGIT, /* 5 to 9 */
	OCTET1,	/* : */
	ALTER,	/* ; */
	0,	/* < */
	BASIC,	/* = */
	0,	/* > */
	0,	/* ? */
	0,	/* @ */
	XDIGIT, XDIGIT, XDIGIT, XDIGIT, XDIGIT, XDIGIT, /* A to F */
	OCTET1, OCTET1, OCTET1, OCTET1, OCTET1, /* G to K */
	OCTET1, OCTET1, OCTET1, OCTET1, OCTET1, /* L to P */
	OCTET1, OCTET1, OCTET1, OCTET1, OCTET1, /* Q to U */
	OCTET1, OCTET1, OCTET1, OCTET1, OCTET1, /* V to Z */
	0,	/* [ */
	0,	/* \ */
	0,	/* ] */
	0,	/* ^ */
	OCTET1,	/* _ */
	0,	/* ` */
	XDIGIT, XDIGIT, XDIGIT, XDIGIT, XDIGIT, XDIGIT, /* a to f */
	OCTET1, OCTET1, OCTET1, OCTET1, OCTET1, /* g to k */
	OCTET1, OCTET1, OCTET1, OCTET1, OCTET1, /* l to p */
	OCTET1, OCTET1, OCTET1, OCTET1, OCTET1, /* q to u */
	OCTET1, OCTET1, OCTET1, OCTET1, OCTET1, /* v to z */
	0,	/* { */
	0,	/* | */
	0,	/* } */
	0,	/* ~ */
	0,	/* 0x7f */
	/* everything else is 0 */
};
