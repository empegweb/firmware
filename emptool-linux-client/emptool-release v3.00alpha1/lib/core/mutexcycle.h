/* mutexcycle.h
 *
 * In debug builds, check for cycles in the mutex-holding graph
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.13 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *    Peter Hartley <peter@empeg.com>
 */

#ifndef included_mutexcycle_h
#define included_mutexcycle_h

#if !defined(WIN32)
#include <map>
#include <set>

#include "mutex.h"
#include "cyclecheck.h"
#include <unistd.h>

/** A class for checking that mutex lock nesting obeys a total order on
 * mutexes.
 * To enable the checking, just create an instance of this object somewhere
 * (at the moment player.cpp creates one in main() in DEBUG>=3 builds).
 */
class MutexCycleChecker
{
    static RawMutex m; // leaf, doesn't figure in our own calculations!
    static MutexCycleChecker *instance;

public:
    MutexCycleChecker();
    ~MutexCycleChecker();

#if DEBUG>0
    static void LockNotify( const RawMutex*, const char *name=0,
	bool shared=false );
    static void UnlockNotify( const RawMutex* );
    static void DestroyNotify( const RawMutex* );
#else
    static void LockNotify( const RawMutex*, const char * = 0, bool = false ) {}
    static void UnlockNotify( const RawMutex* ) {}
    static void DestroyNotify( const RawMutex* ) {}
#endif

private:
    typedef std::set<const RawMutex*> mutex_set_t;
    typedef std::set<const RawMutex*>::iterator mutex_iterator;

    typedef std::map<pid_t, mutex_set_t> mutex_owners_t;
    mutex_owners_t mutex_owners;
    mutex_owners_t shared_owners;

    typedef std::map<const RawMutex*,const char*> mutex_map_t;
    mutex_map_t mutex_names;

    CycleChecker<const RawMutex*> cc;

    void AddEdges( mutex_set_t, const RawMutex*, bool );
};
#endif /* !defined(WIN32) && !defined(ECOS) */
#endif
