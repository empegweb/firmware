/* trace2.h
 *
 * Debug tracing
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.9 01-Apr-2003 18:52 rob:)
 */

#ifndef TRACE2_H
#define TRACE2_H	1

// for ATTRIBUTE
#ifndef CONFIG_H
#include "config.h"
#endif
// for va_list
// check against assumed system header _STDARG_H for gcc
#if defined(WIN32) || !defined(_STDARG_H)
#include "stdarg.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}; // silly xemacs gets confused
#endif

#define CALLED_BY			__builtin_return_address(0)
#define NOP
#define STRIZE(A)			#A

#define CAUSE_FAULT(L) \
	do \
	{ \
		unsigned addr = 0xf0000000U + (L) * 16; \
		*(unsigned *) addr = addr; \
	} while(0)

void RegisterAbortCleanUpHandler(void (*)(void));

// use these two #defines to generate much less code if your binary is getting
// way too big.

// this makes ASSERT cause a page fault rather than printing anything
// small difference, not much gain in using this
#define DODGY_ASSERT			0

// This makes ASSERT_PTR just load the address.
// All this does is makes sure the fault happens before the address is used
// with an offset, e.g a member of a structure.
// This also checks alignment using hardware if alignment handler is turned
// OFF in the the kernel.
// This is a rather big space and clock cycle saver.
#define DODGY_ASSERT_PTR		0

// this always exists
void ABORT(void) ATTRIBUTE((__noreturn__));
void empeg_error(const char *file, unsigned line, const char *fmt, ...) \
    ATTRIBUTE((__format__(printf, 3, 4)));
void do_empeg_cleanup(void);
void empeg_abort_action(void) ATTRIBUTE((__noreturn__));

#define ERROR(A, B...) \
	do { \
		empeg_error(__FILE__, __LINE__, A, ## B); \
	} while(0)

#if DEBUG == 0

# define DECLARE_MAGIC(M)
# define INITIALISE_MAGIC()
# define INVALIDATE_MAGIC()
# define ASSERT_MAGIC()

# define TRACE(A, B...)			NOP
# define TRACEC(C, A, B...)		NOP
# define TRACE_SLEEP(A, B...)		NOP
# define TRACEC_SLEEP(A, B...)		NOP
# define WARN(A, B...)			NOP
# define ASSERT(C)			NOP
# define ASSERT_EX(C, A, B...)		NOP
# define TRACE_HEX(H, B, E)		NOP
# define ASSERT_VALID(P)		NOP
# define ASSERT_VALID_EX(A, B, X...)	NOP
# define ALIGNED(T, A)			NOP
# define ASSERT_TPTR(T, A)		NOP
# define ASSERT_PTR(A)			NOP
# define ASSERT_TEMPLATED_PTR(A)	NOP
# define ASSERT_PTR_EX(A, B, X...)	NOP
# define ASSERT_PRE(A, C)		NOP
# define ASSERT_POST(A, C)		NOP

# define VERIFY(A)			((void)(A))

#else

# define DECLARE_MAGIC(Value) \
	enum MagicType \
	{ \
		MAGIC_VALUE = (Value), \
		MAGIC_INVERSE = (~(Value)), \
	} m_magic
# define INITIALISE_MAGIC()		m_magic = MAGIC_VALUE
# define INVALIDATE_MAGIC()		m_magic = MAGIC_INVERSE
# define ASSERT_MAGIC() \
	ASSERT_EX(m_magic == MAGIC_VALUE, \
		  "magic=%d, expected=%d\n", m_magic, MAGIC_VALUE)

#if DEBUG > 4
int IsValidPointer(const void *p);
#else
#ifdef ARCH_EMPEG
#define IsValidPointer(p) \
	((bool) (p >= (void *) 0x2000000U) && (p < (void *) 0xc0000000U))
#else
#define IsValidPointer(p) \
	((bool) (p >= (void *) 0x8000000U) && (p < (void *) 0xc0000000U))
#endif
#endif

void empeg_trace(const char *file, unsigned line, const char *fmt, ...) \
    ATTRIBUTE((__format__(printf, 3, 4)));
void empeg_tracev(const char *file, unsigned line,
		  const char *fmt, va_list args);
void empeg_trace_sleep(const char *file, unsigned line, const char *fmt, ...) \
    ATTRIBUTE((__format__(printf, 3, 4)));
void empeg_warn(const char *file, unsigned line, const char *fmt, ...) \
    ATTRIBUTE((__format__(printf, 3, 4)));
void empeg_assert(const char *file, unsigned line, void *called_by,
		  const char *assertion) ATTRIBUTE((__noreturn__));
void empeg_assert_ex(const char *file, unsigned line, void *called_by,
		     const char *assertion, const char *fmt, ...) \
    ATTRIBUTE((__format__(printf, 5, 6), __noreturn__));
void empeg_trace_hex(const char *file, unsigned line, const char *header,
		     const void *begin, const void *end);

# define TRACE(A, B...) \
	do { \
		empeg_trace(__FILE__, __LINE__, A, ## B); \
	} while(0)
# define TRACEC(C, A, B...) \
	do { \
		if(C) TRACE(A, ## B); \
	} while(0)
# define TRACE_SLEEP(A, B...) \
	do { \
		empeg_trace_sleep(__FILE__, __LINE__, A, ## B); \
	} while(0)
# define TRACEC_SLEEP(C, A, B...) \
	do { \
		if(C) TRACE_SLEEP(A, ## B); \
	} while(0)
# define WARN(A, B...) \
	do { \
		empeg_warn(__FILE__, __LINE__, A, ## B); \
	} while(0)

# if DODGY_ASSERT
#  define ASSERT(C)		do { if(!(C)) CAUSE_FAULT(__LINE__); } while(0)
#  define ASSERT_EX(C, A, B...)	do { if(!(C)) CAUSE_FAULT(__LINE__); } while(0)
# else
#  define ASSERT(C) \
	do { \
		if(!(C)) empeg_assert(__FILE__, __LINE__, CALLED_BY, #C ); \
	} while(0)
#  define ASSERT_EX(C, A, B...) \
	do { \
		if(!(C)) empeg_assert_ex(__FILE__, __LINE__, \
					 CALLED_BY, #C, A, ## B); \
	} while(0)
# endif

# define TRACE_HEX(H, B, E) \
	do { \
		empeg_trace_hex(__FILE__, __LINE__, H, B, E); \
	} while(0)

# define VERIFY(A)			ASSERT(A)

# define ASSERT_PRE(A, C)		ASSERT_EX(C, A)
# define ASSERT_POST(A, C)		ASSERT_EX(C, A)

# define ALIGNED(T, A) \
	( ((unsigned long)A) % (__alignof__(T) ? __alignof__(T) : 1) == 0)

# if DODGY_ASSERT_PTR

#  define TestValidPointer(P) \
	do { \
		(void) *(volatile const char *) (P); \
	} while(0)
#  define ASSERT_VALID(A)		TestValidPointer(A)
#  define ASSERT_VALID_EX(A, B, X...)	TestValidPointer(A)
#  define ASSERT_TPTR(T, A)		TestValidPointer(A)
#  define ASSERT_PTR(A)			TestValidPointer(A)
#  define ASSERT_TEMPLATED_PTR(A)	TestValidPointer(A)
#  define ASSERT_PTR_EX(A, B, X...)	TestValidPointer(A)

# else

#  define ASSERT_VALID(A) \
	ASSERT_EX(IsValidPointer(A), #A "=%p\n", (A))
#  define ASSERT_VALID_EX(A, B, X...)	ASSERT_EX(IsValidPointer(A), B , X)
#  define ASSERT_TPTR(T, A) \
	ASSERT_EX(IsValidPointer(A) && ALIGNED(T, A), #A "=%p\n", (A))
#  define ASSERT_PTR(A)			ASSERT_TPTR(typeof(*(A)), (A))
#  define ASSERT_TEMPLATED_PTR(A) \
	ASSERT_EX(IsValidPointer(A) && ALIGNED(T, A), #A "=%p\n", (A))
#  define ASSERT_PTR_EX(A, B, X...)	ASSERT_EX(IsValidPointer(A), B , X)

# endif

#endif
	    
#if 0
{ // silly xemacs gets confused
#endif
#if defined(__cplusplus)
}; // extern "C"
#endif

#endif
