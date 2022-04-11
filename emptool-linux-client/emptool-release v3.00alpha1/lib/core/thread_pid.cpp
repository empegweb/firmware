/* thread_pid.cpp
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.10 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "thread_pid.h"
#include <pthread.h>

#ifndef ECOS

# include <unistd.h>

static unsigned get_pid_function()
{
    return getpid();
}

#else // ECOS

# include <cyg/hal/drv_api.h>
# include "mutex.h"

unsigned ThreadPid::s_isr_count = 0;
unsigned ThreadPid::s_dsr_count = 0;

void ThreadPid::IncrementInIsr()
{
    cyg_drv_isr_lock();
    ++s_isr_count;
    cyg_drv_isr_unlock();
}

void ThreadPid::DecrementInIsr()
{
    cyg_drv_isr_lock();
    --s_isr_count;
    cyg_drv_isr_unlock();
}

void ThreadPid::IncrementInDsr()
{
    cyg_drv_isr_lock();
    ++s_dsr_count;
    cyg_drv_isr_unlock();
}

void ThreadPid::DecrementInDsr()
{
    cyg_drv_isr_lock();
    --s_dsr_count;
    cyg_drv_isr_unlock();
}

bool ThreadPid::InIsrOrDsr()
{
    return s_isr_count > 0 || s_dsr_count > 0;
}

#endif

#ifndef ECOS
static pthread_key_t thread_id_key;

static void key_create_function()
{
    pthread_key_create(&thread_id_key, NULL);
}
#endif

unsigned ThreadPid::Get()
{
#ifdef ECOS
    if(InIsrOrDsr())
    {
	// IRQ/DSR handlers upset pthreads. Kernel instrumentation
	// returns this for interrupt tasks
	return 0x0fff;
    }
    else
	return (unsigned) Cyg_Thread::self()->get_unique_id();
#else
    static pthread_once_t key_create_once = PTHREAD_ONCE_INIT;
    // Initialise the key (if not already initialised - this is atomic)
    pthread_once(&key_create_once, key_create_function);
    // See if we've already set it
    unsigned pid = (unsigned) pthread_getspecific(thread_id_key);
    if(pid)
	return pid;
    // Haven't grabbed it yet
    pid = get_pid_function();
    pthread_setspecific(thread_id_key, (const void *) pid);
    return pid;
#endif
}

#if defined(TEST)
int main(void)
{
    /** @todo Some tests, perhaps? */
    return 0;
}
#endif
