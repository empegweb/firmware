/* filename_to_title.cpp
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "filename_to_title.h"
#include "file.h"

tstring TranslateFilenameToTitle(const tstring &filename)
{
    // Chop off the prefix and suffix.
    tstring result = util::TruncateFilename(filename);

    // Turn all underscores into spaces.
    for(tstring::iterator i = result.begin(); i != result.end(); ++i)
    {
	if (*i == _T('_'))
	    *i = _T(' ');
    }

    return result;
}
