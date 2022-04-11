/* types.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.6 01-Apr-2003 18:52 rob:)
 */

#ifndef TYPES_H
#define TYPES_H 1

// Types of a given size:
typedef unsigned char BYTE;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef int INT32;
typedef UINT16 UTF16CHAR;

/* Types of 64-bits width:
 *
 * LONGLONGFMT is used like this (to replace, e.g. "%03d"):
 *   printf("%03" LONGLONGFMT "d",
 *		l % 1000);
 * or:
 *   printf("%09" LONGLONGFMT "u",
 *		a - b);
 */
#if defined(_MSC_VER)
 typedef __int64 INT64;
 typedef unsigned __int64 UINT64;

 #define LONGLONGFMT "I64"
 typedef __int64 LONGLONG;

 #define INT64_DEFINED 1
#endif

#if defined(__GNUC__)
 typedef long long INT64;
 typedef unsigned long long UINT64;

 #define LONGLONGFMT "L"
 typedef long long LONGLONG;

 #define INT64_DEFINED 1
#endif

#if !defined(INT64_DEFINED)
#error 64 bit data type not available on this platform
#endif

// Slightly more exotic types:
#include "guid.h"

// Types specific to Empeg
typedef UINT32 FILEID;
typedef UINT32 FID;

typedef wchar_t WCHAR;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif
