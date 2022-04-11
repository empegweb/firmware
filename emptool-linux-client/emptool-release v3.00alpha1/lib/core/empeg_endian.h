/* empeg_endian.h
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.15 13-Mar-2003 18:15 rob:)
 *
 */

#ifndef EMPEG_ENDIAN_H
#define EMPEG_ENDIAN_H		1

#if defined(__linux__)
// yes this is really a glibc, not linux thing

# include <endian.h>
# define EMPEG_HAVE_LONG_LONG_PAIR	1

#elif defined(WIN32)
// define all the things endian.h would have defined

# define __LITTLE_ENDIAN		1234
# define __BIG_ENDIAN			4321
# define __PDP_ENDIAN			3412
# define __FLOAT_WORD_ORDER		__BYTE_ORDER
# define EMPEG_HAVE_LONG_LONG_PAIR	0

// We'll assume all WIN32 machines are little endian.
// This is probably a good assumption for WIN32.
# define __BYTE_ORDER			__LITTLE_ENDIAN

# define BYTE_ORDER			__BYTE_ORDER
# define LITTLE_ENDIAN			__LITTLE_ENDIAN
# define BIG_ENDIAN			__BIG_ENDIAN

#elif defined(ECOS)

# include <sys/types.h>
# define __BYTE_ORDER			BYTE_ORDER
# define __LITTLE_ENDIAN		LITTLE_ENDIAN
# define __BIG_ENDIAN			BIG_ENDIAN
# define __FLOAT_WORD_ORDER		BYTE_ORDER
# define EMPEG_HAVE_LONG_LONG_PAIR	0

#else // !(__linux__ or WIN32 or eCos)

# error Need a byte order definition for this architecture

#endif

#if !EMPEG_HAVE_LONG_LONG_PAIR
# if __BYTE_ORDER == __LITTLE_ENDIAN
#  define __LONG_LONG_PAIR(HI, LO)	LO, HI
# elif __BYTE_ORDER == __BIG_ENDIAN
#  define __LONG_LONG_PAIR(HI, LO)	HI, LO
# endif
#endif
#undef EMPEG_HAVE_LONG_LONG_PAIR

#include "types.h"

#if defined(__cplusplus)
#define INLINE inline
#else
#define INLINE static
#endif

#if defined(__cplusplus)
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
    #define le64_to_cpu(x) (x)
    inline UINT32 le32_to_cpu(UINT32 x) { return x; }
    inline void le32_to_cpu(UINT32 *) {}
    inline UINT32 be32_to_cpu(UINT32 x) { return endianness_swap32(x); }
    inline void be32_to_cpu(UINT32 *x) { *x = endianness_swap32(*x); }
    inline UINT16 le16_to_cpu(UINT16 x) { return x; }
    inline void le16_to_cpu(UINT16 *) {}
    inline UINT16 be16_to_cpu(UINT16 x) { return endianness_swap16(x); }
    inline void be16_to_cpu(UINT16 *x) { *x = endianness_swap32(*x); }

    inline UINT32 cpu_to_le32(UINT32 x) { return x; }
    inline void cpu_to_le32(UINT32 *) {}
    inline UINT32 cpu_to_be32(UINT32 x) { return endianness_swap32(x); }
    inline void cpu_to_be32(UINT32 *x) { *x = endianness_swap32(*x); }
    inline UINT16 cpu_to_le16(UINT16 x) { return x; }
    inline void cpu_to_le16(UINT16 *) {}
    inline UINT16 cpu_to_be16(UINT16 x) { return endianness_swap16(x); }
    inline void cpu_to_be16(UINT16 *x) { *x = endianness_swap32(*x); }

#elif __BYTE_ORDER == __BIG_ENDIAN
    inline UINT32 le32_to_cpu(UINT32 x) { return endianness_swap32(x); }
    inline void le32_to_cpu(UINT32 *x) { *x = endianness_swap32(*x); }
    inline UINT32 be32_to_cpu(UINT32 x) { return x; }
    inline void be32_to_cpu(UINT32 *) {}
    inline UINT16 le16_to_cpu(UINT16 x) { return endianness_swap16(x); }
    inline void le16_to_cpu(UINT16 *x) { *x = endianness_swap32(*x); }
    inline UINT16 be16_to_cpu(UINT16 x) { return x; }
    inline void be16_to_cpu(UINT16 *) {}

    inline UINT32 cpu_to_le32(UINT32 x) { return endianness_swap32(x); }
    inline void cpu_to_le32(UINT32 *x) { *x = endianness_swap32(*x); }
    inline UINT32 cpu_to_be32(UINT32 x) { return x; }
    inline void cpu_to_be32(UINT32 *) {}
    inline UINT16 cpu_to_le16(UINT16 x) { return endianness_swap16(x); }
    inline void cpu_to_le16(UINT16 *x) { *x = endianness_swap32(*x); }
    inline UINT16 cpu_to_be16(UINT16 x) { return x; }
    inline void cpu_to_be16(UINT16 *) {}
#else
#error Unknown __BYTE_ORDER
#endif
#endif

#endif // EMPEG_ENDIAN_H
