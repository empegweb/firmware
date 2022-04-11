/* mutex.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.36 13-Mar-2003 18:15 rob:)
 *
 * The RawMutex class is a thin veneer over the system implementation. Do not
 * use this directly, use the nice Mutex class instead. This would best be done
 * with an abstract base class full of virtual functions, but for efficiency
 * (and with only five one-line members anyway) we duplicate the code.
 */

#ifndef MUTEX_H
#define MUTEX_H 1

#if DEBUG>2
#define MUTEX_DEBUG 1
#else
#define MUTEX_DEBUG 0
#endif

#if defined(WIN32)

// Win32 semi-implementation (no TryLock)
# define HAVE_SIMPLE_MUTEX	0
# include "mutex_win32.h"
typedef RawMutexWin32 RawMutex;

#elif defined(ECOS)

// eCos implementation
# define HAVE_SIMPLE_MUTEX	0
# include "mutex_ecos.h"
typedef RawMutexEcos RawMutex;

#else

// POSIX-threads implementation
# define HAVE_SIMPLE_MUTEX	1
# include "mutex_posix.h"
typedef RawMutexPosix RawMutex;
typedef SimpleMutexPosix SimpleMutex;

#endif

#define TRACK_MUTEXES		0
#define MUTEX_MAGIC		0xfeedf00dUL

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
    
// TryLock() returns true if the lock has been obtained successfully, or false if not.
#if MUTEX_DEBUG
    bool TryLock(void *caller = NULL);
#else
    bool TryLock();
#endif
#if DEBUG
    const char *GetName() const;
#endif

    inline RawMutex *GetRawMutex() { return &mutex; }
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
	TRACE_WARN("Mutex '%s' kept for too long\n",
	     mutex.GetName());
#endif
}

#if HAVE_SIMPLE_MUTEX
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
#endif // HAVE_SIMPLE_MUTEX

#endif // MUTEX_H
