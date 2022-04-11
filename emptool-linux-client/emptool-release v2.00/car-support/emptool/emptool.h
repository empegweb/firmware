/* emptool.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.4 01-Apr-2003 18:52 rob:)
 */

#ifndef EMPTOOL_H
#define EMPTOOL_H 1

#ifdef WIN32
// We don't want warnings about overlong identifier names.
#pragma warning(disable : 4786)
#define STRICT 1
#include <windows.h>

inline void sleep(int n)
{
    Sleep(n * 1000);
}

#define snprintf _snprintf
#else
#include <unistd.h>

#endif // WIN32

#endif // EMPTOOL_H
