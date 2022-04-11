/* timeout.h
 *
 * Handle automatic timeouts
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.10 13-Mar-2003 18:15 rob:)
 */
#ifndef TIMEOUT_H
#define TIMEOUT_H	1

#ifndef CONFIG_H
#include "config.h"
#endif
#ifndef TRACE_H
#include "trace.h"
#endif
#ifndef EMPEG_TIME_H
#include "empeg_time.h"
#endif

class Timeout
{
    int period;		/// In milliseconds
    bool frozen;

public:
    Time timeout_time;

    inline Timeout(int ms)
	: period(ms),
	  frozen(false),
	  timeout_time()
    {
	// this is much cheaper than a gettimeofday() in Time::Now()
	timeout_time.Clear();
    }

#if DEBUG == 0
    inline void Reset()
    {
	ASSERT_VALID(this);
	timeout_time = Time::Now();
	timeout_time += Time(period / 1000, (period % 1000) * 1000);
	frozen = false;
    }

    inline bool TimedOut() const
    {
	ASSERT_VALID(this);
	return (!frozen) && (Time::Now() > timeout_time);
    }

    inline bool NotTimedOut() const
    {
	ASSERT_VALID(this);
	return frozen || (Time::Now() <= timeout_time);
    }
#else
    // see timeout.cpp
    void Reset();
    bool TimedOut() const;
    bool NotTimedOut() const;
#endif

    inline bool Frozen() const { return frozen; }
    
    inline void Expire()
    {
	timeout_time.Clear();
    }

    inline void Freeze()
    {
	frozen = true;
    }

    inline void Unfreeze()
    {
	frozen = false;
    }

    /// This won't affect a currently active timeout.
    inline void SetPeriod(int ms)
    {
	period = ms;
    }

    inline int GetPeriod() const
    {
	return period;
    }

    inline int GetRemainingMilliseconds() const
    {
        const Time now = Time::Now();
        if (timeout_time < now)
            return 0;
        else
            return (timeout_time - now).GetAsMilliseconds();
    }
};

#endif
