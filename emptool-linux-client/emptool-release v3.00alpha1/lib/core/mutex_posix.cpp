/* mutex_posix.cpp
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.1 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"

#if !defined(ECOS) && !defined(WIN32)

#include "mutex.h"
#include "mutexcycle.h"
#include <unistd.h>
#include "thread_pid.h"

RawMutexPosix::RawMutexPosix()
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
#if defined(__USE_UNIX98)
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#else
    pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_RECURSIVE_NP /*ERRORCHECK_NP*/);
#endif
    int i = pthread_mutex_init(this, &attr);
    if (i)
    {
#if DEBUG
	TRACE_WARN("Couldn't create mutex at %p.\n", this);
#endif
	TRACE_WARN("Result of creating mutex: %d\n", i);
	ASSERT(false);
    }
    pthread_mutexattr_destroy(&attr);
}

#if defined(TEST)
int main(void)
{
    /** @todo Some tests, perhaps? */
    return 0;
}
#endif

#endif // !defined(ECOS) && !defined(WIN32)
