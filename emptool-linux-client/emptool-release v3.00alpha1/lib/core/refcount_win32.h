/* refcount_win32.h
 *   Implementation of reference counted pointer and object functionality for
 *   Win32 platforms.
 *
 * (C) 1999-2000 empeg ltd
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *
 * (:Empeg Source Release 1.10 13-Mar-2003 18:15 rob:)
 */

#ifndef REFCOUNT_H
#define REFCOUNT_H 1

class CountedObject;

#ifndef EMPEG_ERROR_H
#include "empeg_error.h"
#endif

#define ASSERT_VALID_REFPTR(p) do { ASSERT((p).GetPointer()); (p).AssertValid(); } while(0)

template<class T>
class CountedPointer
{
    mutable T *ptr;
	
public:
    CountedPointer()
	: ptr(NULL) {}

    // This function should only be called when the object has been
    // created via malloc or somehow and the constructor has not been
    // called. Failing to call this in such a circumstances invokes
    // undefined behaviour :-)
    void Construct()
    {
	ptr = NULL;
    }
		
    // This constructor allows CountedPointer objects to obey class
    // hierarchies like pointers do.
    template<class U>
    CountedPointer(const CountedPointer<U> &cp)
    {
	ptr = cp.GetPointer();
	if (ptr)
	    ptr->AddRef();
    }

    CountedPointer(const CountedPointer<T> &cp)
    {
	ptr = cp.ptr;
	if (ptr)
	    ptr->AddRef();
    }

    // Use this static function to downcast a CountedPointer to
    // something more specific. I can't think of a way to do this
    // without having a function call :-(
    template<class U>
    static CountedPointer<T> DownCast(const CountedPointer<U> &cp)
    {
	if (!cp.IsValid())
	    return CountedPointer<T>();

	return CountedPointer<T>(static_cast<T *>(cp.GetPointer()));
    }
    
    explicit CountedPointer(T *p)
	: ptr(p)
    {
	ASSERT(ptr);
	ASSERT(ptr->IsValid());
	ptr->AddRef();
    }
	
    ~CountedPointer()
    {
#ifdef _DEBUG
	if (ptr)
	{
	    ASSERT(ptr);
	    ASSERT(ptr->IsValid());
	}
#endif
	ReleaseIfNecessary();
    }
    CountedPointer &operator=(const CountedPointer &cp)
    {
	// Protect against self assigning
	ASSERT(this);
	ASSERT(&cp);
		
	if (&cp != this)
	{
	    ReleaseIfNecessary();
	    ptr = cp.ptr;
	    if (ptr)
	    {
		ASSERT(ptr->IsValid());
		ptr->AddRef();
	    }
	}
	return *this;
    }
    void ReleaseIfNecessary()
    {
	if (ptr)
	{
	    ASSERT(ptr);
	    ASSERT(ptr->IsValid());
	    ptr->Release();
	    ptr = NULL;
	}
	ASSERT(ptr == NULL);
    }

    bool operator==(const CountedPointer &rhs) const
    {
	return (GetPointer() == rhs.GetPointer());
    }

    const T *operator->() const
    {
//	ASSERT_VALID_REFPTR(*this);
	return ptr;
    }
	
    T *operator->()
    {
	ASSERT_VALID_REFPTR(*this);
	return ptr;
    }

    const T &operator*() const
    {
//	ASSERT_VALID_REFPTR(*this);
	return *ptr;
    }

    T &operator*()
    {
	ASSERT_VALID_REFPTR(*this);
	return *ptr;
    }

    T *GetPointer() const
    {
	if (ptr)
	    ASSERT(ptr->IsValid());
	return ptr;
    }
	
    bool IsValid() const
    {
	return (ptr != NULL);
    }
	
    void Invalidate()
    {
	ReleaseIfNecessary();
    }

    void AssertValid()
    {
	ASSERT(ptr);
	ASSERT(ptr->IsValid());
    }
};

#define COUNTED_MAGIC_NUMBER 0xf00fee11

class CountedObject
{
#ifdef _DEBUG
    unsigned long initialized;
#endif
    volatile long count;
	
    // Deliberately not implemented.
    void operator=(const CountedObject &);
    CountedObject(const CountedObject &);
	
protected:
    CountedObject()
	: count(0)
    {
#ifdef _DEBUG
	initialized = COUNTED_MAGIC_NUMBER;
#endif
    }
    virtual ~CountedObject()
    {
	ASSERT(this);
	ASSERT(initialized == COUNTED_MAGIC_NUMBER);
	ASSERT(count == 0);
#ifdef _DEBUG
	initialized = 0x00010001;
#endif
    }
    
public:
    void AddRef()
    {
	ASSERT(this);
	ASSERT(initialized == COUNTED_MAGIC_NUMBER);
	InterlockedIncrement((long *)&count);
    }
    void Release()
    {
	ASSERT(this);
	ASSERT(initialized == COUNTED_MAGIC_NUMBER);
	if(InterlockedDecrement((long *)&count) == 0)
	{
	    delete this;
	}
    }
#ifdef _DEBUG
    void AssertValid() const
    {
	ASSERT(this);
	ASSERT(initialized == COUNTED_MAGIC_NUMBER);
    }

    bool IsValid() const
    {
	return (initialized == COUNTED_MAGIC_NUMBER);
    }
#endif
};
	    
#endif
