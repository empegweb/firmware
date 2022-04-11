/* mutex_posix.h
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.1 13-Mar-2003 18:15 rob:)
 *
 * The RawMutex class is a thin veneer over the system implementation. Do not
 * use this directly, use the nice Mutex class instead. This would best be done
 * with an abstract base class full of virtual functions, but for efficiency
 * (and with only five one-line members anyway) we duplicate the code.
 */

#ifndef MUTEX_POSIX_H
#define MUTEX_POSIX_H 1

// Posix (pthreads) implementation
#include <pthread.h>

#if DEBUG>2
#define MUTEX_DEBUG 1
#else
#define MUTEX_DEBUG 0
#endif

#if MUTEX_DEBUG
#include "empeg_time.h"
#endif

// glibc implementation with recursive mutexes built in
class RawMutexPosix : public pthread_mutex_t
{
 public:
    RawMutexPosix(); // a bit too complex to inline
    ~RawMutexPosix() { pthread_mutex_destroy(this); }
    void Lock()      { pthread_mutex_lock(this); }
    void Unlock()    { pthread_mutex_unlock(this); }
    bool TryLock()   { return pthread_mutex_trylock(this) == 0; }
};

// Non-portable function which works on Linux glibc 2.2 only!
// See eCos implementation if we ever need a fallback
extern "C" int pthread_mutexattr_setkind_np(pthread_mutexattr_t *attr,
					    int kind);

// A SimpleMutex is just a very thin veneer around a pthread mutex. It is
// provided only for people who want to declare a mutex with static storage
// class and don't want to get into static constructor dependency messes.
struct SimpleMutexPosix
{
    pthread_mutex_t mx;
    void Lock()    { pthread_mutex_lock(&mx); }
    void Unlock()  { pthread_mutex_unlock(&mx); }
    bool TryLock() { return pthread_mutex_trylock(&mx) == 0; }
};

#define SIMPLEMUTEX_INITIALISER { PTHREAD_MUTEX_INITIALIZER }

#endif // MUTEX_POSIX_H
