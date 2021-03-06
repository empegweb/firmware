/* Define current locale data for LC_CTYPE category.
   Copyright (C) 1995, 1996, 1997, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

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

#include "localeinfo.h"
#include <ctype.h>
#include <endian.h>
#include <stdint.h>

_NL_CURRENT_DEFINE (LC_CTYPE);

/* We are called after loading LC_CTYPE data to load it into
   the variables used by the ctype.h macros.

   There are three arrays of short ints which need to be indexable
   from -128 to 255 inclusive.  Stored in the locale data file are
   a copy of each for each byte order.  */

void
_nl_postload_ctype (void)
{
#if BYTE_ORDER == BIG_ENDIAN
#define bo(x) x##_EB
#elif BYTE_ORDER == LITTLE_ENDIAN
#define bo(x) x##_EL
#else
#error bizarre byte order
#endif
#define paste(a,b) paste1(a,b)
#define paste1(a,b) a##b

#define current(type,x,offset) \
  ((const type *) _NL_CURRENT (LC_CTYPE, paste(_NL_CTYPE_,x)) + offset)

  extern const unsigned int *__ctype32_b;
  extern const unsigned int *__ctype_names;
  extern const unsigned char *__ctype_width;
  extern const uint32_t *__ctype32_toupper;
  extern const uint32_t *__ctype32_tolower;

  __ctype_b = current (unsigned short int, CLASS, 128);
  __ctype_toupper = current (int, bo (TOUPPER), 128);
  __ctype_tolower = current (int, bo (TOLOWER), 128);
  __ctype32_b = current (unsigned int, CLASS32, 0);
  __ctype32_toupper = current (uint32_t, bo (TOUPPER32), 0);
  __ctype32_tolower = current (uint32_t, bo (TOLOWER32), 0);
  __ctype_names = current (unsigned int, bo (NAMES), 0);
  __ctype_width = current (unsigned char, WIDTH, 0);
}
