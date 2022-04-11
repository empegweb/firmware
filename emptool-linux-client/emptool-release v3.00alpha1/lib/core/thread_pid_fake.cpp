/* thread_pid_fake.cpp
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * This is used when we aren't running threads so that things using
 * this library need not rely on libpthreads.
 *
 * (:Empeg Source Release 1.1 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "thread_pid.h"

unsigned ThreadPid::Get()
{
    return 0;
}
