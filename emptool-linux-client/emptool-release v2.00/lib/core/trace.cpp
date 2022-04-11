/* trace.cpp
 *
 * Debug tracing 
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *
 * (:Empeg Source Release 1.31 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include "types.h"
#include "trace_impl.h"
#include "thread_pid.h"

#define TRACE_UDP	0

#ifndef MERCURY
// sorry, mercury only folks (for no particular reason)
#undef TRACE_UDP
#define TRACE_UDP	0
#endif

#if TRACE_UDP
#include "net/ipaddress.h"
#include "net/socket.h"
#include "var_string.h"
// set these to the IP:port you want
// #define TRACE_UDP_IP			"10.35.1.51"
// #define TRACE_UDP_PORT		31339
#if !defined(TRACE_UDP_IP) || !defined(TRACE_UDP_PORT)
#error Define TRACE_UDP_IP and TRACE_UDP_PORT for UDP tracing
#endif
static IPEndPoint trace_udp_ep;
static DatagramSocket *trace_sock = NULL;
#endif

extern "C"
{
    void empeg_writev(const char *prefix, const char *file, int line, const char *fmt, va_list ap);
    void empeg_write(const char *prefix, const char *file, int line, const char *fmt, ...);
};

void empeg_error(const char *file, int line, const char *fmt, ...)
{
    int err = errno;
    va_list ap;
    va_start(ap, fmt);

    empeg_writev(ERROR_PREFIX, file, line, fmt, ap);
    va_end(ap);
    errno = err;
}

unsigned long empeg_thread_identifier()
{
    return (unsigned long) ThreadPid::Get();
}

static FILE *trace_file = NULL;

inline void empeg_trace_open()
{
    if (trace_file == NULL)
    {
	const char *device = getenv("TRACE");
	if (device != NULL)
	{
	    trace_file = fopen(device, "w");
	    if (!trace_file)
		fprintf(stderr, "Failed to open trace device.\n");
	}
	if (!trace_file)
	    trace_file = stderr;
    }

#if TRACE_UDP
    if(trace_sock == NULL) {
	trace_sock = NEW DatagramSocket();
	IPEndPoint local(IPAddress::ANY, 0);
	STATUS status = trace_sock->Create(local);
	if(FAILED(status)) {
	    WARN("Failed to create socket: 0x%x\n", PrintableStatus(status));
	    return;
	}
	trace_udp_ep = IPEndPoint(IPAddress(TRACE_UDP_IP), TRACE_UDP_PORT);
    }
#endif
}

#ifdef EMPEG_ENABLE_TRACE
//static Mutex trace_mutex("Trace");
//static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void empeg_trace(const char *file, int line, const char *fmt, ...)
{
    int err = errno;
    va_list ap;
    va_start(ap, fmt);

//    empeg_trace_open();
//	pthread_mutex_lock(&mutex);
    empeg_writev(TRACE_PREFIX, file, line, fmt, ap);
//    vfprintf(trace_file, fmt, ap);
//    fflush(trace_file);
//	pthread_mutex_unlock(&mutex);
    va_end(ap);
    errno = err;
}

void empeg_tracev(const char *file, int line, const char *fmt, va_list ap)
{
    empeg_writev(TRACE_PREFIX, file, line, fmt, ap);
}

void empeg_trace_sleep(const char *file, int line, const char *fmt, ...)
{
    int err = errno;
    va_list ap;
    va_start(ap, fmt);

    empeg_writev(TRACE_PREFIX, file, line, fmt, ap);
    usleep(200000);
    errno = err;
}

void empeg_trace_hex(const char *file, int line, const char *m, const void *b, const void *e)
{
    int err = errno;
    
    const int PER_LINE = 16;
    const BYTE *p = reinterpret_cast<const BYTE *>(b);
    const BYTE *end = reinterpret_cast<const BYTE *>(e);

    // Make sure it is roughly in bounds
    ASSERT_PTR(p);
    ASSERT_PTR(end - 1);
    
    empeg_trace(file, line, m);

    while (p < end)
    {
	fprintf(trace_file, "%p: ", p);
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
	    
	    fprintf(trace_file, "%02x%c", p[i], separator);
	}
	for(int j = count; j <= PER_LINE; j++)
	    fprintf(trace_file, "   ");

	for(int k = 0; k < count; k++)
	{
	    fprintf(trace_file, "%c", isprint(p[k]) ? p[k] : '.');
	}
	fprintf(trace_file, "\n");
	p += PER_LINE;
    }
    errno = err;
}

void empeg_warn(const char *file, int line, const char *fmt, ...)
{
    int err = errno;
    va_list ap;
    va_start(ap, fmt);

    empeg_writev(WARN_PREFIX, file, line, fmt, ap);
    va_end(ap);
    errno = err;
}
#endif /* EMPEG_ENABLE_TRACE */

/** This function is available in both release and debug builds. Note
 ** that it uses a different trace format in each.
 **/    
void empeg_writev(const char *prefix, const char *file, int line, const char *fmt, va_list ap)
{
    int err = errno;
    empeg_trace_open();

#ifdef EMPEG_ENABLE_TRACE
    fprintf(stderr, "%s" DEBUG_TRACE_FORMAT, prefix, file, line, empeg_thread_identifier());
#else
    fprintf(stderr, "%s" RELEASE_TRACE_FORMAT, prefix, file, line);
#endif
    vfprintf(trace_file, fmt, ap);
    fflush(trace_file);
#if TRACE_UDP
    if(trace_sock) {
	std::string s = VarString::vPrintf(fmt, ap);
	trace_sock->SendTo(s.data(), s.size(), trace_udp_ep);
    }
#endif
    errno = err;
}

void empeg_write(const char *prefix, const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    empeg_writev(prefix, file, line, fmt, ap);
    va_end(ap);
}


void empeg_release_trace(const char *file, int line, const char *fmt, ...)
{
    int err = errno;
    va_list ap;
    va_start(ap, fmt);

    empeg_writev(TRACE_PREFIX, file, line, fmt, ap);
    
    va_end(ap);
    errno = err;
}    

#ifdef EMPEG_ENABLE_ASSERT

#if DEBUG > 4
/* This is some rather nasty code that provides the ability to check if a *
 * pointer is valid for assertions. It is *very* slow.                    */

static jmp_buf jb;

static void badp(int bp)
{
    write(2, "!!\n", 3);
    longjmp(jb,1);
}
#endif /* DEBUG > 4 */
		  
int IsValidPointer(const void *p)
{
#if DEBUG > 4
    struct sigaction saMine, saSegv, saBus;
    int r = 1;
    int err = errno;
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
    errno = err;
    return r;
#else
    /* Cheat on lower debug level */
    const void *deleted_memory = reinterpret_cast<void *>(0xdddddddd);
    const void *new_memory = reinterpret_cast<void *>(0xcccccccc);
    return (p > reinterpret_cast<void *>(0x4000)) && (p < reinterpret_cast<void *>(0xffff0000))
	&& (p != deleted_memory) && (p != new_memory);
#endif /* DEBUG > 4 */
}
#endif // EMPEG_ENABLE_ASSERT

static void (*cleanup_handler)();

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

#ifdef EMPEG_ENABLE_ASSERT

void empeg_abort()
{
    sleep(1);
    do_empeg_cleanup();
#ifdef ARCH_EMPEG
    sleep(1);
    *reinterpret_cast<int *>(0xbadbad) = 0;
#else
    abort();
#endif
}

void empeg_assert(const char *file, int line, const char *expr, void *called_by)
{
    empeg_write(ASSERT_PREFIX, file, line, "ASSERTION FAILED: %s, called by=%p\n", expr, called_by);
    ABORT();
}

void empeg_assert_fn(const char *file, int line, const char *expr, void *called_by, const char *fn)
{
    empeg_write(ASSERT_PREFIX, file, line, "ASSERTION FAILED: %s, called by=%p, function=\'%s\'\n", expr, called_by, fn);
    ABORT();
}

void empeg_assert_pre(const char *file, int line, const char *expr, void *called_by, const char *fn)
{
    empeg_write(ASSERT_PREFIX, file, line, "ASSERTION FAILED: %s, called by=%p, precond:\'%s\'\n", expr, called_by, fn);
    ABORT();
}

void empeg_assert_post(const char *file, int line, const char *expr, void *called_by, const char *fn)
{
    empeg_write(ASSERT_PREFIX, file, line, "ASSERTION FAILED: %s, called by=%p, postcond:\'%s\'\n", expr, called_by, fn);
    ABORT();
}

void empeg_assert_ex(const char *file, int line, const char *expr, void *called_by, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    empeg_write(ASSERT_PREFIX, file, line, "ASSERTION FAILED: %s, called by=%p\n", expr, called_by);
    empeg_writev(ASSERT_PREFIX, file, line, fmt, ap);
    va_end(ap);
    ABORT();
}
    
#endif // EMPEG_ENABLE_ASSERT


/** \page tracinganddebugging Tracing And Debugging Facilities

The declarations in <tt>empeg/lib/core/trace.h</tt> are at the root of
the debug facilities available on empeg products, although some people
have persuaded gdb to run on some of the devices it is often much
easier to debug using these facilities.

\subsection sub1 Tracing

Normal debug style tracing should be performed using the TRACE macro,
you should remove all uses of this macro (or convert them to TRACEC)
before checking code in. TRACE works just like printf but precedes the
output with file, line and thread information. The format string
should be terminated with a linefeed. Note that if you embed linefeeds
in your output then the prefix will not be applied to subsequent
lines. Example uses:
  - TRACE("foo=%d\n", foo);
  - TRACE("foo=%p, bar=\"%s\", result=0x%08x\n", foo, bar, PrintableStatus(result));

The previously mentioned TRACEC works similarly except it takes an
extra argument indicating the "component" to which the trace is
attached. At the moment these constants are just #defined at the top
of the appropriate source file as either zero (no tracing) or one
(tracing enabled) but we reserve the right to make this much cleverer
in the future. Example uses:
  - #define TRACE_AUDIO 1
    TRACEC(TRACE_AUDIO, "Hello\n");

 */
