/* interval.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.8 13-Mar-2003 18:15 rob:)
 */

#ifndef INTERVAL_H
#define INTERVAL_H 1

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Win32 implementation
class Interval
{
    DWORD start_time;
    int timeout_ms;
    
public:
    Interval(int timeout_in_ms)
	: timeout_ms(timeout_in_ms)
    {
	start_time = GetTickCount();
    }
    bool Pending() const
    {
	DWORD now = GetTickCount();
	
	// Deal with the complex case. Like this:
	// |___t       s___| where | indicates the bounds of the type
	//                        t is the timeout time.
	//                        s is the start time.
	//                        _ is the space during which we are pending
	//                          is the space during which we are timed out.
	
	if (start_time + timeout_ms < start_time)
	{
	    if (now < start_time && now > start_time + timeout_ms)
		return false;
	    else
		return true;
	}
	
	// Deal with the simple case
	// |    s_____t    |
	return (now < start_time + timeout_ms);
    }

    bool Expired() const
    {
	return !Pending();
    }
};
#else

#include "empeg_time.h"

// UNIX implementation
class Interval
{
    Time timeout_time;
    
public:
    Interval(int timeout_in_ms);
    
    bool Pending() const
    {
	return Time::Now() < timeout_time;
    }

    bool Expired() const
    {
	return !Pending();
    }
};
#endif

#endif
