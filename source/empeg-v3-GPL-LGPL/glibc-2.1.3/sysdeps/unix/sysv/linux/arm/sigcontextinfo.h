/* Copyright (C) 1999 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Philip Blundell <philb@gnu.org>, 1999.

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

#include <bits/armsigctx.h>

#define SIGCONTEXT int _a2, int _a3, int _a4, union k_sigcontext
#define SIGCONTEXT_EXTRA_ARGS _a2, _a3, _a4,

#define GET_PC(ctx)	((void *)((ctx.v20.magic == SIGCONTEXT_2_0_MAGIC) ? \
			 ctx.v20.reg.ARM_pc : ctx.v21.arm_pc))
#define GET_FRAME(ctx)	\
	ADVANCE_STACK_FRAME((void *)((ctx.v20.magic == SIGCONTEXT_2_0_MAGIC) ? \
			 ctx.v20.reg.ARM_fp : ctx.v21.arm_fp))
#define GET_STACK(ctx)	((void *)((ctx.v20.magic == SIGCONTEXT_2_0_MAGIC) ? \
			 ctx.v20.reg.ARM_sp : ctx.v21.arm_sp))
#define ADVANCE_STACK_FRAME(frm)	\
			((struct layout *)frm - 1)
