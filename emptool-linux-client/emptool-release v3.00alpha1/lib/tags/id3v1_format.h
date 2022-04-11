/* id3v1.h
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.9 13-Mar-2003 18:15 rob:)
 */

#ifndef ID3V1_FORMAT_H
#define ID3V1_FORMAT_H 1

#include "packing.h"
#if USE_PACK_HEADERS
#include <pshpack1.h>
#endif

#define ID3V1_FIELD_LENGTH 30

PACKED_PRE struct ID3V1_Tag {
    char signature[3];	// Should be 'TAG'
    char title[ID3V1_FIELD_LENGTH];
    char artist[ID3V1_FIELD_LENGTH];
    char album[ID3V1_FIELD_LENGTH];
    char year[4];
    char comment[ID3V1_FIELD_LENGTH];	// Also includes track number as last byte.
    char genre;
} PACKED_POST;

#if USE_PACK_HEADERS
#include <poppack.h>
#endif

#endif /* ID3V1_FORMAT_H */
