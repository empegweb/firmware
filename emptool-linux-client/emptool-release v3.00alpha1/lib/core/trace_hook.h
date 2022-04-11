/* trace_hook.h
 *
 * (C) 2003 Sonicblue
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from Sonicblue.
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 */

#ifndef TRACE_HOOK_H
#define TRACE_HOOK_H

class TraceHook
{
public:
    virtual void Put(const char *) = 0;
};

extern "C" void empeg_settracehook(TraceHook *);
extern "C" void empeg_unsettracehook(TraceHook *);

#endif // TRACE_HOOK_H
