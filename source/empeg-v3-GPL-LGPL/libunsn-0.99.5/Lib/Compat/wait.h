/*
 * Lib/Compat/wait.h -- system compatibility hacks -- wait()
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

#ifndef UNSN_COMPAT_WAIT_H
#define UNSN_COMPAT_WAIT_H 1

#include <Compat/base.h>

/* system headers */

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

/* Prototypes. */

#if defined(HAVE_WAIT3) && !defined(HAVE_PROTOTYPE_WAIT3)
struct rusage;
pid_t wait3(int *, int, struct rusage *);
#endif

#if defined(HAVE_WAIT4) && !defined(HAVE_PROTOTYPE_WAIT4)
struct rusage;
pid_t wait4(pid_t, int *, int, struct rusage *);
#endif

#if defined(HAVE_WAITPID) && !defined(HAVE_PROTOTYPE_WAITPID)
pid_t waitpid(pid_t, int *, int);
#endif

/* The various types of wait. */

#if !defined(HAVE_WAITPID) && defined(HAVE_WAIT4)
# define waitpid(PID, STATUSP, FLAGS) wait4(PID, STATUSP, FLAGS, NULL)
#endif

#if defined(HAVE_WAITPID) || defined(waitpid)
# define wait_specific_flags(PID, STATUSP, FLAGS) waitpid(PID, STATUSP, FLAGS)
# define wait_any_flags(STATUSP, FLAGS) waitpid(-1, STATUSP, FLAGS)
# define wait_specific(PID, STATUSP) waitpid(PID, STATUSP, 0)
# define wait_any(STATUSP) waitpid(-1, STATUSP, 0)
#elif defined(HAVE_WAIT3)
# define wait_any_flags(STATUSP, FLAGS) wait3(STATUSP, FLAGS, NULL)
# define wait_any(STATUSP) wait3(STATUSP, 0, NULL)
#else
# define wait_any(STATUSP) wait(STATUSP)
#endif

#endif
