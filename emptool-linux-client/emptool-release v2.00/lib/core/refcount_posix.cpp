/* refcount_posix.h
 *   Implementation of reference counted pointer and object functionality for
 *   POSIX threaded platforms.
 *
 * (C) 1999-2002 empeg ltd
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *
 * (:Empeg Source Release 1.2 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "refcount_posix.h"

CountedObject::CountedObject()
    : count_mutex("CountedObject"),
      count(0)
{
#if DEBUG
    initialized = COUNTED_MAGIC_NUMBER;
#endif
}

CountedObject::~CountedObject()
{
    ASSERT_PTR(this);
#if DEBUG
    ASSERT(initialized == COUNTED_MAGIC_NUMBER);
#endif
    ASSERT_EX(count == 0, "this=%p, count=%ld\n", this, count);
#if DEBUG
    initialized = 0x00010001;
#endif
    TRACEC(TRACE_REFCOUNT, "Object at %p now destroyed.\n", this);
}

void CountedObject::AddRef()
{
    // TRACEC(TRACE_REFCOUNT, "Trying to add reference to object at %p\n", this);
    ASSERT_PTR(this);
#if DEBUG
    ASSERT(initialized == COUNTED_MAGIC_NUMBER);
#endif
    count_mutex.Lock();
    count++;
    // TRACEC(TRACE_REFCOUNT, "Reference to object at %p added. Count is now %ld\n", this, count);
    count_mutex.Unlock();
}

void CountedObject::Release()
{
    ASSERT_PTR(this);
    ASSERT(count > 0);
#if DEBUG
    ASSERT_EX(initialized == COUNTED_MAGIC_NUMBER,
	      "Object at %p magic %lx\n", this, initialized);
#endif
    count_mutex.Lock();
    if (--count == 0)
    {
	TRACEC(TRACE_REFCOUNT, "Count reduced to zero at %p, called by %p\n",
	       this, CALLED_BY);
	count_mutex.Unlock();
	delete this;
	TRACEC(TRACE_REFCOUNT, "Delete this done.\n");
    }
    else
    {
	// TRACEC(TRACE_REFCOUNT, "Reference to object at %p released. Count is now %ld\n", this, count);
	count_mutex.Unlock();
    }
}

#if DEBUG
void CountedObject::AssertValid() const
{
    ASSERT_PTR(this);
    ASSERT(initialized == COUNTED_MAGIC_NUMBER);
}

bool CountedObject::IsValid() const
{
    ASSERT_PTR(this);
    return (initialized == COUNTED_MAGIC_NUMBER);
}
#endif
