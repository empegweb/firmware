/* id3.h
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.14.2.1 01-Apr-2003 18:52 rob:)
 */

#ifndef ID3_H
#define ID3_H 1

#include "tag_extractor.h"
#include "id3v1_format.h"
#include "id3v2_format.h"
#include <map>
#include <string>

class ID3TagExtractor : public TagExtractor
{
    std::string m_filename;

public:
    ID3TagExtractor(const std::string &filename)
	: m_filename(filename) { }

    /** If this fails, the value of *pp is undefined. */
    static STATUS Create(const std::string &filename, TagExtractor **pp);

    virtual STATUS ExtractTags(SeekableStream *pStm, TagExtractorObserver *pObserver);

public:
    typedef std::map<std::string, std::string> StringMap;
    class CollectFrames : public StringMap {
    public:
	void OnTextFrame(SeekableStream *pStm, const ID3V2_Frame *frameHeader, int frameSize);
	void OnTextFrame(const std::string &frameID, const std::string &frameData);
	void OnTextFrame(const std::string &frameID, const utf16_string &frameData);
	void OnCommentFrame(SeekableStream *pStm, const ID3V2_Frame *frameHeader, int frameSize);
	void OnUnknownFrame(SeekableStream *pStm, const ID3V2_Frame *frameHeader, int frameSize);
	void OnUnknownFrame(const std::string &frameID, const BYTE *data, int size);
    };

    struct FrameIDLookup {
	typedef void (CollectFrames::*PFN) (SeekableStream *pStm, const ID3V2_Frame *frameHeader, int frameSize);

	char frameID[5];
	char frameThreeCC[4];
	const char *frameDescription;
	PFN pfn;
    };

    static const FrameIDLookup frameLookup[];

private:
    STATUS ExtractV1Tag(SeekableStream *pStm, CollectFrames *collect);
    STATUS ExtractV1Tag(const ID3V1_Tag *tag, CollectFrames *collect);

    STATUS ExtractV2Tag(SeekableStream *pStm, CollectFrames *collect, unsigned *offset, TagExtractorObserver *pObserver);
    STATUS ExtractV2Frame(SeekableStream *pStm, const ID3V2_Frame *frame, CollectFrames *collect);

    const char *DescribeFrameID(const char *frameID);
    const char *ConvertThreeCCtoFourCC(const char *threecc);
};

#endif /* ID3_H */
