/* Copyright (C) 1999, 2000 Free Software Foundation, Inc.
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

#if !defined _MATH_H && !defined _COMPLEX_H
# error "Never use <bits/mathdef.h> directly; include <math.h> instead"
#endif

#if defined  __USE_ISOC9X && defined _MATH_H && !defined _MATH_H_MATHDEF
# define _MATH_H_MATHDEF	1

/* GCC does not promote `float' values to `double'.  */
typedef float float_t;		/* `float' expressions are evaluated as
				   `float'.  */
typedef double double_t;	/* `double' expressions are evaluated as
				   `double'.  */

/* Signal that types stay as they were declared.  */
# define FLT_EVAL_METHOD	0

/* Define `INFINITY' as value of type `float'.  */
# define INFINITY	HUGE_VALF


/* The values returned by `ilogb' for 0 and NaN respectively.  */
# define FP_ILOGB0	0x80000001
# define FP_ILOGBNAN	0x7fffffff

/* Number of decimal digits for the `double' type.  */
# define DECIMAL_DIG	15

#endif	/* ISO C99 */

#ifndef __NO_LONG_DOUBLE_MATH
/* Signal that we do not really have a `long double'.  This disables the
   declaration of all the `long double' function variants.  */
/* XXX The FPA does support this but the patterns in GCC are currently
   turned off.  */
# define __NO_LONG_DOUBLE_MATH	1
#endif
