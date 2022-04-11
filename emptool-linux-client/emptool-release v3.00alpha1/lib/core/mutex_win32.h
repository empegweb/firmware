/* mutex_win32.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
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

#ifndef MUTEX_WIN32_H
#define MUTEX_WIN32_H 1

// Win32 implementation: uses CRITICAL_SECTIONs (not mutexes) as they're more
// efficient in a single-process application
#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

class RawMutexWin32
{
    CRITICAL_SECTION m_cs; // a CRITICAL_SECTION is a recursive mutex

 public:
    RawMutexWin32()  { ::InitializeCriticalSection(&m_cs); }
    ~RawMutexWin32() { ::DeleteCriticalSection(&m_cs); }
    void Lock()      { ::EnterCriticalSection(&m_cs); }
    void Unlock()    { ::LeaveCriticalSection(&m_cs); }
    //bool TryLock() { return ::TryEnterCriticalSection(&m_cs); } // Win95 doesn't have this
};

#endif // MUTEX_WIN32_H
