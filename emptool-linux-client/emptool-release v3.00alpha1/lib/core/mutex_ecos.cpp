/* mutex_ecos.cpp
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.4 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"

#if defined(ECOS)

#include "mutex.h"
#include "mutexcycle.h"
#include "thread_pid.h"

#include <cyg/kernel/thread.hxx>

#define TRACE_MUTECOS	0

RawMutexEcos::RawMutexEcos()
    : m_mutex(), m_lock_count(0)
{
    ASSERT( m_mutex.get_owner() == NULL );
    ASSERT( m_lock_count == 0 );
}

RawMutexEcos::~RawMutexEcos()
{
    ASSERT( m_mutex.get_owner() == NULL );
    ASSERT( m_lock_count == 0 );
}

/* It is safe to check recursiveness without a lock, it turns out. */
void RawMutexEcos::Lock()
{
    TRACEC(TRACE_MUTECOS, "Lock: start\n");

    /* It's safe to check get_owner() like this because:
     *
     * If nobody owns the mutex, it returns NULL and we'll simply
     * block if somebody nips in before we get to lock().
     *
     * If somebody else owns the mutex, they won't be returning our
     * self() pointer, so we'll correctly block when we reach
     * lock(). If somebody else obtains the mutex again, we still
     * correctly block. And if the mutex is released, we don't block
     * but we correctly obtain it.
     *
     * If we own the mutex, get_owner() returns our pointer and we can
     * safely assume nobody else will be messing with the mutex - they
     * should all be blocked in lock().
     */
    if(m_mutex.get_owner() != Cyg_Thread::self())
    {
	TRACEC(TRACE_MUTECOS, "Lock: owned by someone else\n");

	/* Retry obtaining the lock until it succeeds. Apparently eCos
	 * quits out of lock() if a signal arrives etc. */
	do
	{
	} while(!m_mutex.lock());
	/* This is the first time our thread has obtained the mutex in
	 * this recursion. The last owner should have left the lock
	 * count at zero. Let's check that. */
	TRACEC(TRACE_MUTECOS, "Lock: obtained lock\n");

	ASSERT( m_lock_count == 0 );
    }
    else
	TRACEC(TRACE_MUTECOS, "Lock: already owned\n");
    	
    // We definitely have the mutex by now.
    // This assertion doesn't catch all, obviously.
    ASSERT( m_mutex.get_owner() == Cyg_Thread::self() );
    // Increment the lock count.
    ++m_lock_count;
    TRACEC(TRACE_MUTECOS, "Lock: count now %u\n", m_lock_count);
}

bool RawMutexEcos::TryLock()
{
    TRACEC(TRACE_MUTECOS, "TryLock: start\n");
    
    // The same rules as Lock() apply here
    if(m_mutex.get_owner() != Cyg_Thread::self())
    {
	TRACEC(TRACE_MUTECOS, "TryLock: owned by someone else\n");
	// Try obtaining the lock, but only once
	if(!m_mutex.trylock())
	{
	    TRACEC(TRACE_MUTECOS, "TryLock: still owned by someone else\n");
	    return false;
	}
	/* We now have the lock, check it's been left in the right
	 * state (zero). */
	TRACEC(TRACE_MUTECOS, "TryLock: obtained lock\n");
	ASSERT( m_lock_count == 0 );
    }
    ASSERT( m_mutex.get_owner() == Cyg_Thread::self() );
    // Increment the lock count
    ++m_lock_count;
    TRACEC(TRACE_MUTECOS, "TryLock: count now %u\n", m_lock_count);
    return true; // tis ours
}

void RawMutexEcos::Unlock()
{
    // We definitely own the mutex or there is a horrible logic bug in
    // the caller. Or this is bugged. Whatever.
    ASSERT( m_mutex.get_owner() == Cyg_Thread::self() );
    if(--m_lock_count == 0)
    {
	/* We've unlocked the mutex as many times as we locked
	 * it. It's time to give somebody else a go. */
	TRACEC(TRACE_MUTECOS, "Unlock: count now 0, unlocking\n");
	m_mutex.unlock();
    }
    else
	TRACEC(TRACE_MUTECOS, "Unlock: count now %u\n", m_lock_count);
}

#if DEBUG
void RawMutexEcos::IncCount()
{
    ++m_lock_count;
}

void RawMutexEcos::DecCount()
{
    VERIFY( m_lock_count-- != 0 );
}
#endif

#endif // ECOS
