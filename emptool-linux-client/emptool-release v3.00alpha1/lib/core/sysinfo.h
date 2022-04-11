/* sysinfo.h
 *
 * Interface to the sysinfo system call
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see
 * file COPYING), unless you possess an alternative written licence
 * from empeg ltd.
 *
 * (:Empeg Source Release 1.4 13-Mar-2003 18:15 rob:)
 */

#ifndef SYSINFO_H
#define SYSINFO_H

#ifndef ECOS
#include <sys/syscall.h>

#if defined(__linux__) && !defined(__sparc__)
#define HAVE_SYSINFO 1
inline _syscall1(int, sysinfo, struct sysinfo*, info);
#else
#define HAVE_SYSINFO 0
#endif
#endif

#endif // defined(SYSINFO_H)
