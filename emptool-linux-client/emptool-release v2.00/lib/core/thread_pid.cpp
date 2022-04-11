/* thread_pid.cpp
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "thread_pid.h"
#include <unistd.h>
#include <pthread.h>

static pthread_key_t thread_id_key;

static void key_create_function()
{
    pthread_key_create(&thread_id_key, NULL);
}

unsigned ThreadPid::Get()
{
    static pthread_once_t key_create_once = PTHREAD_ONCE_INIT;
    // Initialise the key (if not already initialised - this is atomic)
    pthread_once(&key_create_once, key_create_function);
    // See if we've already set it
    unsigned pid = (unsigned) pthread_getspecific(thread_id_key);
    if(pid)
	return pid;
    // Haven't grabbed it yet
    pid = getpid();
    pthread_setspecific(thread_id_key, (const void *) pid);
    return pid;
}
