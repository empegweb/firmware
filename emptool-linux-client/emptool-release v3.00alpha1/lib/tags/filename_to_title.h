/* filename_to_title.h
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 13-Mar-2003 18:15 rob:)
 */

#ifndef FILENAME_TO_TITLE
#define FILENAME_TO_TITLE 1

#include <string>
#include "empeg_tchar.h"

/** Since there's nothing better available, turn the filename into the best title
 ** we can come up with.
 **/
tstring TranslateFilenameToTitle(const tstring &filename);

#endif // FILENAME_TO_TITLE