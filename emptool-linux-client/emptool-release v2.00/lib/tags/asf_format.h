/* asf_format.h
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.13 01-Apr-2003 18:52 rob:)
 */

#ifndef ASF_FORMAT_H
#define ASF_FORMAT_H 1

#include "empeg_endian.h"
#if __BYTE_ORDER != __LITTLE_ENDIAN
#error This is all broken for big endian
#endif

#include "packing.h"
#if USE_PACK_HEADERS
#include <pshpack1.h>
#endif

#include "types.h"

PACKED_PRE struct WMA_object {
    GUID guid;
    UINT64 size;
} PACKED_POST;

// {75B22630-668E-11CF-A6D9-00AA0062CE6C}
DEFINE_GUID(GUID_WMA_header_object,
	    0x75b22630, 0x668e, 0x11cf, 0xa6, 0xd9, 0x0, 0xaa, 0x0, 0x62, 0xce, 0x6c);

PACKED_PRE struct WMA_header_object {
    WMA_object o;
    UINT32 number_headers;
    BYTE reserved1; // should be 1
    BYTE reserved2; // should be 2
} PACKED_POST;

// {75B22633-668E-11CF-A6D9-00AA0062CE6C}
DEFINE_GUID(GUID_WMA_content_description_object,
	    0x75b22633, 0x668e, 0x11cf, 0xa6, 0xd9, 0x0, 0xaa, 0x0, 0x62, 0xce, 0x6c);

PACKED_PRE struct WMA_content_description_object {
    WMA_object o;
    UINT16 title_len;
    UINT16 author_len;
    UINT16 copyright_len;
    UINT16 description_len;
    UINT16 rating_len;
    /*
    UTF16CHAR title[title_len/2];
    UTF16CHAR author[author_len/2];
    UTF16CHAR copyright[copyright_len/2];
    UTF16CHAR description[description_len/2];
    UTF16CHAR rating[rating_len/2];
    */
} PACKED_POST;

// {8CABDCA1-A947-11CF-8EE4-00C00C205365}
DEFINE_GUID(GUID_WMA_properties_object,
	    0x8cabdca1, 0xa947, 0x11cf, 0x8e, 0xe4, 0x0, 0xc0, 0xc, 0x20, 0x53, 0x65);

PACKED_PRE struct WMA_properties_object {
    WMA_object o;
    GUID multimedia_stream_id;
    UINT64 total_size;
    UINT64 created;
    UINT64 num_interleave_packets;
    UINT64 play_duration_100ns;
    UINT64 send_duration_100ns;
    UINT64 preroll_ms;
    UINT32 flags;
    UINT32 min_interleave_packet_size;
    UINT32 max_interleave_packet_size;
    UINT32 maximum_bit_rate;
} PACKED_POST;

// {B7DC0791-A9B7-11CF-8EE6-00C00C205365}
DEFINE_GUID(GUID_WMA_stream_properties_object,
	    0xb7dc0791, 0xa9b7, 0x11cf, 0x8e, 0xe6, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65);

PACKED_PRE struct WMA_stream_properties_object {
    WMA_object o;
    GUID stream_type;
    GUID error_correction_type;
    UINT64 offset;
    UINT32 type_specific_len;
    UINT32 error_correction_data_len;
    UINT16 stream_number;
    UINT32 reserved;
    /*
    BYTE type_specific_data[type_specific_len];
    BYTE error_correction_data[error_correction_data_len];
     */
} PACKED_POST;

DEFINE_GUID(GUID_WMA_stream_type_audio,
	    0xf8699e40, 0x5b4d, 0x11cf, 0xa8, 0xfd, 0x00, 0x80, 0x5f, 0x5c, 0x44, 0x2b);

// {5FBF03B5-A92E-11CF-8EE3-00C00C205365}
DEFINE_GUID(GUID_WMA_clock_object,
	    0x5fbf03b5, 0xa92e, 0x11cf, 0x8e, 0xe3, 0x0, 0xc0, 0xc, 0x20, 0x53, 0x65);

// {75B22636-668E-11CF-A6D9-00AA0062CE6C}
DEFINE_GUID(GUID_WMA_data_section_object,
	    0x75b22636, 0x668e, 0x11cf, 0xa6, 0xd9, 0x0, 0xaa, 0x0, 0x62, 0xce, 0x6c);

// {D2D0A440-E307-11D2-97F0-00A0C95EA850}
DEFINE_GUID(GUID_WMA_text_tags,
	    0xD2D0A440, 0xE307, 0x11D2, 0x97, 0xF0, 0x00, 0xA0, 0xC9, 0x5E, 0xA8, 0x50);

/* It seems to be laid out like this:
PACKED_PRE struct GUID_WMA_text_tags {
    WMA_object o;
    UINT16 num_tags;

    PACKED_PRE struct WMA_unknown_string_1 {
	UINT16 tag_size;
	UTF16CHAR tag[tag_size/2];  // null-terminated
	UINT16 unknown;
	UINT16 value_size;
	UTF16CHAR value[value_size/2];	// null-terminated
    } PACKED_POST;

    ASF_unknown_string_1 data[num_tags];
} PACKED_POST;
*/

// {86D15240-311D-11D0-A3A4-00A0C90348F6}
DEFINE_GUID(GUID_WMA_unknown_text_2,
	    0x86D15240, 0x311D, 0x11D0, 0xA3, 0xA4, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6);

/* Suspect that it's laid out like this:
PACKED_PRE struct WMA_unknown_text_2 {
    WMA_object o;
    GUID unknown_guid;
    UINT32 unknown;
    UINT16 num_strings;

    PACKED_PRE struct WMA_unknown_string_2 {
	UINT16 size;
	UTF16CHAR text[size];
    } PACKED_POST;

    WMA_unknown_string_2 strings[num_strings];
} PACKED_POST;
*/

// {7BF875CE-468D-11D1-8D82-006097C9A2B2}
DEFINE_GUID(GUID_WMA_unknown_numbers_1,
	    0x7BF875CE, 0x468D, 0x11D1, 0x8D, 0x82, 0x00, 0x60, 0x97, 0xC9, 0xA2, 0xB2);

PACKED_PRE struct WMA_unknown_numbers_1 {
    WMA_object o;
    UINT16 unknown1;	// almost always seems to be 1
    UINT16 unknown2;	// almost always seems to be 1
    UINT16 unknown3;	// almost always seems to be 0xFC88
    UINT16 unknown4;	// almost always seems to be zero
} PACKED_POST;

// {1EFB1A30-0B62-11D0-A39B-00A0C90348F6}
DEFINE_GUID(GUID_WMA_unknown_drm_1,
	    0x1EFB1A30, 0x0B62, 0x11D0, 0xA3, 0x9B, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6);

/*
PACKED_PRE struct WMA_unknown_drm_1 {
    WMA_object o;
    GUID unknown_guid;
    UINT16 unknown1;	// one of these is the count of tags
    UINT16 unknown2;

    PACKED_PRE struct WMA_unknown_drm_tag_name {
	UINT16 size;
	UTF16CHAR tag_name[size];	// not null-terminated
    } PACKED_POST;

    WMA_unknown_drm_tag_name tag_names[num_tags];

    UINT32 unknown3;	// one of these is the length of the string
    UINT32 unknown4;

    UTF16CHAR value_name[value_size];
} PACKED_POST;
*/

#if USE_PACK_HEADERS
#include <poppack.h>
#endif

#endif /* ASF_FORMAT_H */
