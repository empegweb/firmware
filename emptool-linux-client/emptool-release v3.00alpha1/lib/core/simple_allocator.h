/* simple_allocator.h
 *
 * (C) 2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 13-Mar-2003 18:15 rob:)
 */

#ifndef SIMPLE_ALLOCATOR_H
#define SIMPLE_ALLOCATOR_H 1

#include "allocator.h"

/*!
  Implementation of Allocator in terms of malloc() and free().
 */
class SimpleAllocator : public Allocator
{
public:
    virtual void *Alloc(int cb)    { return malloc(cb); }
    virtual void *Realloc(void *p, int cb) { return realloc(p, cb); }
    virtual void Free(void *p) { /* return */ free(p); }
#if DEBUG>0
    virtual bool IsPointerValid(void * /*base*/, void * /*ptr*/) { return true; }
#endif
};

#endif /* SIMPLE_ALLOCATOR_H */
