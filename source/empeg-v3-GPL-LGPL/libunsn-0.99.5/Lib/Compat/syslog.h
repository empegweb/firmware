/*
 * Lib/Compat/syslog.h -- system compatibility hacks -- syslog()
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

#ifndef UNSN_COMPAT_SYSLOG_H
#define UNSN_COMPAT_SYSLOG_H 1

#include <Compat/base.h>

/* system headers */

#ifdef HAVE_SYSLOG_H
# include <syslog.h>
#endif

#ifdef HAVE_SYS_SYSLOG_H
# include <sys/syslog.h>
#endif

#endif
