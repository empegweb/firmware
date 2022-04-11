/* mutex.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.29 01-Apr-2003 18:52 rob:)
 *
 * The RawMutex class is a thin veneer over the system implementation. Do not
 * use this directly, use the nice Mutex class instead. This would best be done
 * with an abstract base class full of virtual functions, but for efficiency
 * (and with only five one-line members anyway) we duplicate the code.
 */

#ifndef MUTEX_H
#define MUTEX_H 1

#ifndef CONFIG_H
#include "config.h"
#endif

#ifdef WIN32

// Win32 implementation: uses CRITICAL_SECTIONs (not mutexes) as they're more
// efficient in a single-process application
#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

class RawMutex
{
    CRITICAL_SECTION m_cs; // a CRITICAL_SECTION is a recursive mutex

 public:
    RawMutex()     { ::InitializeCriticalSection(&m_cs); }
    ~RawMutex()    { ::DeleteCriticalSection(&m_cs); }
    void Lock()    { ::EnterCriticalSection(&m_cs); }
    void Unlock()  { ::LeaveCriticalSection(&m_cs); }
    //bool TryLock() { return ::TryEnterCriticalSection(&m_cs); } // Win95 doesn't have this
};

#else /* !WIN32 */

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

class RawMutex : public pthread_mutex_t
{
 public:
    RawMutex();    // a bit too complex to inline
    ~RawMutex()    { pthread_mutex_destroy(this); }
    void Lock()    { pthread_mutex_lock(this); }
    void Unlock()  { pthread_mutex_unlock(this); }
    bool TryLock() { return pthread_mutex_trylock(this) == 0; }
};

#endif

//#include "mutexcycle.h"

#define TRACK_MUTEXES 0

// now THIS is dodgy
#ifndef WIN32
extern "C" int pthread_mutexattr_setkind_np(pthread_mutexattr_t *attr,
					    int kind);
#endif

#define MUTEX_MAGIC 0xfeedf00dUL

class Mutex
{
    RawMutex mutex;
#if DEBUG
    unsigned long magic;
    const char *name;
#endif
#if MUTEX_DEBUG
    bool contention_watch;
    int locked_by_pid;
    void *locked_by_caller;
#endif
private:
    // No implementation
    Mutex(const Mutex &);
    void operator=(const Mutex &);

public:
#if MUTEX_DEBUG
    explicit Mutex(const char * = "Anonymous", bool contention_watch = false);
#else
    explicit Mutex(const char * = "Anonymous");
#endif
    ~Mutex();
    void Lock();
    void Unlock();
#if MUTEX_DEBUG
    bool TryLock(void *caller = NULL);
#else
    bool TryLock();
#endif
#if DEBUG
    const char *GetName() const;
#endif

    inline RawMutex *GetRawMutex() { return &mutex; }

    friend class Condition;
};

class MutexLock
{
    DISALLOW_COPYING(MutexLock);

    Mutex &mutex;
#if TRACK_MUTEXES
    Time time_start;
#endif
public:
    explicit MutexLock(Mutex &m);
    ~MutexLock();
};

#if !MUTEX_DEBUG && !DEBUG
inline void Mutex::Lock()
{
    mutex.Lock();
}

inline void Mutex::Unlock()
{
    mutex.Unlock();
}

#ifndef WIN32
inline bool Mutex::TryLock()
{
    return mutex.TryLock();
}
#endif // !WIN32
#endif // !MUTEX_DEBUG && !DEBUG

#if DEBUG
inline const char *Mutex::GetName() const
{
    return name;
}
#endif

inline MutexLock::MutexLock(Mutex &m) : mutex(m)
{
    mutex.Lock();
#if TRACK_MUTEXES
    time_start = Time::Now();
#endif
}

inline MutexLock::~MutexLock()
{
#if TRACK_MUTEXES
    static const Time threshold(500000);
    Time time_end = Time::Now();
#endif
    mutex.Unlock();
#if TRACK_MUTEXES
    if (time_end - time_start > threshold)
	WARN("Mutex '%s' kept for too long\n",
	     mutex.GetName());
#endif
}

#ifndef WIN32
// A SimpleMutex is just a very thin veneer around a pthread mutex. It is
// provided only for people who want to declare a mutex with static storage
// class and don't want to get into static constructor dependency messes.
struct SimpleMutex
{
    pthread_mutex_t mx;
    void Lock()    { pthread_mutex_lock(&mx); }
    void Unlock()  { pthread_mutex_unlock(&mx); }
    bool TryLock() { return pthread_mutex_trylock(&mx) == 0; }
};

class SimpleMutexLock
{
    SimpleMutex *m_psm;
    DISALLOW_COPYING(SimpleMutexLock);

 public:
    SimpleMutexLock( SimpleMutex *psm ): m_psm(psm)
	{ m_psm->Lock(); }
    ~SimpleMutexLock()
	{ m_psm->Unlock(); }
};

#define SIMPLEMUTEX_INITIALISER { PTHREAD_MUTEX_INITIALIZER }

#endif

#endif
