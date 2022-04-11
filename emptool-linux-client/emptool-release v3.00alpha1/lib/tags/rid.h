/* rid.h
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 */

#ifndef RID_H
#define RID_H

#ifndef STREAM_H
#include "stream.h"
#endif
#ifndef EMPEG_STATUS_H
#include "empeg_status.h"
#endif
#include <string>

/** Calculate a unique ID for the interesting section of the file */
STATUS CalculateRid(SeekableStream *stm,
		    unsigned int startpos, unsigned int endpos,
		    std::string *result);

#endif
