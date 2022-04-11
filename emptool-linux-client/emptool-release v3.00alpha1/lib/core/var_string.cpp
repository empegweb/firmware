/* var_string.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.15 13-Mar-2003 18:15 rob:)
 */

#include <stdio.h>
#include <stdarg.h>
#include <string>

#include "config.h"
#include "trace.h"
#include "var_string.h"

#ifdef WIN32
#define empeg_vsnprintf _vsnprintf
#define empeg_vsnwprintf _vsnwprintf
#else
#define empeg_vsnprintf vsnprintf
#define empeg_vsnwprintf vswprintf  // glibc "helpfully" takes a count, but drops the 'n'.  Bastards.
#endif

#ifndef ECOS
std::string VarString::vPrintf(const char *fmt, va_list ap)
{
    char *p = 0;
    int size;
    char junk[] = "";

    size = empeg_vsnprintf( junk, 1, fmt, ap );
    if ( size >= 0 )
    {
	// glibc2.1 helpfully returns the size needed
	p = NEW char[size+1];
	empeg_vsnprintf( p, size+1, fmt, ap );
    }
    else
    {
	// glibc2.0 et pre, and Win32, unhelpfully return -1, so we must work
	// it out ourselves
	for(size = 128; size < 65536; size *= 2) {
	    p = NEW char[size+1];
	    int len = empeg_vsnprintf(p, size, fmt, ap);
	    p[size] = 0;
	    if(len >=0) break;
	    delete[] p;
	}
	if(size == 65536) {
	    p = NEW char[size+1];
	    empeg_vsnprintf(p, size, fmt, ap);
	    p[size] = 0;
	}
    }
    std::string result = p;
    delete[] p;
    return result;
}
#else
std::string VarString::vPrintf(const char *fmt, va_list ap)
{
    // F&*($*& S£(*& !!!    
    char *p = 0;
    int size = 32;
    int last_ret = -1;

    for(;;)
    {
	p = NEW char[size + 1];
	int ret = empeg_vsnprintf(p, size, fmt, ap);
	if(ret == last_ret)
	    break;
	delete[] p;
	last_ret = ret;
	size *= 2;
    }
    std::string result = p;
    delete[] p;
    return result;
}
#endif

#if defined(WIN32)
// Unfortunately our version of glibc on aphex doesn't support
// the vswprintf function so we can't compile this for Unix or
// probably eCos.
std::wstring VarString::WvPrintf(const wchar_t *fmt, va_list ap)
{
    wchar_t *p = 0;
    int size;
    wchar_t junk[] = L"";

    size = empeg_vsnwprintf( junk, 1, fmt, ap );
    if ( size >= 0 )
    {
	// glibc2.1 helpfully returns the size needed
	p = NEW wchar_t[size+1];
	empeg_vsnwprintf( p, size+1, fmt, ap );
    }
    else
    {
	// glibc2.0 et pre, and Win32, unhelpfully return -1, so we must work
	// it out ourselves
	for(size = 128; size < 65536; size *= 2) {
	    p = NEW wchar_t[size+1];
	    int len = empeg_vsnwprintf(p, size, fmt, ap);
	    p[size] = 0;
	    if(len >=0) break;
	    delete[] p;
	}
	if(size == 65536) {
	    p = NEW wchar_t[size+1];
	    empeg_vsnwprintf(p, size, fmt, ap);
	    p[size] = 0;
	}
    }
    std::wstring result = p;
    delete[] p;
    return result;
}

std::wstring VarString::WPrintf(const wchar_t *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    std::wstring result = WvPrintf(fmt, ap);
    va_end(ap);
    return result;
}
#endif // WIN32

std::string VarString::Printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    std::string result = vPrintf(fmt, ap);
    va_end(ap);
    return result;
}

#if defined(TEST)
int main(void)
{
#if 0
    printf("%s", (VarString::T() << "hi " << 3 << L"there\n").c_str());
#endif
    return 0;
}
#endif
