/* empeg_stat.h
 *
 * Platform independent stat(2) support
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 */

#ifndef EMPEG_STAT_H
#define EMPEG_STAT_H 1

#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct _stat empeg_stat_struct;

inline int empeg_stat(const TCHAR *file_name, empeg_stat_struct *buf)
{
#if defined(UNICODE)
    return _wstat(file_name, buf);
#else
    return _stat(file_name, buf);
#endif
}

#else // !defined(WIN32)

// Hope that it is UNIX like
#include <unistd.h>

typedef struct stat empeg_stat_struct;

inline int empeg_stat(const char *file_name, empeg_stat_struct *buf)
{
    return stat(file_name, buf);
}
#endif // !defined(WIN32)

#endif // EMPEG_STAT_H
