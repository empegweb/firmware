/* empeg_time_hack.cpp
 *
 * big-player hack for empeg_time. These functions don't get brought
 * in otherwise.
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.1 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "empeg_time.h"

#if DEBUG == 0 && defined(MERCURY)

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

#endif
