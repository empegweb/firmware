/* dyndata_format.cpp
 *
 * The dynamic data stored per-fid on the player
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.7 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "types.h"
#include "trace.h"
#include <time.h>
#include <string.h>

#include "dyndata_format.h"

DynamicData1 DynamicData1::Empty; // Initialised to zero automagically.

void ConvertDynamicData( const BYTE *buf, size_t bufsize, int version,
			 DynamicData *result )
{
    // Only used in debug builds.
    UNUSED(bufsize);
    if ( version == DYNAMICDATA_VERSION )
    {
	ASSERT( bufsize == sizeof(DynamicData) );
	memcpy( result, buf, sizeof(DynamicData) );
	return;
    }

    //UNUSED(bufsize);

    // Failing? Someone needs to write version conversion code
    ASSERT(false);
}

/* eof */
