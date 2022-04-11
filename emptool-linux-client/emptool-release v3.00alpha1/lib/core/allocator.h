/* allocator.h
 *
 * Generic allocation interface
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 13-Mar-2003 18:15 rob:)
 */

#ifndef ALLOCATOR_H
#define ALLOCATOR_H 1

class Allocator
{
public:
    virtual void *Alloc(int bytes) = 0;
    virtual void *Realloc(void *base, int bytes) = 0;
    virtual void Free(void *base) = 0;
#if DEBUG>0
    virtual bool IsPointerValid(void *base, void *ptr) = 0;
#endif
};

#endif

