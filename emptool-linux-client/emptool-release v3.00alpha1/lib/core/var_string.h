/* var_string.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.10 13-Mar-2003 18:15 rob:)
 */

#ifndef VAR_STRING_H
#define VAR_STRING_H

#include <stdarg.h>	// for va_list
#include <string>	// for std::string
#include "config.h"
#include "empeg_tchar.h"

namespace VarString
{
    std::string vPrintf(const char *fmt, va_list ap);
    std::wstring WvPrintf(const wchar_t *fmt, va_list ap);
    std::string Printf(const char *fmt, ...) ATTRIBUTE((__format__(printf, 1, 2)));
    std::wstring WPrintf(const wchar_t *fmt, ...) /*ATTRIBUTE((__format__(wprintf, 1, 2)))*/;

#if defined(UNICODE)
    /** @todo Does __format__(wprintf... actually work? */
    inline tstring TPrintf(const wchar_t *fmt, ...) /*ATTRIBUTE((__format__(wprintf, 1, 2)))*/
    {
	va_list ap;
	va_start(ap, fmt);
	tstring result = WvPrintf(fmt, ap);
	va_end(ap);
	return result;
    }
#else
    tstring TPrintf(const char *fmt, ...) ATTRIBUTE((__format__(printf, 1, 2)));

    inline tstring TPrintf(const char *fmt, ...)
    {
	va_list ap;
	va_start(ap, fmt);
	tstring result = vPrintf(fmt, ap);
	va_end(ap);
	return result;
    }
#endif

#if 0
    class T
    {
	tstring m_buffer;

    public:
	T();
	T& operator<<(int);
	T& operator<<(std::string);
	T& operator<<(std::wstring);

	operator tstring() { return m_buffer; }
	const TCHAR *c_str() { return m_buffer.c_str(); }
    };
#endif

};

#endif
