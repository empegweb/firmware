/* Copyright (C) 1996, 1997, 1998, 1999, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.org>, 1996.

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

#include <errno.h>
#include <gconv.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wcsmbsload.h>

#include <assert.h>

#ifndef EILSEQ
# define EILSEQ EINVAL
#endif


/* This is the private state used if PS is NULL.  */
static mbstate_t state;

size_t
__mbsrtowcs (dst, src, len, ps)
     wchar_t *dst;
     const char **src;
     size_t len;
     mbstate_t *ps;
{
  struct gconv_step_data data;
  size_t result;
  int status;

  /* Tell where we want the result.  */
  data.invocation_counter = 0;
  data.internal_use = 1;
  data.is_last = 1;
  data.statep = ps ?: &state;

  /* Make sure we use the correct function.  */
  update_conversion_ptrs ();

  /* We have to handle DST == NULL special.  */
  if (dst == NULL)
    {
      wchar_t buf[64];		/* Just an arbitrary size.  */
      const unsigned char *inbuf = (const unsigned char *) *src;
      const unsigned char *srcend = inbuf + strlen (inbuf) + 1;

      result = 0;
      data.outbufend = (char *) buf + sizeof (buf);
      do
	{
	  data.outbuf = (char *) buf;

	  status = (*__wcsmbs_gconv_fcts.towc->fct) (__wcsmbs_gconv_fcts.towc,
						     &data, &inbuf, srcend,
						     &result, 0);

	  result += (wchar_t *) data.outbuf - buf;
	}
      while (status == GCONV_FULL_OUTPUT);

      if (status == GCONV_OK || status == GCONV_EMPTY_INPUT)
	{
	  /* There better should be a NUL wide char at the end.  */
	  assert (((wchar_t *) data.outbuf)[-1] == L'\0');
	  /* Don't count the NUL character in.  */
	  --result;
	}
    }
  else
    {
      /* This code is based on the safe assumption that all internal
	 multi-byte encodings use the NUL byte only to mark the end
	 of the string.  */
      const unsigned char *srcend;

      srcend = (const unsigned char *) (*src
					+ __strnlen (*src, len * MB_CUR_MAX)
					+ 1);

      data.outbuf = (unsigned char *) dst;
      data.outbufend = data.outbuf + len * sizeof (wchar_t);

      status = (*__wcsmbs_gconv_fcts.towc->fct) (__wcsmbs_gconv_fcts.towc,
						 &data,
						 (const unsigned char **) src,
						 srcend, &result, 0);

      result = (wchar_t *) data.outbuf - dst;

      /* We have to determine whether the last character converted
	 is the NUL character.  */
      if ((status == GCONV_OK || status == GCONV_EMPTY_INPUT)
	  && ((wchar_t *) dst)[result - 1] == L'\0')
	{
	  assert (result > 0);
	  assert (__mbsinit (data.statep));
	  *src = NULL;
	  --result;
	}
    }

  /* There must not be any problems with the conversion but illegal input
     characters.  */
  assert (status == GCONV_OK || status == GCONV_EMPTY_INPUT
	  || status == GCONV_ILLEGAL_INPUT
	  || status == GCONV_INCOMPLETE_INPUT || status == GCONV_FULL_OUTPUT);

  if (status != GCONV_OK && status != GCONV_FULL_OUTPUT
      && status != GCONV_EMPTY_INPUT)
    {
      result = (size_t) -1;
      __set_errno (EILSEQ);
    }

  return result;
}
weak_alias (__mbsrtowcs, mbsrtowcs)
