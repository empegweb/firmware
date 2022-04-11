/* wav.h
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 01-Apr-2003 18:52 rob:)
 */

#ifndef WAV_H
#define WAV_H 1

#include "tag_extractor.h"

class WAVTagExtractor : public TagExtractor
{
    std::string m_filename;

public:
    WAVTagExtractor(const std::string &filename)
	: m_filename(filename) { }

    static STATUS Create(const std::string &filename, TagExtractor **ppExtractor);

    virtual STATUS ExtractTags(SeekableStream *pStm, TagExtractorObserver *pObserver);

private:
    STATUS IdentifyWAVStream(SeekableStream *stream, TagExtractorObserver *pObserver);
};

#endif /* WAV_H */
