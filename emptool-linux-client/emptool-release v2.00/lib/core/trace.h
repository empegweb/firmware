/* trace.h
 *
 * Debug tracing
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.55 01-Apr-2003 18:52 rob:)
 */

#ifndef TRACE_H
#define TRACE_H 1

#include <stdarg.h>

#ifndef CONFIG_H
#include "config.h"
#endif

#ifdef __cplusplus
#ifndef DNEW_H
#include "dnew.h"
#endif // !DNEW_H
#endif // __cplusplus

// Define some macros to keep platform specific stuff isolated.
#ifdef __GNUC__
#define TRACE_PRINTF_ATTRIBUTE(FORMAT_INDEX, FIRST_VARARG) __attribute__((format(printf, FORMAT_INDEX, FIRST_VARARG)))
#else
#define TRACE_PRINTF_ATTRIBUTE(FORMAT_INDEX, FIRST_VARARG)
#endif
    
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
    static inline void empeg_nopc(int flag, const char *fmt, ...) { UNUSED(flag); UNUSED(fmt); }
    static inline void empeg_nop(const char *fmt, ...) { UNUSED(fmt); }

    void empeg_trace(const char *file, int line, const char *, ...) TRACE_PRINTF_ATTRIBUTE(3, 4);
    void empeg_trace_sleep(const char *file, int line, const char *, ...) TRACE_PRINTF_ATTRIBUTE(3, 4);    
    void empeg_tracev(const char *file, int line, const char*, va_list);
    void empeg_trace_hex(const char *file, int line, const char *m, const void *b, const void *e);
    void empeg_release_trace(const char *file, int line, const char *, ...) TRACE_PRINTF_ATTRIBUTE(3, 4);
    
    void empeg_warn(const char *file, int line, const char *, ...) TRACE_PRINTF_ATTRIBUTE(3, 4);
    void empeg_error(const char *file, int line, const char *, ...) TRACE_PRINTF_ATTRIBUTE(3, 4);

    void empeg_assert(const char *file, int line, const char *expr, void *called_by);
    void empeg_assert_fn(const char *file, int line, const char *expr, void *called_by, const char *fn);
    void empeg_assert_pre(const char *file, int line, const char *expr, void *called_by, const char *fn);
    void empeg_assert_post(const char *file, int line, const char *expr, void *called_by, const char *fn);
    void empeg_assert_ex(const char *file, int line, const char *expr, void *called_by, const char *fmt, ...) TRACE_PRINTF_ATTRIBUTE(5, 6);
    
    void empeg_abort(void);
    int IsValidPointer(const void *p);
    void RegisterAbortCleanUpHandler(void (*)(void));
#ifdef __cplusplus
} // extern "C"
#endif

    
#define NOP do {} while (0)

/* Decide whether to do our assertions or tracing. For MFC applications, afx.h
 * decides for us, and provides all the necessary macros.
 */
#if !defined(_MFC) && !defined(__AFX_H__)
# if DEBUG == 0
#  undef  EMPEG_ENABLE_ASSERT
#  undef  EMPEG_ENABLE_TRACE
# elif DEBUG == 1
#  define EMPEG_ENABLE_ASSERT
#  undef  EMPEG_ENABLE_TRACE
# else /* DEBUG >= 2 */
#  define EMPEG_ENABLE_ASSERT
#  define EMPEG_ENABLE_TRACE
# endif
#endif

#if 0
/* Set up various format strings */
#ifdef ARCH_EMPEG
# define EFMT "E !! %-18s %3d: "
# define TFMT "E %-14s %3d (%5ld): "
# define WFMT "E #W %-11s %3d (%5ld): "
# define AFMT "E #! %-11s %3d (%5ld): "
#else
# define EFMT "P !! %-18s %3d: "
# define TFMT "P %-14s %3d (%5ld): "
# define WFMT "P #W %-11s %3d (%5ld): "
# define AFMT "P #! %-11s %3d (%5ld): "
#endif
#endif

# ifdef __GNUC__
#  define CALLED_BY __builtin_return_address(0)
# else
#  define CALLED_BY ((void*)0xbadf00d)
# endif


/* Debug tracing support */
#ifdef EMPEG_ENABLE_TRACE

#define TRACEX(A) empeg_trace A
#define TRACEX_SLEEP(A) empeg_trace_sleep A

# ifdef __GNUC__ /* Only gcc has varargs macros */
#  define TRACE(A, B...) TRACEX((__FILE__, __LINE__, A , ## B))
#  define TRACEC(X, A, B...) do { if (X) TRACEX((__FILE__, __LINE__, A , ##B)); } while(0)
#  define TRACE_SLEEP(A, B...) TRACEX_SLEEP((__FILE__, __LINE__, A , ## B))
#  define TRACEC_SLEEP(X, A, B...) do { if (X) TRACEX_SLEEP((__FILE__, __LINE__, A , ##B)); } while(0)
#  define TRACEV(A, B...) empeg_tracev(__FILE__, __LINE__, A , ##B)
#  define WARN(A, B...) empeg_warn(__FILE__, __LINE__, A , ## B)
# endif

# define TRACE_HEX(A, B, E) empeg_trace_hex(__FILE__, __LINE__, A, B, E)

#endif // EMPEG_ENABLE_TRACE

/* We always support release tracing on gcc. */
#ifdef __GNUC__
#  define RTRACE(A, B...) empeg_release_trace(__FILE__, __LINE__, A , ## B)
#  define RTRACEC(X, A, B...) do { if (X) empeg_release_trace(__FILE__, __LINE__, A, ##B); } while(0)
#endif

/* Error reporting support */
#ifdef __GNUC__
# define ERROR(A, B...) empeg_error(__FILE__, __LINE__, A , ## B)
#endif

/* Why do we have to go through this? */
#define STRIZE2(A) #A
#define STRIZE(A) STRIZE2(A)

/* Assertion support */

#ifdef EMPEG_ENABLE_ASSERT

#ifdef WIN32
# include <assert.h>
# define ASSERT(A) assert(A)
#endif

# ifndef ASSERT
#  define ASSERT(A) do { if (!(A)) { empeg_assert(__FILE__, __LINE__, STRIZE(A), CALLED_BY); } } while(0)
# endif

# define VERIFY(A) ASSERT(A)
# define ASSERT2(F, A) do { if (!(A)) { empeg_assert_fn(__FILE__, __LINE__, STRIZE(A), CALLED_BY, F); } } while(0)
# define ASSERT_PRE(F, A) do { if (!(A)) { empeg_assert_pre(__FILE__, __LINE__, STRIZE(A), CALLED_BY, F); } } while(0)
# define ASSERT_POST(F, A) do { if (!(A)) { empeg_assert_post(__FILE__, __LINE__, STRIZE(A), CALLED_BY, F); } } while(0)

# ifdef __GNUC__ /* Only gcc has varargs macros */
#  define ASSERT_EX(A, B, X...) do { if (!(A)) { empeg_assert_ex(__FILE__, __LINE__, STRIZE(A), CALLED_BY, B , X); } } while(0)
# endif

#endif

/* If there's no tracing (or asserting) */

#ifndef TRACE
# ifdef __GNUC__
#  define TRACE(A, B...) NOP
# else
#  define TRACE empeg_nop
# endif
#endif
#ifndef TRACEC
# ifdef __GNUC__
#  define TRACEC(X, A, B...) NOP
# else
#  define TRACEC empeg_nopc
# endif
#endif
#ifndef TRACE_SLEEP
# ifdef __GNUC__
#  define TRACE_SLEEP(A, B...) NOP
# else
#  define TRACE_SLEEP empeg_nop
# endif
#endif
#ifndef TRACEC_SLEEP
# ifdef __GNUC__
#  define TRACEC_SLEEP(X, A, B...) NOP
# else
#  define TRACEC_SLEEP empeg_nopc
# endif
#endif
#ifndef TRACE_HEX
# define TRACE_HEX(A, B, E) NOP
#endif
#ifndef TRACEV
# define TRACEV(A,B) NOP
#endif

#ifndef TRACEX
# define TRACEX(A) NOP
#endif
#ifndef TRACEX_SLEEP
# define TRACEX_SLEEP(A) NOP
#endif
#ifndef ASSERT
# define ASSERT(A) NOP
#endif
#ifndef VERIFY
# define VERIFY(A) ((void)(A))
#endif
#ifndef ASSERT2
# define ASSERT2(F, A) NOP
#endif
#ifndef ASSERT_PRE
# define ASSERT_PRE(F, A) NOP
#endif
#ifndef ASSERT_POST
# define ASSERT_POST(F, A) NOP
#endif
#ifndef ASSERT_EX
# ifdef __GNUC__
#  define ASSERT_EX(X, A, B...)
# else
#  define ASSERT_EX empeg_nopc
# endif
#endif
#ifndef WARN
# ifdef __GNUC__
#  define WARN(A, B...) NOP
# else
#  define WARN empeg_nop
# endif
#endif // __GNUC__


/* Other debugging functions which depend on the above */

#ifndef ASSERT_VALID /* Again, MFC comes with one of these in afx.h */
# ifdef WIN32
#  define ASSERT_VALID(A) ASSERT( (A) != 0 )
# else
#  define ASSERT_VALID(A) ASSERT_EX(IsValidPointer(A), #A "=%p\n", (A))
# endif
#endif

#ifdef __GNUC__ /* More varargs macros */
# define ASSERT_VALID_EX(A, B, X...) ASSERT_EX(IsValidPointer(A), B , X)
# define ALIGNED(T, A) ( ((unsigned long)A) % (__alignof__(T) ? __alignof__(T) : 1) == 0)
# define ASSERT_TPTR(T, A) ASSERT_EX(IsValidPointer(A) && ALIGNED(T, A), #A "=%p\n", (A))
# define ASSERT_PTR(A) ASSERT_TPTR(typeof(*(A)), (A))

// Older versions of GCC barf on using __alignof__ on templated types.
# define ASSERT_TEMPLATED_PTR(A) ASSERT_EX(IsValidPointer(A) && ALIGNED(T, A), #A "=%p\n", (A))

# define ASSERT_PTR_EX(A, B, X...) ASSERT_EX(IsValidPointer(A), B , X)
#else /* !gcc */
# define ASSERT_PTR(A) ASSERT(A)
# define ASSERT_TPTR(T, A) ASSERT(A)
#endif 

#ifdef WIN32
# define ABORT() assert(0)
#else
# define ABORT() empeg_abort()
#endif

#if defined(__cplusplus)
#include "magic.h"
#include "static_check.h"

#ifndef WIN32
class Entering
{
    const char *m_routine;
    DISALLOW_COPYING(Entering);
 public:
    explicit Entering(const char *s) : m_routine(s)
	{ TRACE("Entering %s\n", s); }
    ~Entering()
	{ TRACE("Leaving %s\n", m_routine); }
};

/** A debugging class to demonstrate exactly where in a sequence of member
 * constructors something is falling over and/or corrupting the arena.
 */
class Constructing
{
    int *m_p;
    int m_i;
    DISALLOW_COPYING(Constructing);
public:
    Constructing() : m_p(new int), m_i(0)
    {
	TRACE("Constructing1\n");
	empeg_walk();
	m_i=0x6b6e6974;
	*m_p=0x6b6e6f74;
	TRACE("Constructing2\n");
	empeg_walk();
    }
    Constructing( int i ) : m_p(new int), m_i(i)
    {
	TRACE("Constructing(%d)\n", i );
    }
    ~Constructing()
    {
	delete m_p;
    }
};

/** A debugging class to demonstrate exactly where in a sequence of member
 * destructors something is falling over and/or corrupting the arena.
 */
class Destructing
{
 public:
    ~Destructing()
     {
	 TRACE("Destructing\n");
     }
};
#endif

#endif // __cplusplus

#endif // TRACE_H
