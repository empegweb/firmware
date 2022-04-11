/* ogg.h
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 13-Mar-2003 18:15 rob:)
 */

#ifndef OGG_H
#define OGG_H 1

#ifndef TAG_EXTRACTOR_H
#include "tag_extractor.h"
#endif

namespace tags {

class OggTagExtractor: public TagExtractor
{
    tstring m_filename;

    STATUS ExtractRidSpecific(SeekableStream *pStm, unsigned int length, std::string *rid);

 public:
    OggTagExtractor(const tstring& filename) : m_filename(filename) {}

    static STATUS Create(const tstring& filename, TagExtractor **pp);
    virtual STATUS ExtractTags(SeekableStream *pStm,
			       TagExtractorObserver *teo);
    virtual STATUS ExtractRid(SeekableStream *pStm, std::string *rid);
};

}; // namespace tags

#endif
