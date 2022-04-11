/* empeg note: although copyright Xing, this code may be freely used in any
 * MP3 player, according to Xing's web site at
 * http://www.xingtech.com/support/partner_developer/mp3/vbr_sdk/
 *
 * empeg note: unlike the rest of this release, this source file is NOT
 * under the GNU General Public Licence.
 *
 * (:Empeg Source Release 1.4 13-Mar-2003 18:15 rob:)
 */

/*---- DXhead.h --------------------------------------------


decoder MPEG Layer III

handle Xing header


Copyright 1998 Xing Technology Corp.
-----------------------------------------------------------*/

// A Xing header may be present in the ancillary
// data field of the first frame of an mp3 bitstream
// The Xing header (optionally) contains
//      frames      total number of audio frames in the bitstream
//      bytes       total number of bytes in the bitstream
//      toc         table of contents

// toc (table of contents) gives seek points
// for random access
// the ith entry determines the seek point for
// i-percent duration
// seek point in bytes = (toc[i]/256.0) * total_bitstream_bytes
// e.g. half duration seek point = (toc[50]/256.0) * total_bitstream_bytes


#define FRAMES_FLAG     0x0001
#define BYTES_FLAG      0x0002
#define TOC_FLAG        0x0004
#define VBR_SCALE_FLAG  0x0008

#define FRAMES_AND_BYTES (FRAMES_FLAG | BYTES_FLAG)

// structure to receive extracted header
// toc may be NULL
typedef struct {
    int h_id;       // from MPEG header, 0=MPEG2, 1=MPEG1
    int samprate;   // determined from MPEG header
    int flags;      // from Xing header data
    unsigned long frames;     // total bit stream frames from Xing header data
    unsigned long bytes;      // total bit stream bytes from Xing header data
    unsigned long vbr_scale;  // encoded vbr scale from Xing header data
    unsigned char toc[256];  // table of contents
} xing_vbr_header;


