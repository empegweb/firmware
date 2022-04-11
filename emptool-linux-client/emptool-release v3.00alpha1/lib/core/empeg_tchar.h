/* empeg_tchar.h
 *
 * Nasty stuff to allow compiling in Unicode and Latin1.
 *
 * (C) 2002 SONICblue Inc
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.9 13-Mar-2003 18:15 rob:)
 */

#ifndef EMPEG_TCHAR_H
#define EMPEG_TCHAR_H 1

#ifdef ECOS
#include "empeg_wchar.h"
#else
#include <wchar.h>
#endif

#ifdef __cplusplus
#include <string>
#if defined(WIN32)
#ifdef _UNICODE
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif
#else  // !WIN32
typedef std::string tstring;
#endif // WIN32
#endif /* __cplusplus */

#if defined(WIN32)
#include <tchar.h>
#define empeg_tprintf _tprintf
#define empeg_tcschr _tcschr
#define empeg_ttoi _ttoi
#define empeg_tgetenv _tgetenv
#else
#define _T(x) x
#define empeg_tprintf printf
#define empeg_tcschr strchr
#define empeg_ttoi atoi
#define empeg_tgetenv getenv
typedef char TCHAR;
#endif

#if defined(UNICODE)
#define empeg_fopen _wfopen
#else
#define empeg_fopen fopen
#endif

#if defined(UNICODE)
#define empeg_open _wopen
#else
#define empeg_open open
#endif

#endif /* EMPEG_TCHAR_H */
