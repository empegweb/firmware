/* timeout.cpp
 *
 * Handle automatic timeouts
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.4 01-Apr-2003 18:52 rob:)
 */
#include "config.h"
#include "trace.h"
#include "timeout.h"

#if DEBUG > 0

void Timeout::Reset()
{
    ASSERT_VALID(this);
    timeout_time = Time::Now();
    timeout_time += period * 1000;
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
