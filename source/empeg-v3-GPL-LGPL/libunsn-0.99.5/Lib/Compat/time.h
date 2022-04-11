/*
 * Lib/Compat/time.h -- system compatibility hacks -- time definitions
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

#ifndef UNSN_COMPAT_TIME_H
#define UNSN_COMPAT_TIME_H 1

#include <Compat/base.h>

/* system headers */

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
#endif

#include <time.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#endif
