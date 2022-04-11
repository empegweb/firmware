/* empeg_time.cpp
 *
 * Operations on time values
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.14 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "empeg_time.h"

#ifdef ECOS
// eCos support functions

#include <cyg/hal/hal_platform_ints.h>

#define ECOS_TIMER_FREQ		100
#define ECOS_TIMER_PERIOD_USEC	(1000000 / ECOS_TIMER_FREQ)

cyg_tick_count Time::ToEcosClockTicks(bool round_up) const
{
    // Convert microseconds (tv_sec:tv_usec) to clock ticks
    cyg_tick_count ticks = GetSeconds() * ECOS_TIMER_FREQ;
    unsigned frac = GetFractionalMicroseconds();
    if(round_up)
	frac += ECOS_TIMER_PERIOD_USEC - 1;
    frac /= ECOS_TIMER_PERIOD_USEC;
    ticks += frac;
    return ticks;
}

Time Time::FromEcosClockTicks(cyg_tick_count ticks)
{
    cyg_tick_count usecs = ticks * ECOS_TIMER_PERIOD_USEC;
    return Time(usecs / 1000000, usecs % 1000000);
}

Time Time::Now()
{
    // Get time using 512/627.2kHz timer
    cyg_uint64 ticks = HAL_HIRES_CLOCK_READ();
    // Convert to usecs
#if defined(GRANITE)
# if CYGHWR_HAL_ARM_GRANITE_PROCESSOR_CLOCK < 90316
    ticks = (ticks * 125) / 64;	// equivalent to 1000000/512000
# else
    ticks = (ticks * 625) / 392; // equivalent to 1000000/627200
# endif
#elif defined(ONYX)
    // ticks already at 1MHz
#else
# error Dunno the tick rate here
#endif
    return Time(ticks / 1000000, ticks % 1000000);
}
#endif

#if DEBUG > 0

void Time::Clear()
{
    ASSERT_VALID(this);
    tv_sec = tv_usec = 0;
}

Time &Time::operator+=(int n)
{
    ASSERT_VALID(this);
    tv_usec += n;
    Normalise();
    return *this;
}

Time &Time::operator+=(const Time &t)
{
    ASSERT_VALID(this);
    ASSERT_VALID(&t);
    tv_sec += t.tv_sec;
    tv_usec += t.tv_usec;
    Normalise();
    return *this;
}

Time &Time::operator-=(int n)
{
    ASSERT_VALID(this);
    tv_usec -= n;
    Normalise();
    return *this;
}
    
Time &Time::operator-=(const Time &t)
{
    ASSERT_VALID(this);
    ASSERT_VALID(&t);
    tv_sec -= t.tv_sec;
    tv_usec -= t.tv_usec;
    Normalise();
    return *this;
}
	
void Time::Dump(int trace, const char *s)
{
    TRACEC(trace, "%s: %10ld.%06ld\n", s, tv_sec, tv_usec);
//    UNUSED(trace);
//    UNUSED(s);
}

#endif

void Time::Normalise()
{
    ASSERT_VALID(this);
#if 0
    while (tv_usec < 0)
    {
	tv_sec--;
	tv_usec += ONE_SECOND;
    }
	
    while (tv_usec > ONE_SECOND)
    {
	tv_sec++;
	tv_usec -= ONE_SECOND;
    }
#else
    if(tv_usec < 0) {
	int sub_secs = (999999 - tv_usec) / 1000000;
	tv_sec -= sub_secs;
	tv_usec += 1000000 * sub_secs;
    }
    else if(tv_usec >= 1000000) {
	tv_sec += tv_usec / 1000000;
	tv_usec = tv_usec % 1000000;
    }
#endif
}

bool operator<(const Time &tv1, const Time &tv2)
{
    ASSERT_VALID(&tv1);
    ASSERT_VALID(&tv2);
    
    if (tv1.tv_sec == tv2.tv_sec)
	return tv1.tv_usec < tv2.tv_usec;
    else
        return tv1.tv_sec < tv2.tv_sec;
}

bool operator<=(const Time &tv1, const Time &tv2)
{
    ASSERT_VALID(&tv1);
    ASSERT_VALID(&tv2);
	
    if (tv1.tv_sec == tv2.tv_sec)
	return tv1.tv_usec <= tv2.tv_usec;
    else
        return tv1.tv_sec <= tv2.tv_sec;
}

bool operator>(const Time &tv1, const Time &tv2)
{
    ASSERT_VALID(&tv1);
    ASSERT_VALID(&tv2);
	
    if (tv1.tv_sec == tv2.tv_sec)
	return tv1.tv_usec > tv2.tv_usec;
    else
        return tv1.tv_sec > tv2.tv_sec;
}

bool operator>=(const Time &tv1, const Time &tv2)
{
    ASSERT_VALID(&tv1);
    ASSERT_VALID(&tv2);
	
    if (tv1.tv_sec == tv2.tv_sec)
	return tv1.tv_usec >= tv2.tv_usec;
    else
        return tv1.tv_sec >= tv2.tv_sec;
}

#if defined(TEST)
int main(void)
{
    /** @todo Some tests, perhaps? */
    return 0;
}
#endif
