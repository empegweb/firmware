/* fids.h
 *
 * All data files on the player are referred to by file id or "fid"
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * Authors
 *  Mike Crowe <mac@empeg.com>
 *  Hugo Fiennes <hugo@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.23.2.3 01-Apr-2003 18:52 rob:)
 */

#ifndef FIDS_H
#define FIDS_H

#ifndef TYPES_H
#include "types.h"
#endif

/* Predefined fids */
#define FID_VERSION		0
#define FID_ID			1
#define FID_TAGINDEX		2
#define FID_STATICDATABASE     	3
#define FID_DYNAMICDATABASE	4
#define FID_PLAYLISTDATABASE	5
#define FID_CONFIGFILE		6
#define FID_PLAYERTYPE		7
#define FID_VISUALLIST		8
#define FID_EXTRADB		9

/* Internal FIDs - these are for the use of
 * emplode - they should never appear on the player
 */
#define FID_INTERNAL_CLIPBOARD	    0x70
#define FID_INTERNAL_TRASHCAN	    0x80
#define FID_INTERNAL_SEARCHRESULTS  0x90

/* Reserved FIDs that are used on the player
 */
#define FID_ROOTPLAYLIST 0x100
#define FID_LOSTANDFOUND 0x110

/* Convenient definition to find the first normal item
 * on the player.
 */
#define FID_FIRSTNORMAL 0x120

#define FID_MASK_RADIO 0x40000000L
#define FID_RADIOROOT  (FID_MASK_RADIO | FID_ROOTPLAYLIST)

//#define FID_PLAYLIST_ALLTUNES (0x30)

#define FIDTYPE_TUNE 0
#define FIDTYPE_TAGS 1
#define FIDTYPE_LOWBITRATE 2
#define FIDTYPE_MASK (0xf)

inline unsigned int GetFidNumber(FID fid)
{
    return (fid >> 4);
}

inline unsigned int GetFidType(FID fid)
{
    return (fid & 0xf);
}

inline FID MakeFid(unsigned int n, unsigned int t)
{
    return (n << 4) | (t & 0xf);
}

// Options flags: this is a 32-bit bitset, which defines options for a tune or
// playlists. The default options setting is zero.

// Treat entire playlist as one tune
// Not supported: #define PLAYLIST_OPTION_ASONE		0x00000002

// Play playlist without gaps
// Not supported: #define PLAYLIST_OPTION_NOGAPS	0x00000004

// Randomise playlist?
#define PLAYLIST_OPTION_RANDOMISE	0x00000008

// Automatically loop playlist
#define PLAYLIST_OPTION_LOOP		0x00000010

// Don't treat as part of parent
#define PLAYLIST_OPTION_IGNOREASCHILD	0x00000020

// This flag set when the fid information has been fully completed from cd info (or edited manually)
#define PLAYLIST_OPTION_CDINFO_RESOLVED 0x00000040

// Is there a copyright on this fid
#define PLAYLIST_OPTION_COPYRIGHT	0x00000080

// Is this fid marked as a copy
#define PLAYLIST_OPTION_COPY		0x00000100

// Postprocessing: Do we want to bleed the stereo (eg Beatles via headphones)
#define PLAYLIST_OPTION_STEREO_BLEED	0x00000200

#define DEFAULT_SUICIDE_TIMEOUT_MS 60000 /* one minute */
#define MAX_SUICIDE_TIMEOUT_MS 172800000 /* 48 hours */
#define MIN_SUICIDE_TIMEOUT_MS 1000 /* one second */

#endif
