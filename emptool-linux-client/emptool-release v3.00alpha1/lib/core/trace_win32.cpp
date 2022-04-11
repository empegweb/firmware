/* trace_win32.cpp
 *
 * Windows implementation of empeg_trace
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.20 13-Mar-2003 18:15 rob:)
 */

#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "trace.h"
#include "var_string.h"
#include <stdarg.h>
#include "trace_impl.h"
#include <time.h>
#include "trace_hook.h"

static FILE *g_tracefile = NULL;
static TraceHook *g_tracehook = NULL;

extern "C" void empeg_longtimestamp(char *time_stamp);

extern "C" void empeg_setlogfile(const char *filename, int append)
{
    if (g_tracefile)
	fclose(g_tracefile);

    if (filename)
	g_tracefile = fopen(filename, append ? "a" : "w");

    if (g_tracefile)
    {
	char time_stamp[31];
	empeg_longtimestamp(time_stamp);

	fprintf(g_tracefile, "Logfile '%s' opened %s\n", filename, time_stamp);
    }
}

extern "C" void empeg_settracehook(TraceHook *hook)
{
    ASSERT(g_tracehook == NULL);
    g_tracehook = hook;
}

extern "C" void empeg_unsettracehook(TraceHook *hook)
{
    ASSERT(g_tracehook == hook);
    g_tracehook = NULL;
}

/** time_stamp must be at least 30 characters long, to allow room for the null terminator.
 **/
extern "C" void empeg_longtimestamp(char *time_stamp)
{
    static const char * const dayName[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
    };

    static const char * const monthName[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    };

    time_t now = time(NULL);
    struct tm *ptm = gmtime(&now);

    _snprintf(time_stamp, 30, "%-3s %02d %-3s %04d %02d:%02d:%02d GMT ",
	dayName[ptm->tm_wday], ptm->tm_mday, monthName[ptm->tm_mon], ptm->tm_year+1900,
	ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
}

extern "C" void empeg_write_trace(const char *text)
{
    DWORD last_error = GetLastError();

    OutputDebugStringA(text);
    fputs(text, stderr);

    if (g_tracefile)
    {
//    	fprintf(g_tracefile, "%d %s", GetTickCount(), text);
	fputs(text, g_tracefile);
        fflush(g_tracefile);
    }

    if (g_tracehook)
	g_tracehook->Put(text);

    SetLastError(last_error);
}

extern "C" void empeg_trace(const char *file, int line, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    empeg_tracev(file, line, fmt, va);
    va_end( va );
}

extern "C" void empeg_tracev(const char *file, int line, const char *fmt, va_list va)
{
#ifdef EMPEG_ENABLE_TRACE
    std::string s(VarString::Printf(DEBUG_TRACE_FORMAT, file, line, GetCurrentThreadId()));
#else
    std::string s(VarString::Printf(RELEASE_TRACE_FORMAT, file, line));
#endif

    s += VarString::vPrintf(fmt, va);

    empeg_write_trace(s.c_str());
}

extern "C" void empeg_release_tracev(const char *file, int line, const char *fmt, va_list va)
{
    empeg_tracev(file, line, fmt, va);
}

#if DEBUG>0
extern "C" void empeg_abort()
{
    _CrtDbgBreak();
}
#endif // DEBUG>0

void empeg_trace_hex(const char *file, int line, const char *m, const void *b, const void *e)
{
    int err = errno;
    
    const int PER_LINE = 16;
    const BYTE *p = reinterpret_cast<const BYTE *>(b);
    const BYTE *end = reinterpret_cast<const BYTE *>(e);

    // Make sure it is roughly in bounds
    ASSERT_PTR(p);
    ASSERT_PTR(end - 1);
    
#ifdef EMPEG_ENABLE_TRACE
    std::string s(VarString::Printf(DEBUG_TRACE_FORMAT, file, line, GetCurrentThreadId()));
#else
    std::string s(VarString::Printf(RELEASE_TRACE_FORMAT, file, line));
#endif

    s += m;

    empeg_write_trace(s.c_str());

    //empeg_trace(file, line, m);

    while (p < end)
    {
        std::string s(VarString::Printf("%p: ", p));
	//fprintf(trace_file, "%p: ", p);
	int count = end - p;
	if (count > PER_LINE)
	    count = PER_LINE;
	for(int i = 0; i < count; i++)
	{
	    char separator;
	    if (i == 7)
		separator = '|';
	    else if (i == 3 || i == 11)
		separator = '.';
	    else
		separator = ' ';
	    
            s += VarString::Printf("%02x%c", p[i], separator);
	    //fprintf(trace_file, "%02x%c", p[i], separator);
	}
	for(int j = count; j <= PER_LINE; j++)
        {
            s += "   ";
	    //fprintf(trace_file, "   ");
        }

	for(int k = 0; k < count; k++)
	{
            if (isprint(p[k]))
                s += p[k];
            else
                s += ".";
            
	    //fprintf(trace_file, "%c", isprint(p[k]) ? p[k] : '.');
	}
	//fprintf(trace_file, "\n");
        s += "\n";
        empeg_write_trace(s.c_str());

	p += PER_LINE;
    }
    errno = err;
}

extern "C" int IsValidPointer(const void *p)
{
    return !IsBadReadPtr(p, 1);
}
