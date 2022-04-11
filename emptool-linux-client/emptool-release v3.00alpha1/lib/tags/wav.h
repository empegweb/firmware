/* wav.h
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.7 13-Mar-2003 18:15 rob:)
 */

#ifndef WAV_H
#define WAV_H 1

#include "tag_extractor.h"

class WAVTagExtractor : public TagExtractor
{
    tstring m_filename;

public:
    WAVTagExtractor(const tstring &filename)
	: m_filename(filename) { }

    static STATUS Create(const tstring &filename, TagExtractor **ppExtractor);

    virtual STATUS ExtractTags(SeekableStream *pStm, TagExtractorObserver *pObserver);
    virtual STATUS ExtractRid(SeekableStream *pStm, std::string *rid);

private:
    STATUS IdentifyWAVStream(SeekableStream *stream, TagExtractorObserver *pObserver);
};

#endif /* WAV_H */
