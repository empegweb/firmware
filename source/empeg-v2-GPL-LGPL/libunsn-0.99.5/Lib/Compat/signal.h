/*
 * Lib/Compat/signal.h -- system compatibility hacks -- signal handling
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

#ifndef UNSN_COMPAT_SIGNAL_H
#define UNSN_COMPAT_SIGNAL_H 1

#include <Compat/base.h>

/* system headers */

#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif

/* these flags are essentially optional */

#ifndef SA_NOCLDSTOP
# define SA_NOCLDSTOP 0
#endif

#ifndef SA_RESTART
# define SA_RESTART 0
#endif

#endif
