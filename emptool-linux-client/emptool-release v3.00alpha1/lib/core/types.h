/* types.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.19 13-Mar-2003 18:15 rob:)
 */

#ifndef TYPES_H
#define TYPES_H 1

#if defined(__cplusplus)
#include <string>
#endif

#include "empeg_tchar.h"
#include <limits.h>

// Types of a given size:
typedef unsigned char BYTE;
typedef unsigned short UINT16;

#if !defined(UINT32_DEFINED)
typedef unsigned int UINT32;

#define UINT32_DEFINED
#endif

#if !defined(INT32_DEFINED)
typedef int INT32;

#define INT32_DEFINED
#endif

#if WCHAR_MAX > 65535
typedef wchar_t UCS4CHAR;
#else
typedef UINT32 UCS4CHAR;
#endif
typedef UINT16 UTF16CHAR;
typedef char UTF8CHAR;

/* Types of 64-bits width:
 *
 * LONGLONGFMT is used like this (to replace, e.g. "%03d"):
 *   printf("%03" LONGLONGFMT "d",
 *		l % 1000);
 * or:
 *   printf("%09" LONGLONGFMT "u",
 *		a - b);
 * or (ick):
 *   s = VarString::TPrintf(_T("%") _T(LONGLONGFMT) _T("d"), a - b);
 *
 * LONGLONGLITERAL is used where you want to write a 64-bit
 * literal. I.e. where you would normally write:
 *
 *   const LONGLONG l = 1024LL;
 * or:
 *   const LONGLONG l = 1024i64;
 *
 * you can now just write:
 *   const LONGLONG l = LONGLONGLITERAL(1024);
 */
#if defined(_MSC_VER)
 typedef __int64 INT64;
 typedef unsigned __int64 UINT64;

 #define LONGLONGFMT "I64"
 typedef __int64 LONGLONG;
 #define LONGLONGLITERAL(x) (x ## i64)
 #define UINT64LITERAL(x) (x ## ui64)

 #define INT64_DEFINED 1
#endif

#if defined(__GNUC__)
 typedef long long INT64;
 typedef unsigned long long UINT64;

 #define LONGLONGFMT "L"
 typedef long long LONGLONG;

 #define LONGLONGLITERAL(x) (x ## LL)
 #define UINT64LITERAL(x) (x ## ULL)

 #define INT64_DEFINED 1
#endif

#if defined(__cplusplus)
class Int64 {
    INT64 m_value;

public:
    Int64(INT64 value)
	: m_value(value) { }

    INT32 GetHigh() const { return (INT32)(m_value >> 32); }
    UINT32 GetLow() const { return (UINT32)(m_value & 0xFFFFFFFF); }
};

class UInt64 {
    UINT64 m_value;

public:
    UInt64(UINT64 value)
	: m_value(value) { }

    UINT32 GetHigh() const { return (UINT32)(m_value >> 32); }
    UINT32 GetLow() const { return (UINT32)(m_value & 0xFFFFFFFF); }
};
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

#if defined(__cplusplus)
typedef std::basic_string<UCS4CHAR> ucs4_string;
typedef std::basic_string<UTF16CHAR> utf16_string;
typedef std::basic_string<UTF8CHAR> utf8_string;
#endif // __cplusplus

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif
