/*
 * $Header: /tramp/usr/src/CVS-REPO/e2fsprogs/lib/et/init_et.c,v 1.15 1999/10/23 01:16:06 tytso Exp $
 * $Source: /tramp/usr/src/CVS-REPO/e2fsprogs/lib/et/init_et.c,v $
 * $Locker:  $
 *
 * Copyright 1986, 1987, 1988 by MIT Information Systems and
 *	the MIT Student Information Processing Board.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose is hereby granted, provided that
 * the names of M.I.T. and the M.I.T. S.I.P.B. not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  M.I.T. and the
 * M.I.T. S.I.P.B. make no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#include <stdio.h>
#include <errno.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include "com_err.h"
#include "error_table.h"

#ifndef __STDC__
#define const
#endif

struct foobar {
    struct et_list etl;
    struct error_table et;
};

extern struct et_list * _et_list;

#ifdef __STDC__
int init_error_table(const char * const *msgs, int base, int count)
#else
int init_error_table(msgs, base, count)
    const char * const * msgs;
    int base;
    int count;
#endif
{
    struct foobar * new_et;

    if (!base || !count || !msgs)
	return 0;

    new_et = (struct foobar *) malloc(sizeof(struct foobar));
    if (!new_et)
	return ENOMEM;	/* oops */
    new_et->etl.table = &new_et->et;
    new_et->et.msgs = msgs;
    new_et->et.base = base;
    new_et->et.n_msgs= count;

    new_et->etl.next = _et_list;
    _et_list = &new_et->etl;
    return 0;
}
