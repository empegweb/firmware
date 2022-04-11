/* var_string.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.9 01-Apr-2003 18:52 rob:)
 */

#include <stdio.h>
#include <stdarg.h>
#include <string>

#include "config.h"
#include "trace.h"
#include "var_string.h"

#ifdef WIN32
#define vsnprintf _vsnprintf
#endif

std::string VarString::vPrintf(const char *fmt, va_list ap)
{
    char *p = 0;
    int size;
    char junk[] = "";

    size = vsnprintf( junk, 1, fmt, ap );
    if ( size >= 0 )
    {
	// glibc2.1 helpfully returns the size needed
	p = NEW char[size+1];
	vsnprintf( p, size+1, fmt, ap );
    }
    else
    {
	// glibc2.0 et pre, and Win32, unhelpfully return -1, so we must work
	// it out ourselves
	for(size = 128; size < 65536; size *= 2) {
	    p = NEW char[size+1];
	    int len = vsnprintf(p, size, fmt, ap);
	    p[size] = 0;
	    if(len >=0) break;
	    delete[] p;
	}
	if(size == 65536) {
	    p = NEW char[size+1];
	    vsnprintf(p, size, fmt, ap);
	    p[size] = 0;
	}
    }
    std::string result = p;
    delete[] p;
    return result;
}

std::string VarString::Printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    std::string result = vPrintf(fmt, ap);
    va_end(ap);
    return result;
}
