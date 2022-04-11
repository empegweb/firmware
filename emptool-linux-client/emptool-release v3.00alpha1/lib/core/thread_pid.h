/* thread_pid.h
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 *
 */

#ifndef THREAD_PID_H
#define THREAD_PID_H	1

class ThreadPid
{
#ifdef ECOS
    static unsigned s_isr_count, s_dsr_count;
    static bool InIsrOrDsr();
#endif
public:
#ifdef ECOS
    // Special hack^H^H^H^Hsupport for tracing inside IRQ/DSR handlers
    static void IncrementInIsr();
    static void DecrementInIsr();
    static void IncrementInDsr();
    static void DecrementInDsr();
#endif
    static unsigned Get();	// equivalent to getpid() but much faster
};

#endif
