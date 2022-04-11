/* dyndata_format.h
 *
 * The dynamic data stored per-fid on the player
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.15 13-Mar-2003 18:15 rob:)
 */

#ifndef DYNDATA_FORMAT_H
#define DYNDATA_FORMAT_H

#ifndef TYPES_H
#include "core/types.h"
#endif

#define DYNAMICDATA_VERSION 1

/** @todo Work out how to tell emplode about the existence of track profiler data */


/** This is supposed to be fixed size.  So _don't_ add things to it.
 * If you want to add things to this structure, talk to Peter to find out how.
 */
struct DynamicData1
{
    unsigned unused0 : 1;		/* was "drive", but it's bogus. */
    unsigned mark : 1;			/* is the track marked for attention? */
    unsigned unused1 : 30;		/* spare bits - not used yet */
    unsigned short	normalisation;	/* not used yet */
    unsigned short	play_count;	/* number of times played */
    int			play_last;	/* last time played */
    unsigned int        bpm_data;       /* stuff needed for bpm & beat detection */
    unsigned int        skipped_count;	/* number of times skipped */
    static DynamicData1 Empty;
};

/* Last Time Played : time_t
 *
 * This is tricky, so pay attention:  The player has TZ set to GMT.
 * The user doesn't know this, and assumes that it's set to local time,
 * which would generally be PDT for most of our client base.
 *
 * Thus, when retrieving any times from the player, we must assume
 * that they're in local time.
 *
 * In short, treat the times on the player as local time.
 */

typedef DynamicData1 DynamicData;

/** This structure wraps the raw DynamicData for sending to the player (see
 * lib/model/Node.cpp, Node::Synchronise) so that the player can detect wrong
 * versions.
 */

struct WrappedDynamicData
{
    size_t version;
    size_t itemsize;
    DynamicData data;
};

void ConvertDynamicData( const BYTE *buf, size_t bufsize, int version,
			 DynamicData *result );

#endif
