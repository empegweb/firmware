/* tag_writer.h
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.4 13-Mar-2003 18:15 rob:)
 */

#ifndef TAG_WRITER_H
#define TAG_WRITER_H

#include "empeg_status.h"
#include "empeg_tchar.h"

namespace tags {

/** Write metadata back to a content file (e.g. to ID3 tags)
 */
class TagWriter
{
 public:
    virtual ~TagWriter() {}
    virtual STATUS SetTag(const char *tagname, const char *tagValue) = 0;
    virtual STATUS Write() = 0;
    
    static STATUS Create(const tstring& filename,
			 const tstring& suffix,
			 TagWriter **ppWriter);
};

}; // namespace tags

#endif
