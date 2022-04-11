/*
 * chattr.c		- Change file attributes on an ext2 file system
 *
 * Copyright (C) 1993, 1994  Remy Card <card@masi.ibp.fr>
 *                           Laboratoire MASI, Institut Blaise Pascal
 *                           Universite Pierre et Marie Curie (Paris VI)
 *
 * This file can be redistributed under the terms of the GNU General
 * Public License
 */

/*
 * History:
 * 93/10/30	- Creation
 * 93/11/13	- Replace stat() calls by lstat() to avoid loops
 * 94/02/27	- Integrated in Ted's distribution
 * 98/12/29	- Ignore symlinks when working recursively (G M Sipe)
 * 98/12/29	- Display version info only when -V specified (G M Sipe)
 */

#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <sys/param.h>
#include <sys/stat.h>
#include <linux/ext2_fs.h>

#ifndef S_ISLNK			/* So we can compile even with gcc-warn */
# ifdef __S_IFLNK
#  define S_ISLNK(mode)	 __S_ISTYPE((mode), __S_IFLNK)
# else
#  define S_ISLNK(mode)  0
# endif
#endif

#include "et/com_err.h"
#include "e2p/e2p.h"

#include "../version.h"
#include "nls-enable.h"

static const char * program_name = "chattr";

static int add = 0;
static int rem = 0;
static int set = 0;
static int set_version = 0;

static unsigned long version;

static int recursive = 0;
static int verbose = 0;

static unsigned long af;
static unsigned long rf;
static unsigned long sf;

static void fatal_error(const char * fmt_string, int errcode)
{
	fprintf (stderr, fmt_string, program_name);
	exit (errcode);
}

#define usage() fatal_error(_("usage: %s [-RV] [-+=AacdisSu] [-v version] files...\n"), \
			     1)

static int decode_arg (int * i, int argc, char ** argv)
{
	char * p;
	char * tmp;

	switch (argv[*i][0])
	{
	case '-':
		for (p = &argv[*i][1]; *p; p++)
			switch (*p)
			{
			case 'A':
				rf |= EXT2_NOATIME_FL;
				rem = 1;
				break;
			case 'R':
				recursive = 1;
				break;
			case 'S':
				rf |= EXT2_SYNC_FL;
				rem = 1;
				break;
			case 'V':
				verbose = 1;
				break;
			case 'a':
				rf |= EXT2_APPEND_FL;
				rem = 1;
				break;
			case 'c':
				rf |= EXT2_COMPR_FL;
				rem = 1;
				break;
			case 'd':
				rf |= EXT2_NODUMP_FL;
				rem = 1;
				break;
			case 'i':
				rf |= EXT2_IMMUTABLE_FL;
				rem = 1;
				break;
			case 's':
				rf |= EXT2_SECRM_FL;
				rem = 1;
				break;
			case 'u':
				rf |= EXT2_UNRM_FL;
				rem = 1;
				break;
			case 'v':
				(*i)++;
				if (*i >= argc)
					usage ();
				version = strtol (argv[*i], &tmp, 0);
				if (*tmp)
				{
					com_err (program_name, 0,
						 _("bad version - %s\n"), argv[*i]);
					usage ();
				}
				set_version = 1;
				break;
			default:
				fprintf (stderr, _("%s: Unrecognized argument: %c\n"),
					 program_name, *p);
				usage ();
			}
		break;
	case '+':
		add = 1;
		for (p = &argv[*i][1]; *p; p++)
			switch (*p)
			{
			case 'A':
				af |= EXT2_NOATIME_FL;
				break;
			case 'S':
				af |= EXT2_SYNC_FL;
				break;
			case 'a':
				af |= EXT2_APPEND_FL;
				break;
			case 'c':
				af |= EXT2_COMPR_FL;
				break;
			case 'd':
				af |= EXT2_NODUMP_FL;
				break;
			case 'i':
				af |= EXT2_IMMUTABLE_FL;
				break;
			case 's':
				af |= EXT2_SECRM_FL;
				break;
			case 'u':
				af |= EXT2_UNRM_FL;
				break;
			default:
				usage ();
			}
		break;
	case '=':
		set = 1;
		for (p = &argv[*i][1]; *p; p++)
			switch (*p)
			{
			case 'A':
				sf |= EXT2_NOATIME_FL;
				break;
			case 'S':
				sf |= EXT2_SYNC_FL;
				break;
			case 'a':
				sf |= EXT2_APPEND_FL;
				break;
			case 'c':
				sf |= EXT2_COMPR_FL;
				break;
			case 'd':
				sf |= EXT2_NODUMP_FL;
				break;
			case 'i':
				sf |= EXT2_IMMUTABLE_FL;
				break;
			case 's':
				sf |= EXT2_SECRM_FL;
				break;
			case 'u':
				sf |= EXT2_UNRM_FL;
				break;
			default:
				usage ();
			}
		break;
	default:
		return EOF;
		break;
	}
	return 1;
}

static int chattr_dir_proc (const char *, struct dirent *, void *);

static void change_attributes (const char * name)
{
	unsigned long flags;
	struct stat st;

	if (lstat (name, &st) == -1) {
		com_err (program_name, errno, _("while trying to stat %s"), 
			 name);
		return;
	}
	if (S_ISLNK(st.st_mode) && recursive)
		return;

	/* Don't try to open device files, fifos etc.  We probably
           ought to display an error if the file was explicitly given
           on the command line (whether or not recursive was
           requested).  */
	if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode) &&
	    !S_ISDIR(st.st_mode))
		return;

	if (set) {
		if (verbose) {
			printf (_("Flags of %s set as "), name);
			print_flags (stdout, sf, 0);
			printf ("\n");
		}
		if (fsetflags (name, sf) == -1)
			perror (name);
	} else {
		if (fgetflags (name, &flags) == -1)
			com_err (program_name, errno,
			         _("while reading flags on %s"), name);
		else {
			if (rem)
				flags &= ~rf;
			if (add)
				flags |= af;
			if (verbose) {
				printf (_("Flags of %s set as "), name);
				print_flags (stdout, flags, 0);
				printf ("\n");
			}
			if (fsetflags (name, flags) == -1)
				com_err (program_name, errno,
				         _("while setting flags on %s"), name);
		}
	}
	if (set_version) {
		if (verbose)
			printf (_("Version of %s set as %lu\n"), name, version);
		if (fsetversion (name, version) == -1)
			com_err (program_name, errno,
			         _("while setting version on %s"), name);
	}
	if (S_ISDIR(st.st_mode) && recursive)
		iterate_on_dir (name, chattr_dir_proc, NULL);
}

static int chattr_dir_proc (const char * dir_name, struct dirent * de,
			    void * unused_private)
{
	if (strcmp (de->d_name, ".") && strcmp (de->d_name, "..")) {
	        char *path;

		path = malloc(strlen (dir_name) + 1 + strlen (de->d_name) + 1);
		if (!path)
			fatal_error(_("Couldn't allocate path variable "
				    "in chattr_dir_proc"), 1);
		sprintf (path, "%s/%s", dir_name, de->d_name);
		change_attributes (path);
		free(path);
	}
	return 0;
}

int main (int argc, char ** argv)
{
	int i, j;
	int end_arg = 0;

#ifdef ENABLE_NLS
	setlocale(LC_MESSAGES, "");
	bindtextdomain(NLS_CAT_NAME, LOCALEDIR);
	textdomain(NLS_CAT_NAME);
#endif
	if (argc && *argv)
		program_name = *argv;
	i = 1;
	while (i < argc && !end_arg) {
		if (decode_arg (&i, argc, argv) == EOF)
			end_arg = 1;
		else
			i++;
	}
	if (i >= argc)
		usage ();
	if (set && (add || rem)) {
		fprintf (stderr, _("= is incompatible with - and +\n"));
		exit (1);
	}
	if ((rf & af) != 0) {
		fprintf (stderr, "Can't both set and unset same flag.\n");
		exit (1);
	}
	if (!(add || rem || set || set_version)) {
		fprintf (stderr, _("Must use '-v', =, - or +\n"));
		exit (1);
	}
	if (verbose)
		fprintf (stderr, _("chattr %s, %s for EXT2 FS %s, %s\n"),
			 E2FSPROGS_VERSION, E2FSPROGS_DATE,
			 EXT2FS_VERSION, EXT2FS_DATE);
	for (j = i; j < argc; j++)
		change_attributes (argv[j]);
	exit(0);
}
