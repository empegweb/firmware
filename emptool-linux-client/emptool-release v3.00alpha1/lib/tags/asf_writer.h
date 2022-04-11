/* asf_writer.h
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * Author:
 *   Mike Crowe <mcrowe@sonicblue.com>
 *
 * (:Empeg Source Release 1.4 13-Mar-2003 18:15 rob:)
 */


#ifndef ASF_WRITER_H
#define ASF_WRITER_H

#ifndef TAG_WRITER_H
#include "tag_writer.h"
#endif
#ifndef ID3_H
#include "id3.h"
#endif
#ifndef ID3_H
#include "id3v1_format.h"
#endif

#include <string>
#include <set>
#include <vector>

class FileStream;

namespace tags 
{
    class ASFTagWriter: public TagWriter
    {
    public:
        static STATUS Create(const tstring& filename, TagWriter **ppWriter);

        // Being a TagWriter
        STATUS Write() = 0;
        STATUS SetTag(const char *tagname, const char *tagValue) = 0;
    };

}; // namespace tags

#endif
