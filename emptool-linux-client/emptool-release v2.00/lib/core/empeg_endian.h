/* empeg_endian.h
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.10 01-Apr-2003 18:52 rob:)
 *
 */

#ifndef EMPEG_ENDIAN_H
#define EMPEG_ENDIAN_H		1

#if defined(__linux__)
// yes this is really a glibc, not linux thing
# include <endian.h>

#elif defined(WIN32)
// define all the things endian.h would have defined
# define __LITTLE_ENDIAN		1234
# define __BIG_ENDIAN			4321
# define __PDP_ENDIAN			3412

# ifndef __FLOAT_WORD_ORDER
#  define __FLOAT_WORD_ORDER		__BYTE_ORDER
# endif

# if __BYTE_ORDER == __LITTLE_ENDIAN
#  define __LONG_LONG_PAIR(HI, LO)	LO, HI
# elif __BYTE_ORDER == __BIG_ENDIAN
#  define __LONG_LONG_PAIR(HI, LO)	HI, LO
# endif

// We'll assume all WIN32 machines are little endian.
// This is probably a good assumption.
# define __BYTE_ORDER			__LITTLE_ENDIAN

#endif // __linux__ or WIN32

#include "types.h"

/// @todo C variant?
inline UINT32 endianness_swap32(UINT32 x)
{
    return ((x & 0xff000000) >> 24) |
	   ((x & 0x00ff0000) >> 8) |
	   ((x & 0x0000ff00) << 8) |
	   ((x & 0x000000ff) << 24);
}

inline UINT32 endianness_swap16(UINT32 x)
{
    return ((x & 0xff00) >> 8) |
	   ((x & 0x00ff) << 8);
}

#if __BYTE_ORDER == __LITTLE_ENDIAN
    #define le32_to_cpu(x) (x)
    #define le64_to_cpu(x) (x)
    #define be32_to_cpu(x) (endianness_swap32(x))
#elif __BYTE_ORDER == __BIG_ENDIAN
    #define le32_to_cpu(x) (endianness_swap32(x))
    #define be32_to_cpu(x) (x)
#else
#error Unknown __BYTE_ORDER
#endif

#endif // EMPEG_ENDIAN_H
