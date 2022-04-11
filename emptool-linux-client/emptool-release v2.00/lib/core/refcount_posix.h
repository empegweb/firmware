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
 * (:Empeg Source Release 1.14 01-Apr-2003 18:52 rob:)
 */

#ifndef REFCOUNT_H
#define REFCOUNT_H 1

#ifndef TRACE_H
#include "trace.h"
#endif

#ifndef MUTEX_H
#include "mutex.h"
#endif

class CountedObject;

#define ASSERT_VALID_REFPTR(p) do { ASSERT_PTR(&p); ASSERT_VALID((p).GetPointer()); (p).AssertValid(); } while(0)

#define TRACE_REFCOUNT 0

template<class T>
class CountedPointer
{
#if DEBUG
    enum { MAGIC_VALUE = 0xa0b1c2d3 };
    unsigned long magic;
#endif
    mutable T *ptr;

public:
    CountedPointer()
	: ptr(NULL)
    {
#if DEBUG
	magic = MAGIC_VALUE;
#endif
    }

    // This function should only be called when the object has been
    // created via malloc or somehow and the constructor has not been
    // called. Failing to call this in such a circumstances invokes
    // undefined behaviour :-)
    void Construct()
    {
	// Earlier versions of GCC barf on this line, it's only for debug
	// purposes so who cares.
	ASSERT_TEMPLATED_PTR(this);
#if DEBUG
	magic = MAGIC_VALUE;
#endif
    }

    CountedPointer(const CountedPointer<T> &cp)
    {
#if DEBUG
	magic = MAGIC_VALUE;
#endif
	ASSERT_VALID(&cp);
	cp.AssertMagic();
	ptr = cp.ptr;
	if (ptr)
	{
	    ASSERT_VALID(ptr);
#if DEBUG
	    ASSERT(ptr->IsValid());
#endif
	    ptr->AddRef();
	}
    }

    // This constructor allows CountedPointer objects to obey class
    // hierarchies like pointers do.
    template<class U>
    CountedPointer(const CountedPointer<U> &cp)
    {
#if DEBUG
	magic = MAGIC_VALUE;
#endif
	(void)(ptr == cp.GetPointer()); // won't compile unless T<:U or T:>U

	ASSERT_VALID(&cp);
	cp.AssertMagic();
	ptr = cp.GetPointer();
	if (ptr)
	{
	    ASSERT_VALID(ptr);
#if DEBUG
	    ASSERT(ptr->IsValid());
#endif
	    ptr->AddRef();
	}
    }

    // Use this static function to downcast a CountedPointer to
    // something more specific. I can't think of a way to do this
    // without having a function call :-(
    template<class U>
    static CountedPointer<T> DownCast(const CountedPointer<U> &cp)
    {
	ASSERT_VALID(&cp);
	cp.AssertMagic();
	ASSERT_VALID_REFPTR(cp);
	return CountedPointer<T>(static_cast<T *>(cp.GetPointer()));
    }

    explicit CountedPointer(T *p)
	: ptr(p)
    {
#if DEBUG
	magic = MAGIC_VALUE;
#endif
	ASSERT_VALID(ptr);
#if DEBUG
	ASSERT(ptr->IsValid());
#endif
	ptr->AddRef();
    }

    ~CountedPointer()
    {
//	TRACEC(TRACE_REFCOUNT, "Reference pointer going away pointing to %p\n", ptr);
	ASSERT_TEMPLATED_PTR(this);
	AssertMagic();
#if DEBUG
	if (ptr)
	{
	    ASSERT_VALID(ptr);
	    ASSERT(ptr->IsValid());
	}
#endif
	ReleaseIfNecessary();
#if DEBUG
	magic = ~MAGIC_VALUE;
	ptr = NULL;
#endif
    }
    CountedPointer &operator=(const CountedPointer &cp)
    {
	ASSERT_TEMPLATED_PTR(this);
	AssertMagic();
	ASSERT_VALID(&cp);
	cp.AssertMagic();
#if DEBUG
	if (ptr)
	    ASSERT_VALID(ptr);
#endif

	// Protect against self assigning
	if (&cp != this)
	{
	    ReleaseIfNecessary();
	    ptr = cp.ptr;
	    if (ptr)
	    {
		ASSERT_VALID(ptr);
#if DEBUG
		ASSERT_EX(ptr->IsValid(), "ptr=%p\n", ptr);
#endif
		ptr->AddRef();
	    }
	}
	return *this;
    }
    void ReleaseIfNecessary()
    {
	ASSERT_TEMPLATED_PTR(this);
	AssertMagic();
	if (ptr)
	{
	    TRACEC(TRACE_REFCOUNT, "It is necessary to release the object at %p held by %p\n", ptr, this);
	    ASSERT_VALID(ptr);
#if DEBUG
	    ASSERT(ptr->IsValid());
#endif
	    ptr->Release();
	    ptr = NULL;
	}
	ASSERT(ptr == NULL);
    }

    const T *operator->() const
    {
	AssertValid();
	ASSERT_VALID_REFPTR(*this);
	return ptr;
    }

    T *operator->()
    {
	AssertValid();
	ASSERT_VALID_REFPTR(*this);
	return ptr;
    }

    const T &operator*() const
    {
	ASSERT_VALID_REFPTR(*this);
	return *ptr;
    }

    T &operator*()
    {
	ASSERT_VALID_REFPTR(*this);
	return *ptr;
    }

    T *GetPointer() const
    {
	ASSERT_TEMPLATED_PTR(this);
	AssertMagic();
#if DEBUG
	if (ptr)
	    ASSERT(ptr->IsValid());
#endif
	return ptr;
    }

    bool IsValid() const
    {
	ASSERT_TEMPLATED_PTR(this);
	AssertMagic();
	return (ptr != NULL);
    }

    void Invalidate()
    {
	ASSERT_TEMPLATED_PTR(this);
	AssertMagic();
	ReleaseIfNecessary();
    }
    /* pdh 15-Mar-01: removed this, it meant that when a vital function's
     * return type changed from FID to CountedPointer<FidInfo>, everything
     * still compiled but then broke :-(
     *
    operator bool() const
    {
	ASSERT_TEMPLATED_PTR(this);
	AssertMagic();
	return (ptr != NULL);
    }
     */
    void AssertValid() const
    {
	ASSERT_TEMPLATED_PTR(this);
	AssertMagic();
	ASSERT_VALID(ptr);
#if DEBUG
	ASSERT(ptr->IsValid());
#endif
    }

    void AssertMagic() const
    {
#if DEBUG
	ASSERT_EX(magic == MAGIC_VALUE, "magic=0x%lx, should be=0x%x\n", magic, MAGIC_VALUE);
#endif
    }
};

#define COUNTED_MAGIC_NUMBER 0xf00fee11

class CountedObject
{
#if DEBUG > 0
    unsigned long initialized;
#endif
    Mutex count_mutex;
    volatile unsigned long count;

    // Deliberately not implemented.
    void operator=(const CountedObject &);
    CountedObject(const CountedObject &);
    void operator&();

protected:
    CountedObject();
    virtual ~CountedObject();

public:
    void AddRef();
    void Release();

#if DEBUG
    void AssertValid() const;
    bool IsValid() const;
#else
    inline void AssertValid() const { }
    inline bool IsValid() const { return true; }
#endif
};

#endif
