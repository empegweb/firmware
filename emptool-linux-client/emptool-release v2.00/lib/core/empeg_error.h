/* empeg_error.h
 *
 * Some hackery to make error handling portable Win32/Unix
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.57 01-Apr-2003 18:52 rob:)
 */

#ifndef EMPEG_ERROR_H
#define EMPEG_ERROR_H 1

#if defined(WIN32)
 #ifndef _WINDOWS_
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
 #endif
 #include <winerror.h>
typedef HRESULT STATUS;
// To convert from a Win32 LONG error, use MakeWin32Status, defined below.
#endif

#ifndef CONFIG_H
#include "config.h"
#endif

#include "trace.h"
#include <string>

#define WINDOWS_FACILITY_NONE	0
#define WINDOWS_FACILITY_ITF	4
#define WINDOWS_FACILITY_WIN32	7

#define SEV_ERROR		3
#define SEV_WIN32_ERROR		2   // Win32 uses 0x8 as a prefix.  Makes error lookups easier.
#define SEV_WARNING		1
#define SEV_SUCCESS		0

#define COMPONENT_STANDARD	0
#define COMPONENT_ERRNO		1
#define COMPONENT_WMADECODER	2
#define COMPONENT_MP3DECODER	3
#define COMPONENT_PLAYER	4
#define COMPONENT_WAVDECODER	5
#define COMPONENT_HTTP		6
#define COMPONENT_CDROM		7
#define COMPONENT_TAGS		8
#define COMPONENT_RIOCOM	9

#if DEBUG>3
// If STATUS is a class (with no operator int) then we can catch dodgy
// usages of STATUS. Impressively, gcc -O6 compiles all this away and
// you end up with pretty much the same code as "typedef int STATUS".
// Mind you, gcc with no optimisation produces *reams* of code for it,
// so for normal-debug-level builds we stick with the opaque type.

class STATUS
{
    int m_status;
    explicit STATUS(int i) : m_status(i) {}
 public:
    STATUS()
	: m_status( 0 ) {}
    STATUS(int sev, int fac, int com, int res)
	: m_status( (((sev)&0x3) << 30)
		    | ((fac) << 16)
		    | (((com)&0xf) << 12)
		    | ((res)&0xfff) ) {}
    STATUS(int sev, int com, int res)
	: m_status( (((sev)&0x3) << 30)
		    | ((WINDOWS_FACILITY_ITF) << 16)
		    | (((com)&0xf) << 12)
		    | ((res)&0xfff) ) {}
    bool Succeeded() const { return m_status >= 0; }
    bool Failed() const { return m_status < 0; }
    int Severity() const { return (m_status >> 30) & 3; }
    int Facility() const { return (m_status>>16) & 0xfff; }
    int Component() const { return (m_status>>12) & 0xf; }
    int Result() const { return m_status & 0xfff; }
    int Printable() const { return m_status; }
    int ToInteger() const { return m_status; }
    static STATUS FromInteger(int i) { return STATUS(i); }

    // I'm not even sure we want these two: should "foo==S_OK" be rewritten
    // as SUCCEEDED(foo) ?
    bool operator==( const STATUS& s ) const { return s.m_status == m_status; }
    bool operator!=( const STATUS& s ) const { return s.m_status != m_status; }

#ifdef WIN32
    explicit STATUS( HRESULT hr ) : m_status(hr) {}

    //operator HRESULT() { return (HRESULT)m_status; }
#endif
};

#ifdef WIN32
#undef FAILED
#undef SUCCEEDED

inline bool FAILED(STATUS foo) { return foo.Failed(); }
inline bool FAILED(HRESULT hr) { return hr < 0; }

inline bool SUCCEEDED(STATUS foo) { return foo.Succeeded(); }
inline bool SUCCEEDED(HRESULT hr) { return hr >= 0; }

#else
#define SUCCEEDED(X) ((X).Succeeded())
#define FAILED(X) ((X).Failed())
#endif

#define MAKE_STATUS2(S,F,C,R) STATUS(S,F,C,R)
#define MAKE_STATUS(S,C,R) STATUS(S,C,R)
#define STATUS_SEVERITY(X) ((X).Severity())
#define STATUS_WINDOWS_FACILITY(X) ((X).Facility())
#define STATUS_COMPONENT(X) ((X).Component())
#define STATUS_RESULT(X) ((X).Result())
#define STATUS_TO_INTEGER(X) ((X).ToInteger())
#define STATUS_FROM_INTEGER(X) (STATUS::FromInteger(X))

inline int PrintableStatus(STATUS s)
{
    return s.Printable();
}

#else // WIN32 or DEBUG<=3

#if defined(WIN32)
typedef HRESULT STATUS;
#else
typedef struct empeg_status_ *STATUS; // struct empeg_status_ never defined
#endif

// OK, a STATUS is an int really. But nobody outside empeg_error.h is meant
// to know that. Pay no attention to the man behind the curtain.
#define MAKE_STATUS2(SEV, FAC, COM, RES) ((STATUS)((((SEV)&0x3) << 30) | ((FAC) << 16) | (((COM)&0xf) << 12) | ((RES)&0xfff)))
#define MAKE_STATUS(SEV, COM, RES) ((STATUS)((((SEV)&0x3) << 30) | ((WINDOWS_FACILITY_ITF) << 16) | (((COM)&0xf) << 12) | ((RES)&0xfff)))

#define STATUS_SEVERITY(S) (((unsigned int)(S))>>30)
#define STATUS_WINDOWS_FACILITY(S) ((((int)(S))>>16)&0xfff)
#define STATUS_COMPONENT(S) ((((int)(S))>>12)&0xf)
#define STATUS_RESULT(S) (((int)(S))&0xfff)
#define STATUS_TO_INTEGER(S) ((int)(S))
#define STATUS_FROM_INTEGER(I) ((STATUS)(I))

#ifndef WIN32
#define SUCCEEDED(X) (((int)(X))>=0)
#define FAILED(X) (((int)(X))<0)
#endif

inline int PrintableStatus(STATUS s)
{
    return (int)s;
}

inline int StatusHack(STATUS s)
{
    return (int)s;
}

#endif

/* Error code #defines are built from the corresponding .mes file.
 * Add your errors there, please.
 */
#include "core_errors.h"

#if !defined(WIN32)
 #define S_OK MAKE_STATUS2(SEV_SUCCESS, WINDOWS_FACILITY_NONE, COMPONENT_STANDARD, 0)
 #define S_FALSE MAKE_STATUS2(SEV_SUCCESS, WINDOWS_FACILITY_NONE, COMPONENT_STANDARD, 1)
#endif /* !WIN32 */

#include <errno.h>

inline STATUS MakeErrnoStatus()
{
#ifdef ASSERT
    ASSERT(errno != 0);
#endif
    return MAKE_STATUS(SEV_ERROR, COMPONENT_ERRNO, errno);
}

inline STATUS MakeErrnoStatus(int e)
{
#ifdef ASSERT
     ASSERT(e > 0);
     ASSERT(e < 0x10000);
#endif
     return MAKE_STATUS(SEV_ERROR, COMPONENT_ERRNO, e);
}

// WinError.h has the following:
// #define HRESULT_FROM_WIN32(x)
//      ((HRESULT)(x) <= 0
//			? ((HRESULT)(x))
//			: ((HRESULT) (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000)))
inline STATUS MakeWin32Status(DWORD dw)
{
#if defined(WIN32)
    return HRESULT_FROM_WIN32(dw);
#else
    return MAKE_STATUS2(SEV_WIN32_ERROR, WINDOWS_FACILITY_WIN32, COMPONENT_STANDARD, dw);
#endif
}

/** If the error cannot be looked up, returns blank string. */
std::string FormatErrorMessage(STATUS status);

/** If the error cannot be looked up, returns Format("Error 0x%8x", status) */
std::string FormatErrorMessageEx(STATUS status);

#endif /* EMPEG_ERROR_H */
