/* mutex_ecos.h
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 *
 * The RawMutex class is a thin veneer over the system implementation. Do not
 * use this directly, use the nice Mutex class instead. This would best be done
 * with an abstract base class full of virtual functions, but for efficiency
 * (and with only five one-line members anyway) we duplicate the code.
 */

#ifndef MUTEX_ECOS_H
#define MUTEX_ECOS_H 1

// eCos implementation where we provide our own recursiveness
#include <pkgconf/kernel.h>
#include <cyg/kernel/mutex.hxx>

class RawMutexEcos
{
    Cyg_Mutex m_mutex;
    unsigned m_lock_count;
public:
    RawMutexEcos();
    ~RawMutexEcos();
    void Lock();	// almost trivial
    bool TryLock();	// very nearly trivial
    void Unlock();      // very very nearly trivial
    inline Cyg_Mutex *GetImpl() { return &m_mutex; }
    void IncCount();
    void DecCount();
    inline unsigned GetLockCount() const { return m_lock_count; }
};

#if !DEBUG
inline void RawMutexEcos::IncCount()
{
    ++m_lock_count;
}
inline void RawMutexEcos::DecCount()
{
    --m_lock_count;
}
#endif

#endif // MUTEX_ECOS_H
