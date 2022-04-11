/* timeout.cpp
 *
 * Handle automatic timeouts
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.6 13-Mar-2003 18:15 rob:)
 */
#include "config.h"
#include "trace.h"
#include "timeout.h"

#if DEBUG > 0

void Timeout::Reset()
{
    ASSERT_VALID(this);
    timeout_time = Time::Now();
    timeout_time += Time(period / 1000, (period % 1000) * 1000);
    frozen = false;
}

bool Timeout::TimedOut() const
{
    ASSERT_VALID(this);
    return (!frozen) && (Time::Now() > timeout_time);
}

bool Timeout::NotTimedOut() const
{
    ASSERT_VALID(this);
    return frozen || (Time::Now() <= timeout_time);
}

#endif

#if defined(TEST)
int main(void)
{
    /** @todo Some tests, perhaps? */
    return 0;
}
#endif
