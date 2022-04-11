/* singleton.h
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * Authors:
 *   John Graley <jgraley@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 13-Mar-2003 18:15 rob:)
 *
 * Singleton base class. This provides the following features:
 * - Ensures only one instance can be in existance at a time
 * - Provides a static GetInstance() that checks the object exists
 *
 * How to use:
 *
 * Use "backward template" instantiation on your class. So if it was
 * class Foo {...};
 * now use
 * class Foo : public Singleton<Foo> {...};
 *
 * (:Empeg Source Release 1.5 13-Mar-2003 18:15 rob:)
 */
 
#ifndef SINGLETON_H
#define SINGLETON_H


#include "trace.h"

template<class T>
class Singleton
{
public:
    enum t_lifecycle
    {
	BEFORE_CONSTRUCT = 0,
	EXISTANT = 1,
	AFTER_DESTRUCT = 2
    };

private:
#if DEBUG>0
    static t_lifecycle sm_lifecycle;
#endif
    static T *sm_instance;

    Singleton( const Singleton<T> & ); // Do not allow this operation
    Singleton<T> &operator=( const Singleton<T> & ); // Do not allow this operation
    
protected:
    Singleton()
    {
	// Object should not have been constructed before because it is a singleton
#if DEBUG>0
	ASSERT_EX(sm_lifecycle == BEFORE_CONSTRUCT, "sm_lifecycle=%d\n", sm_lifecycle);
	sm_lifecycle = EXISTANT;
#endif
	sm_instance = static_cast<T *>(this);	
    }
    
    ~Singleton()
    { 
	// Object should have been constructed already
	ASSERT( sm_instance == static_cast<T *>(this) );
#if DEBUG>0
	ASSERT_EX(sm_lifecycle == EXISTANT, "sm_lifecycle=%d\n", sm_lifecycle);
	sm_lifecycle = AFTER_DESTRUCT; // Once we enter this state, ThErE'S NO ESCAPE BWAWAHAHAHAHAHAH!!!!!
#endif
	sm_instance = 0;	
    }
    
#if DEBUG>0
    // This returns the object's status. Use in derived class for eg lazy construction
    static t_lifecycle GetLifecycle()
    {
	return sm_lifecycle;
    }
#endif
    
public:
    // This returns a pointer to the singleton object itself
    static T *GetInstance()
    {
	// Object should have been constructed already (this singleton class does NOT
	// support lazy construction (would be ify with real-time constraints))
#if DEBUG>0
	ASSERT_EX(sm_lifecycle == EXISTANT, "sm_lifecycle=%d\n", sm_lifecycle);
#endif
	return sm_instance;
    }
};

#if DEBUG>0
template<class T>
typename Singleton<T>::t_lifecycle Singleton<T>::sm_lifecycle = BEFORE_CONSTRUCT;
#endif

template<class T>
T *Singleton<T>::sm_instance = 0;

#endif // SINGLETON_H
