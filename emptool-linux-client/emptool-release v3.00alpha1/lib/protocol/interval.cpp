/* interval.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "interval.h"

#if !defined(WIN32) // Win32 constructor's in the header file
Interval::Interval(int timeout_in_ms)
    : timeout_time(Time::Now() + timeout_in_ms * 1000)
{
}

#endif // WIN32
