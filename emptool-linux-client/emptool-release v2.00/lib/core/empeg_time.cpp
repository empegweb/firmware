/* empeg_time.cpp
 *
 * Operations on time values
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.6 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "empeg_time.h"

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
    {
	if (tv1.tv_usec == tv2.tv_usec)
	    return false;
	else if (tv1.tv_usec < tv2.tv_usec)
	    return true;
	else
	    return false;
    } else if (tv1.tv_sec < tv2.tv_sec)
	return true;
    else
	return false;
}

bool operator<=(const Time &tv1, const Time &tv2)
{
    ASSERT_VALID(&tv1);
    ASSERT_VALID(&tv2);
	
    if (tv1.tv_sec == tv2.tv_sec)
    {
	if (tv1.tv_usec == tv2.tv_usec)
	    return true;
	else if (tv1.tv_usec < tv2.tv_usec)
	    return true;
	else
	    return false;
    } else if (tv1.tv_sec < tv2.tv_sec)
	return true;
    else
	return false;
}

bool operator>(const Time &tv1, const Time &tv2)
{
    ASSERT_VALID(&tv1);
    ASSERT_VALID(&tv2);
	
    if (tv1.tv_sec == tv2.tv_sec)
    {
	if (tv1.tv_usec == tv2.tv_usec)
	    return false;
	else if (tv1.tv_usec > tv2.tv_usec)
	    return true;
	else
	    return false;
    } else if (tv1.tv_sec > tv2.tv_sec)
	return true;
    else
	return false;
}

bool operator>=(const Time &tv1, const Time &tv2)
{
    ASSERT_VALID(&tv1);
    ASSERT_VALID(&tv2);
	
    if (tv1.tv_sec == tv2.tv_sec)
    {
	if (tv1.tv_usec == tv2.tv_usec)
	    return true;
	else if (tv1.tv_usec > tv2.tv_usec)
	    return true;
	else
	    return false;
    } else if (tv1.tv_sec > tv2.tv_sec)
	return true;
    else
	return false;
}
