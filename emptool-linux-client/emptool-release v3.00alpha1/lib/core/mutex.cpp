/* mutex.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.28 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "mutex.h"
#ifndef WIN32
#include "mutexcycle.h"
#endif
#include "thread_pid.h"

#if MUTEX_DEBUG
Mutex::Mutex(const char *s, bool cw)
    : name(s), contention_watch(cw), locked_by_pid(0),
      locked_by_caller(CALLED_BY)
{
    ASSERT_VALID(name);
    magic = MUTEX_MAGIC;
}
#elif DEBUG
Mutex::Mutex(const char *s)
    : name(s)
{
    ASSERT_VALID(name);
    magic = MUTEX_MAGIC;
}
#else
Mutex::Mutex(const char *)
{
}
#endif

Mutex::~Mutex()
{
    ASSERT_VALID(this);
#if DEBUG
    ASSERT(magic == MUTEX_MAGIC);
#endif
	
    // What's going on here exactly?
    // we'll just let things break if you're not careful
    // ASSERT(TryLock());
    // Unlock();

#if MUTEX_DEBUG
    MutexCycleChecker::DestroyNotify( &mutex );
#endif

#if DEBUG
    magic = 0;
#endif
}

//extern int audio_pid_you_didnt_see_this_right;

#if DEBUG || MUTEX_DEBUG
void Mutex::Lock()
{
    ASSERT_VALID(this);
    ASSERT_EX(magic == MUTEX_MAGIC, "this=%p, magic = 0x%lx, expected = 0x%lx\n", this, magic, MUTEX_MAGIC);

#if MUTEX_DEBUG
    if (!TryLock(CALLED_BY))
    {
	if (contention_watch
	    //|| ThreadPid::Get() == audio_pid_you_didnt_see_this_right
	    )
	{
	    TRACE("Blocking on locking mutex '%s', called by %p\n", name, CALLED_BY);
	    TRACE("Thread %d had it locked inside %p\n", locked_by_pid, locked_by_caller);
	    //TRACE("Blocking on locking mutex '%s', called by %p\n", name, CALLED_BY);
	}
	
	MutexCycleChecker::LockNotify( &mutex, name );
	
	mutex.Lock();
	locked_by_pid = ThreadPid::Get();
	locked_by_caller = CALLED_BY;
    }
#else
    mutex.Lock();
#endif
}

void Mutex::Unlock()
{
    ASSERT_VALID(this);
    ASSERT(magic == MUTEX_MAGIC);

    //Don't do this because the chances are that someone won't get
    //to look at them in time!
    //locked_by_pid = 0;
    //locked_by_caller = NULL;
    
    mutex.Unlock();

#if MUTEX_DEBUG
    MutexCycleChecker::UnlockNotify( &mutex );
#endif
}

#ifndef WIN32
bool Mutex::TryLock(
#if MUTEX_DEBUG
    void *caller
#endif
    )
{
    ASSERT_VALID(this);
    ASSERT(magic == MUTEX_MAGIC);
    bool result = mutex.TryLock();

#if MUTEX_DEBUG
    if ( result )
    {
	MutexCycleChecker::LockNotify( &mutex, name );
	locked_by_pid = ThreadPid::Get();
	locked_by_caller = caller ? caller : CALLED_BY;
    }
#endif

    return result;
}
#endif // !WIN32

#endif // DEBUG || MUTEX_DEBUG

#if defined(TEST)
int main(void)
{
    /** @todo Some tests, perhaps? */
    return 0;
}
#endif
