/* trace_win32.cpp
 *
 * Windows implementation of empeg_trace
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.6 01-Apr-2003 18:52 rob:)
 */

#include "windows.h"
#include "trace.h"
#include "var_string.h"
#include <stdarg.h>
#include "trace_impl.h"

extern "C" void empeg_trace(const char *file, int line, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);

#ifdef EMPEG_ENABLE_TRACE
    std::string s(VarString::Printf(DEBUG_TRACE_FORMAT, file, line, GetCurrentThreadId()));
#else
    std::string s(VarString::Printf(RELEASE_TRACE_FORMAT, file, line));
#endif

    s += VarString::vPrintf(fmt, va);

    OutputDebugString(s.c_str());

    va_end( va );
}

extern "C" void empeg_abort_action()
{
    abort();
}
