/* dnew.h
 *
 * Memory leak detecting C++ operator new. Ideally suited for use with
 * Electric Fence.
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *
 * Originally (C) 1997-2000 Mike Crowe.
 *
 * This source file is non-exclusively licensed for use by Empeg Ltd for however
 * it sees fit but without warranty.
 *
 * empeg ltd in turn licenses it to you under the GNU General Public Licence
 * (see file COPYING), unless you possess an alternative written licence from
 * empeg ltd.
 *
 * (:Empeg Source Release 1.22 01-Apr-2003 18:52 rob:)
 */
 
#ifndef DNEW_H
#define DNEW_H 1

#include <stdlib.h>
#ifdef __cplusplus
#include <memory>
#endif

#ifdef WIN32
 #ifdef _DEBUG
  #ifdef _MFC_VER
   #define NEW_NOTHROW DEBUG_NEW
   #define NEW DEBUG_NEW
   #define NEW_HARMLESS_LEAK DEBUG_NEW
  #else
   #define NEW_NOTHROW new
   #define NEW new
   #define NEW_HARMLESS_LEAK new
  #endif // _MFC_VER
  #define PLACEMENT_NEW(x) new(x)
  #define EMALLOC(N) _malloc_dbg(N, _NORMAL_BLOCK, __FILE__, __LINE__)
  #define EREALLOC(P, N) _realloc_dbg(P, N, _NORMAL_BLOCK, __FILE__, __LINE__)
  #define ECALLOC(N, M) _calloc_dbg(N, M, _NORMAL_BLOCK, __FILE__, __LINE__)
  #define EFREE(P) _free_dbg(P, _NORMAL_BLOCK)
 #else // _DEBUG
  #define NEW_NOTHROW new
  #define NEW_HARMLESS_LEAK new
  #define NEW new
  #define EMALLOC malloc
  #define EREALLOC realloc
  #define ECALLOC calloc
  #define EFREE free
  #define PLACEMENT_NEW(x) new(x)
 #endif // _DEBUG
 inline void empeg_walk() {}
#else

#ifdef __EXCEPTIONS
#define THROW(s) throw(s)
#define THROW_NONE throw()
#define DO_THROW(s) throw s
#else
#define THROW(s)
#define THROW_NONE
#define DO_THROW(s) ASSERT(false)
#endif

#ifdef DEBUG_MEMORY
#include <new>

void *operator new(size_t s) THROW(std::bad_alloc);
void *operator new(size_t, const char *, int, void *caller = NULL) THROW(std::bad_alloc);
void *operator new[](size_t s) THROW(std::bad_alloc);
void *operator new[](size_t, const char *, int, void *caller = NULL) THROW(std::bad_alloc);

void *operator new(size_t, nothrow_t) THROW_NONE;
void *operator new(size_t, nothrow_t, const char *, int, void *caller = NULL) THROW_NONE;
void *operator new[](size_t, nothrow_t) THROW_NONE;
void *operator new[](size_t, nothrow_t, const char *, int, void *caller = NULL) THROW_NONE;
void operator delete(void *) THROW_NONE;
void operator delete[](void *) THROW_NONE;
void dumpleaks();

void *empeg_malloc(size_t s, const char *, int, void *);
void empeg_free(void *p, const char *, int, void *);
void *empeg_calloc(size_t s, const char *, int, void *);
void *empeg_realloc(void *p, size_t s, const char *, int, void *);

#define NEW new(__FILE__, __LINE__)
#define NEW_EX(FILE, LINE, CALLER) new((FILE), (LINE), (CALLER))
#define NEW_NOTHROW NEW
#define NEW_HARMLESS_LEAK new(__FILE__ " (will leak)", -49)
#define OPERATOR_NEW(X) operator new((X), __FILE__, __LINE__)
#define OPERATOR_NEW_EX(X, FILE, LINE) operator new(X, (FILE), (LINE))
#define OPERATOR_NEW_NOTHROW(X) operator new(X, __FILE__, __LINE__)
#define PLACEMENT_NEW new
#define EMALLOC(X) empeg_malloc(X, __FILE__, __LINE__, CALLED_BY)
#define EFREE(X) empeg_free(X, __FILE__, __LINE__, CALLED_BY)
#define ECALLOC(X) empeg_calloc(X, __FILE__, __LINE__, CALLED_BY)
#define EREALLOC(X, S) empeg_realloc(X, S, __FILE__, __LINE__, CALLED_BY)

extern int GetCommitBytes();
extern int GetCommitCount();
extern int GetCommitCountEver();

extern void empeg_meminfo();

extern void empeg_walk();

#if DEBUG>=3
// Ugly STL hack.
#define __USE_MALLOC 1
#endif // DEBUG>3

#else
#define NEW new
#define NEW_EX(FILE, LINE, CALLER) new
#define NEW_NOTHROW NEW
#define NEW_HARMLESS_LEAK new
#define OPERATOR_NEW(X) operator new((X))
#define OPERATOR_NEW_EX(X, FILE, LINE) operator new((X))
#define OPERATOR_NEW_NOTHROW(X) operator new(X)
#define PLACEMENT_NEW new

// Nothing should really be using these now.
#define EMALLOC(X) malloc(X)
#define EFREE(X) free(X)
#define ECALLOC(X) calloc(X)
#define EREALLOC(X, S) realloc(X, S)

#ifdef __cplusplus
inline void empeg_walk() {}
#else
#define empeg_walk()
#endif

#endif // DEBUG_MEMORY
#endif // WIN32

#endif

