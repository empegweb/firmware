/* Copyright (C) 1998, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1998.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef _REPERTOIREMAP_H
#define _REPERTOIREMAP_H	1

#include <stdint.h>
#include <obstack.h>

#include "simple-hash.h"
#include "linereader.h"

struct repertoire_t
{
  struct obstack mem_pool;
  hash_table char_table;
};


/* Prototypes for repertoire map handling functions.  */
extern struct repertoire_t *repertoire_read (const char *filename);

/* Find value for name in repertoire map.  */
extern uint32_t repertoire_find_value (const hash_table *repertoire,
				       const char *name, size_t len);

#endif /* repertoiremap.h */
