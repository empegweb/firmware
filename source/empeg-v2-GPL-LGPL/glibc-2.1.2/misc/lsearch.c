/* Linear search functions.
   Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.

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

#include <search.h>
#include <string.h>


void *
lsearch (const void *key, void *base, size_t *nmemb, size_t size,
	 __compar_fn_t compar)
{
  void *result;

  /* Try to find it.  */
  result = lfind (key, base, nmemb, size, compar);
  if (result == NULL)
    {
      /* Not available.  Insert at the end.  */
      result = memcpy (base + (*nmemb) * size, key, size);
      ++(*nmemb);
    }

  return result;
}


void *
lfind (const void *key, const void *base, size_t *nmemb, size_t size,
       __compar_fn_t compar)
{
  const void *result = base;
  size_t cnt = 0;

  while (cnt < *nmemb && (*compar) (key, result) != 0)
    {
      result += size;
      ++cnt;
    }

  return cnt < *nmemb ? (void *) result : NULL;
}