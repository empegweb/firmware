/*
 * feature.c --- convert between features and strings
 * 
 * Copyright (C) 1999  Theodore Ts'o <tytso@mit.edu>
 * 
 * This file can be redistributed under the terms of the GNU Library General
 * Public License
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "e2p.h"

/*
 * Feature set definitions (that might not be in ext2_fs.h
 */

#ifndef EXT2_FEATURE_COMPAT_DIR_PREALLOC
#define EXT2_FEATURE_COMPAT_DIR_PREALLOC	0x0001
#endif

#ifndef EXT2_FEATURE_COMPAT_IMAGIC_INODES /* for AFS, etc. */
#define EXT2_FEATURE_COMPAT_IMAGIC_INODES		0x0002
#endif

#ifndef EXT3_FEATURE_COMPAT_HAS_JOURNAL
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL		0x0004
#endif

#ifndef EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER	0x0001
#endif

#ifndef EXT2_FEATURE_RO_COMPAT_LARGE_FILE
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE	0x0002
#define i_size_high i_dir_acl
#endif

#ifndef EXT2_FEATURE_RO_COMPAT_BTREE_DIR
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR	0x0004
#endif

#ifndef EXT2_FEATURE_INCOMPAT_COMPRESSION
#define EXT2_FEATURE_INCOMPAT_COMPRESSION	0x0001
#endif

#ifndef EXT2_FEATURE_INCOMPAT_FILETYPE
#define EXT2_FEATURE_INCOMPAT_FILETYPE	0x0002
#endif

#ifndef EXT3_FEATURE_INCOMPAT_RECOVER
#define EXT3_FEATURE_INCOMPAT_RECOVER	0x0004
#endif

struct feature {
	int		compat;
	unsigned int	mask;
	const char	*string;
};

static struct feature feature_list[] = {
	{	E2P_FEATURE_COMPAT, EXT2_FEATURE_COMPAT_DIR_PREALLOC,
			"dir_prealloc" },
	{	E2P_FEATURE_COMPAT, EXT3_FEATURE_COMPAT_HAS_JOURNAL,
			"has_journal" },
	{	E2P_FEATURE_COMPAT, EXT2_FEATURE_COMPAT_IMAGIC_INODES,
			"imagic_inodes" },
	{	E2P_FEATURE_RO_INCOMPAT, EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER,
			"sparse_super" },
	{	E2P_FEATURE_RO_INCOMPAT, EXT2_FEATURE_RO_COMPAT_LARGE_FILE,
			"large_file" },
	{	E2P_FEATURE_RO_INCOMPAT, EXT2_FEATURE_RO_COMPAT_BTREE_DIR,
			"btree_dir" },
	{	E2P_FEATURE_INCOMPAT, EXT2_FEATURE_INCOMPAT_COMPRESSION,
			"compression" },
	{	E2P_FEATURE_INCOMPAT, EXT2_FEATURE_INCOMPAT_FILETYPE,
			"filetype" },
	{	E2P_FEATURE_INCOMPAT, EXT3_FEATURE_INCOMPAT_RECOVER,
			"needs_recovery" },
	{	0, 0, 0 },
};

const char *e2p_feature2string(int compat, unsigned int mask)
{
	struct feature  *f;
	static char buf[20];
	char	fchar;
	int	fnum;

	for (f = feature_list; f->string; f++) {
		if ((compat == f->compat) &&
		    (mask == f->mask))
			return f->string;
	}
	switch (compat) {
	case  E2P_FEATURE_COMPAT:
		fchar = 'C';
		break;
	case E2P_FEATURE_INCOMPAT:
		fchar = 'I';
		break;
	case E2P_FEATURE_RO_INCOMPAT:
		fchar = 'R';
		break;
	default:
		fchar = '?';
		break;
	}
	for (fnum = 0; mask >>= 1; fnum++);
	sprintf(buf, "FEATURE_%c%d", fchar, fnum);
	return buf;
}

int e2p_string2feature(char *string, int *compat_type, unsigned int *mask)
{
	struct feature  *f;
	char		*eptr;
	int		num;

	for (f = feature_list; f->string; f++) {
		if (!strcasecmp(string, f->string)) {
			*compat_type = f->compat;
			*mask = f->mask;
			return 0;
		}
	}
	if (strncasecmp(string, "FEATURE_", 8))
		return 1;

	switch (string[8]) {
	case 'c':
	case 'C':
		*compat_type = E2P_FEATURE_COMPAT;
		break;
	case 'i':
	case 'I':
		*compat_type = E2P_FEATURE_INCOMPAT;
		break;
	case 'r':
	case 'R':
		*compat_type = E2P_FEATURE_RO_INCOMPAT;
		break;
	default:
		return 1;
	}
	if (string[9] == 0)
		return 1;
	num = strtol(string+9, &eptr, 10);
	if (num > 32 || num < 0)
		return 1;
	if (*eptr)
		return 1;
	*mask = 1 << num;
	return 0;
}

static char *skip_over_blanks(char *cp)
{
	while (*cp && isspace(*cp))
		cp++;
	return cp;
}

static char *skip_over_word(char *cp)
{
	while (*cp && !isspace(*cp) && *cp != ',')
		cp++;
	return cp;
}

/*
 * Edit a feature set array as requested by the user.  The ok_array,
 * if set, allows the application to limit what features the user is
 * allowed to set or clear using this function.
 */
int e2p_edit_feature(const char *str, __u32 *compat_array, __u32 *ok_array)
{
	char	*cp, *buf, *next;
	int	neg;
	unsigned int	mask;
	int		compat_type;

	buf = malloc(strlen(str)+1);
	if (!buf)
		return 1;
	strcpy(buf, str);
	cp = buf;
	while (cp && *cp) {
		neg = 0;
		cp = skip_over_blanks(cp);
		next = skip_over_word(cp);
		if (*next == 0)
			next = 0;
		else
			*next = 0;
		switch (*cp) {
		case '-':
		case '^':
			neg++;
		case '+':
			cp++;
			break;
		}
		if (e2p_string2feature(cp, &compat_type, &mask))
			return 1;
		if (ok_array && !(ok_array[compat_type] & mask))
			return 1;
		if (neg)
			compat_array[compat_type] &= ~mask;
		else
			compat_array[compat_type] |= mask;
		cp = next ? next+1 : 0;
	}
	return 0;
}

