/* config.h
 *
 * Central header file (sets up debugging macros etc)
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.50 13-Mar-2003 18:15 rob:)
 */

#ifndef EMPEG_CONFIG_H
#define EMPEG_CONFIG_H 1

/* Convert Windows ifndef/ifdef _DEBUG into our DEBUG=0/2 */
#ifdef WIN32
 #undef DEBUG	// undef - ATL defines it (without value) under certain circumstances
 #ifdef _DEBUG
  #define DEBUG 2
  #define DEBUG_MEMORY 1
 #else
  #define DEBUG 0
 #endif
#endif

#if DEBUG>0 || PROFILE == 2
 #define DEBUG_THREAD_NAMES 1
#else
 #define DEBUG_THREAD_NAMES 0
#endif

#if defined(__EXCEPTIONS)
 #define SUPPORT_EXCEPTIONS 1
#endif

#if defined(WIN32)
 #define SUPPORT_EXCEPTIONS 1
#endif

#ifndef SUPPORT_EXCEPTIONS
 #define SUPPORT_EXCEPTIONS 0
#endif

#ifndef WIN32
 #ifdef ARCH_EMPEG
  #define ARCH "empeg"
 #endif

 #ifdef ARCH_PC
  #define ARCH "pc"
 #endif

 #ifdef ARCH_SPARC
  #define ARCH "pc"
 #endif

 // Dogs is dogs, and cats is dogs, but this here tortoise is a hinsect
 #ifdef ARCH_MIPS
  #define ARCH "empeg"
 #endif

 #ifndef ARCH
  #error Unsupported architecture
 #endif
#endif

#ifndef UNUSED
#define UNUSED(x) do { (void)(x); } while (0)
#endif

#ifndef COUNTOF
#define COUNTOF(A) (sizeof((A)) / sizeof((A)[0]))
#endif

#define DISALLOW_COPYING(T)	\
	T(const T&);	\
	void operator=(const T&)

#ifdef WIN32
#define HAVE_REALPATH 0
#define HAVE_TIMESPEC 0
#elif defined(ECOS)
#define HAVE_REALPATH 0
#define HAVE_TIMESPEC 1
#else /* Unix */
#define HAVE_REALPATH 1
#define HAVE_TIMESPEC 1
#endif

#if !defined(WIN32)
/** Double-word; that is, twice the word size of an Intel 80286.
 *
 * @todo Portable code should not need to know what twice the word size of
 *       an Intel 80286 is.  Call it ULONG then.  Or if you need 4-bytes,
 *       UINT32 or UINT4, or something.
 */
typedef unsigned long DWORD;
#endif

#ifdef _MSC_VER
#pragma warning(disable:4786) // Debug symbol too long
#pragma warning(disable:4100) // Unreferenced formal parameter
#pragma warning(disable:4127) // Conditional expression is constant
#pragma warning(disable:4355) // 'this' used in base member initialiser list
#pragma warning(disable:4510) // Default constructor could not be generated (pardon?)
#pragma warning(disable:4511) // Copy constructor could not be generated
#pragma warning(disable:4512) // Assignment operator could not be generated
#pragma warning(disable:4663) // Archaic specialisation syntax

// This one could be problematic - the mangled name was truncated, which could lead
// to collisions.  Hopefully unbelievably rare.  Of course, now I've said that, it'll
// happen next compile.
#pragma warning(disable:4503) // Decorated name length exceeded, name was truncated
#pragma warning(disable:4290) // VC7: C++ exception specification ignored except...
#endif

#ifdef __GNUC__
#define ATTRIBUTE(X) __attribute__(X)
#else
#define ATTRIBUTE(X)
#endif

#ifdef __cplusplus
#include "dnew.h"
#endif

#endif /* CONFIG_H */
