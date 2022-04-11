/* empeg_error.h
 *
 * Some hackery to make error handling portable Win32/Unix
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.60 13-Mar-2003 18:15 rob:)
 */

#ifndef EMPEG_ERROR_H
#define EMPEG_ERROR_H 1

#include "trace.h"
#include <string>
#include "empeg_tchar.h"

#ifndef EMPEG_STATUS_H
#include "empeg_status.h"
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
#ifdef WIN32
    return MAKE_STATUS(SEV_WIN32_ERROR, COMPONENT_ERRNO, errno);
#else
    return MAKE_STATUS(SEV_ERROR, COMPONENT_ERRNO, errno);
#endif
}

inline STATUS MakeErrnoStatus(int e)
{
#ifdef ASSERT
     ASSERT(e > 0);
     ASSERT(e < 0x10000);
#endif
#ifdef WIN32
     // Distinguish Win32 errnos from player-side Linux ones
     return MAKE_STATUS(SEV_WIN32_ERROR, COMPONENT_ERRNO, e);
#else
     return MAKE_STATUS(SEV_ERROR, COMPONENT_ERRNO, e);
#endif
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
tstring FormatErrorMessage(STATUS status);

/** If the error cannot be looked up, returns Format("Error 0x%8x", status) */
tstring FormatErrorMessageEx(STATUS status);

#endif /* EMPEG_ERROR_H */
