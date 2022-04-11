/* thread_pid.h
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 01-Apr-2003 18:52 rob:)
 *
 */

#ifndef THREAD_PID_H
#define THREAD_PID_H	1

class ThreadPid
{
public:
    static unsigned Get();	// equivalent to getpid() but much faster
};

#endif
