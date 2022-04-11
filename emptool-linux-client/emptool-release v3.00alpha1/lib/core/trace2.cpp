/* trace.cpp
 *
 * Debug tracing 
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

#include "trace2.h"

static void (*cleanup_handler)();
static void do_trace(const char *tfmt, const char *file, unsigned line,
		     unsigned pid,
		     const char *fmt, va_list args);


void empeg_abort_action()
{
    tcdrain(2);
    sleep(2);
    *(int *)0 = 0;
    // justin case
    exit(1);
}

void RegisterAbortCleanUpHandler(void (*handler)())
{
    ASSERT(cleanup_handler == NULL);
    cleanup_handler = handler;
}

void do_empeg_cleanup()
{
    if (cleanup_handler != NULL)
    {
	void (*cleanup_handler2)() = cleanup_handler;
	cleanup_handler = NULL;
	cleanup_handler2();
    }
}

void ABORT()
{
    do_empeg_cleanup();
    empeg_abort_action();
}

static void do_trace(const char *tfmt, const char *file, unsigned line,
		     unsigned pid,
		     const char *fmt, va_list args)
{
    // note i'm using malloc etc
    // dnew might use trace, which would be bad...
    char *p = 0;
    int size;
    char junk[] = "";

    size = vsnprintf( junk, 1, fmt, args );
    if ( size >= 0 )
    {
	// glibc2.1 helpfully returns the size needed
	p = (char *) malloc(size + 1);
	vsnprintf( p, size + 1, fmt, args );
    }
    else
    {
	// glibc2.0 et pre, and Win32, unhelpfully return -1, so we must work
	// it out ourselves
	for(size = 128; size < 65536; size *= 2) {
	    p = (char *) malloc(size + 1);
	    int len = vsnprintf(p, size, fmt, args);
	    p[size] = 0;
	    if(len >=0)
		break;
	    free(p);
	}
	if(size == 65536) {
	    p = (char *) malloc(size + 1);
	    vsnprintf(p, size, fmt, args);
	    p[size] = 0;
	}
    }
    fprintf(stderr, tfmt, file, line, pid, p);
    free(p);
}

void empeg_trace(const char *file, unsigned line, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    do_trace("E %-14s %4d (%5d): %s", file, line, getpid(), fmt, args);
    va_end(args);
}

void empeg_tracev(const char *file, unsigned line,
		  const char *fmt, va_list args)
{
    do_trace("E %-14s %4d (%5d): %s", file, line, getpid(), fmt, args);
}

void empeg_trace_sleep(const char *file, unsigned line, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    do_trace("E %-14s %4d (%5d): %s", file, line, getpid(), fmt, args);
    va_end(args);
    usleep(200000);
}

void empeg_warn(const char *file, unsigned line, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    do_trace("E #W %-11s %4d (%5d): %s", file, line, getpid(), fmt, args);
    va_end(args);
}

void empeg_error(const char *file, unsigned line, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    do_trace("E !! %-18s %4d (%5d): %s", file, line, getpid(), fmt, args);
    va_end(args);
}

void empeg_assert(const char *file, unsigned line, void *called_by,
		  const char *assertion)
{
    fprintf(stderr,
	    "E #! %-11s %4d (%5d): Assertion failed \"%s\", "
	    "called by %p\n",
	    file, line, getpid(), assertion, called_by);
    empeg_abort_action();
}

void empeg_assert_ex(const char *file, unsigned line, void *called_by,
		     const char *assertion,
		     const char *fmt, ...)
{
    // slightly different format to trace.cpp here.
    // print the _EX part first, then the assertion that failed.
    // simpler code.
    va_list args;
    va_start(args, fmt);
    do_trace("E #! %-11s %4d (%5d): %s", file, line, getpid(), fmt, args);
    va_end(args);
    empeg_assert(file, line, called_by, assertion);
}

void empeg_trace_hex(const char *file, unsigned line, const char *header,
		     const void *void_begin, const void *void_end)
{
    const int PER_LINE = 16;
    const unsigned char *p = (const unsigned char *) void_begin;
    const unsigned char *end = (const unsigned char *) void_end;
    
    // Make sure it is roughly in bounds
    ASSERT_PTR(p);
    ASSERT_PTR(end - 1);

    empeg_trace(file, line, "%s", header);

    while (p < end)
    {
	empeg_trace(file, line, "%p: ", p);
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

	    fprintf(stderr, "%02x%c", p[i], separator);
	}
	for(int j = count; j <= PER_LINE; j++)
	    fprintf(stderr, "   ");

	for(int k = 0; k < count; k++)
	{
	    fprintf(stderr, "%c", isprint(p[k]) ? p[k] : '.');
	}
	fprintf(stderr, "\n");
	p += PER_LINE;
    }
}

#if DEBUG > 4
/* This is some rather nasty code that provides the ability to check if a *
 * pointer is valid for assertions. It is *very* slow.                    */

static jmp_buf jb;

static void badp(int bp)
{
    write(2, "!!\n", 3);
    longjmp(jb,1);
}

int IsValidPointer(const void *p)
{
    struct sigaction saMine, saSegv, saBus;
    int r = 1;
    saMine.sa_handler = badp;
    sigemptyset(&saMine.sa_mask);
    saMine.sa_flags = 0L;

    sigaction(SIGSEGV, &saMine, &saSegv);
    sigaction(SIGBUS, &saMine, &saBus);

    if (setjmp(jb))
	r = 0;
    else
	*(volatile char*)p;
    sigaction(SIGSEGV, &saSegv, NULL);
    sigaction(SIGBUS, &saBus, NULL);
    return r;
}
#endif /* DEBUG > 4 */
