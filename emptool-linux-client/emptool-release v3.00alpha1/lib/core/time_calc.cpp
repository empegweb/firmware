/* time_calc.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.7 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "time_calc.h"

#if !defined(WIN32) && !defined(ECOS)
#include "empeg_time.h"
#include <stdlib.h>

int TimeCalc::Remaining(timeval *till)
{
    timeval now;
    gettimeofday(&now, NULL);

    int msecs = (till->tv_sec - now.tv_sec) * 1000;
    msecs += (till->tv_usec - now.tv_usec) / 1000;

    //    if(msecs < 0) msecs = 0;
    return msecs;
}

void TimeCalc::At(timeval *till, int offset_ms)
{
    gettimeofday(till, NULL);
    div_t divres = div(offset_ms, 1000);
    till->tv_sec += divres.quot;
    till->tv_usec += divres.rem * 1000;
    if(till->tv_usec >= 1000000) {
	till->tv_usec -= 1000000;
	till->tv_sec ++;
    }
}
#endif

#if defined(TEST)
int main(void)
{
    /** @todo Some tests, perhaps? */
    return 0;
}
#endif
