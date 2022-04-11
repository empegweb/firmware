/* interval.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.4 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "interval.h"

#if !defined(WIN32) // Win32 constructor's in the header file
Interval::Interval(int timeout_in_ms)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    timeout_time = now;
    
    timeout_time.tv_usec += (timeout_in_ms % 1000) * 1000;
    timeout_time.tv_sec += timeout_in_ms / 1000;
    
    if (timeout_time.tv_usec > 1000000)
    {
	timeout_time.tv_sec++;
	timeout_time.tv_usec -= 1000000;
    }
    ASSERT(timeout_time.tv_usec < 1000000);
#if 0
    TRACE("Timeout start is %d.%06d, timeout time is %d.%06d, timeout is %d\n",
	  now.tv_sec, now.tv_usec, timeout_time.tv_sec, timeout_time.tv_usec,
	  timeout_in_ms);
#endif
}

#endif // WIN32
