/* id3v2.h
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.16.6.1 01-Apr-2003 18:52 rob:)
 */

#ifndef ID3V2_FORMAT_H
#define ID3V2_FORMAT_H 1

#include "types.h"
#include "empeg_endian.h"
#if __BYTE_ORDER != __LITTLE_ENDIAN
#error This is all broken for big endian
#endif

// DON'T pack this -- it's woefully unaligned.
struct ID3V2_Tag {
    char fileIdentifier[3];
    BYTE minor_version;
    BYTE revision;
    BYTE flags;
    UINT32 size;	    // syncsafe
    /* Followed in the file (but not in memory) by optional
     * extended header, then the tags, then an optional
     * extended footer, then some padding.
     */
};

#define ID3V2_FRAME_ID_SIZE (4)

struct ID3V2_Frame {
    char frameID[ID3V2_FRAME_ID_SIZE];	// 4
    UINT32 size;			// +4=8
    UINT16 flags;			// +2=10
    /* Followed by the frame data. */
};

#define ID3V2_VERSION (4)
#define ID3V2_TEXT_ENCODING_ISO8859_1	0
#define ID3V2_TEXT_ENCODING_UNICODE	1

// Convert a "syncsafe" 32-bit integer to the native type.
inline UINT32 ss32_to_cpu(const unsigned char x[4])
{
    return (x[0]<<21)
	|  (x[1]<<14)
	|  (x[2]<<7)
	|  x[3];
}

// Convert a "syncsafe" 24-bit integer to the native type.
inline UINT32 ss24_to_cpu(const unsigned char x[3])
{
    return (x[0]<<14)
	|  (x[1]<<7)
	|  x[2];
}

// Convert a plain 32-bit integer
inline UINT32 p32_to_cpu(const unsigned char x[4])
{
    return (x[0]<<24)
	|  (x[1]<<16)
	|  (x[2]<<8)
	|  x[3];
}

// Convert a plain 24-bit integer
inline UINT32 p24_to_cpu(const unsigned char x[3])
{
    return (x[0]<<16)
	|  (x[1]<<8)
	|  x[2];
}

const char *DescribeTextEncoding(BYTE textEncoding);
#endif /* ID3V2_FORMAT_H */
