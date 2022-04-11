/* mutexcycle.cpp
 *
 * In debug builds, check for cycles in the mutex-holding graph
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.16 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *    Peter Hartley <peter@empeg.com>
 */

#include "config.h"
#include "trace.h"
#include "mutexcycle.h"

#if !defined(WIN32)
#include "cyclecheck2.h"
#include "empeg_error.h"
#include "thread_pid.h"

#if DEBUG>0

MutexCycleChecker *MutexCycleChecker::instance = 0;
RawMutex MutexCycleChecker::m;

MutexCycleChecker::MutexCycleChecker()
{
    instance = this;
}

MutexCycleChecker::~MutexCycleChecker()
{
#if DEBUG>=2
    /* Dump mutex tree */
    cc.DumpNamed( mutex_names );
#endif

    instance = NULL;
}

typedef std::list<const RawMutex*> mutex_list_t;

void MutexCycleChecker::AddEdges( const mutex_set_t from,
				  const RawMutex* to, bool shared )
{
    for ( mutex_iterator i = from.begin(); i != from.end(); ++i )
    {
	mutex_list_t cycle;
	
	int rc = cc.AddEdge( *i, to, cycle, shared );
	
	if ( rc == CycleChecker<const RawMutex*>::ADDEDGE_LOOPS )
	{
	    const char *s1 = mutex_names[*i];
	    const char *s2 = mutex_names[to];
	    
	    TRACE_WARN("*** MUTEX CYCLE DETECTED\n");
	    TRACE_WARN("*** while trying to add %s edge %p(%s)->%p(%s)\n",
		 shared ? "weak" : "strong",
		 *i, s1 ? s1 : "",
		 to, s2 ? s2 : ""
		);
	    for ( mutex_list_t::iterator j = cycle.begin();
		  j != cycle.end();
		  ++j )
	    {
		TRACE_WARN("*** %p(%s) ->\n", *j, mutex_names[*j] );
	    }
	    TRACE_WARN("*** %p(%s)\n", *cycle.begin(),
		 mutex_names[*cycle.begin()] );
	    TRACE_WARN("*** THIS IS A BAD THING\n\n");
	    ASSERT(false);
	}
	else if ( rc == CycleChecker<const RawMutex*>::ADDEDGE_NEW )
	{
/*	    const char *s1 = mutex_names[*i];
	    const char *s2 = mutex_names[to];
	    
	    if ( !s1 )
		s1 = "";
	    if ( !s2 )
		s2 = "";
	    
	    TRACE("Added edge %p(%s)->%p(%s) %s\n",
		  *i, s1,
		  to, s2,
		  shared ? "(weak)" : ""); */
	}
    }
}

void MutexCycleChecker::LockNotify( const RawMutex *newmutex,
				    const char *name, bool shared )
{
    /* There are just too many of these */
    if ( shared && ( !strcmp(name, "CountedObject")
		     || !strcmp(name,"rwl-chunk") ) )
	return;

    m.Lock();

    MutexCycleChecker *mcc = instance;
    if ( mcc )
    {
	mcc->mutex_names[newmutex] = name ? name : "";
	    
	pid_t owner = ThreadPid::Get();

	//TRACE("%p locks %s\n", (void*)owner, name );

	// Exclusive and shared locks already held by this thread
	mutex_set_t& current_excl = mcc->mutex_owners[owner];
	mutex_set_t& current_shared = mcc->shared_owners[owner];

	/* Getting the same mutex twice in the same thread doesn't count.
	 * Consider:
	 * lock a do
	 *     lock b do
	 *         lock a do
	 *         ...
	 *         end
	 *     end
	 * end
	 *
	 * This is not an error, and can deadlock only with other threads
	 * that lock b then a, not ones that lock a then b.
	 */
	if ( current_excl.find(newmutex) == current_excl.end() )
	    mcc->AddEdges( current_excl, newmutex, shared );

	// Add weak edges from held shared locks to this lock
	if ( !shared )
	{
	    if ( current_shared.find(newmutex) == current_shared.end() )
		mcc->AddEdges( current_shared, newmutex, true );
	}

	if ( shared )
	    current_shared.insert( newmutex );
	else
	    current_excl.insert( newmutex );
    }

    m.Unlock();
}

void MutexCycleChecker::UnlockNotify( const RawMutex *oldmutex )
{
    m.Lock();

    MutexCycleChecker *mcc = instance;
    if ( mcc )
    {
	pid_t owner = ThreadPid::Get();
	mutex_set_t& ms = mcc->mutex_owners[owner];

	ms.erase( oldmutex );

	mcc->shared_owners[owner].erase( oldmutex );
    }

    m.Unlock();
}

void MutexCycleChecker::DestroyNotify( const RawMutex *oldmutex )
{
    m.Lock();

    MutexCycleChecker *mcc = instance;
    if ( mcc )
    {
	ASSERT_PTR(mcc);
	
	// Hopefully it's not in any of the owner lists ;-)
	mcc->cc.DeleteNode( oldmutex );
    }
    
    m.Unlock();
}

#endif

#ifdef TEST

#include "mutex.h"

int main(void)
{
    Mutex a("First");
    Mutex b("Second");

    (void) NEW MutexCycleChecker();

    {
	MutexLock la(a);
	{
	    MutexLock lb(b);

	    cout << "haha I have them both\n";
	}
    }

    {
	MutexLock lb(b);
	{
	    MutexLock la(a);

	    cout << "haha I have them both again\n";
	}
    }

    return 0;
}


#endif  // TEST
#endif	// !WIN32 && !ECOS

/* eof */
