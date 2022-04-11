/* id3.h
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.23 13-Mar-2003 18:15 rob:)
 */

#ifndef ID3_H
#define ID3_H 1

#include "tag_extractor.h"
#include "id3v1_format.h"
#include "id3v2_format.h"
#include <map>
#include <string>

namespace tags { class ID3TagWriter; };

class ID3V2FrameObserver
{
 public:
    virtual ~ID3V2FrameObserver() {}
    virtual void OnFrame(SeekableStream *pStm, const ID3V2_Frame *frame) = 0;
};

class ID3V2TagParser
{
 public:
    static STATUS Parse(SeekableStream*, ID3V2FrameObserver *observer, 
			unsigned *pOffset);
    static STATUS ParseHeader(SeekableStream *pStm, ID3V2_Tag *header, unsigned *offset);
};

void TreatLatin1AsLocal(bool b);
bool ShouldTreatLatin1AsLocal();

class ID3TagExtractor : public TagExtractor, public ID3V2FrameObserver
{
    tstring m_filename;
    class CollectFrames;
    CollectFrames *m_collect;

public:
    ID3TagExtractor(const tstring &filename)
	: m_filename(filename), m_collect(NULL) { }

    /** If this fails, the value of *pp is undefined. */
    static STATUS Create(const tstring &filename, TagExtractor **pp);

    /** Being a TagExtractor */
    virtual STATUS ExtractTags(SeekableStream *pStm, TagExtractorObserver *pObserver);
    virtual STATUS ExtractRid(SeekableStream *pStm, std::string *rid);

    /** Being an ID3V2FrameObserver */
    virtual void OnFrame(SeekableStream*, const ID3V2_Frame*);

private:
    typedef std::map<std::string, std::string> StringMap;
    class CollectFrames : public StringMap {
    public:
	void OnTextFrame(SeekableStream *pStm, const ID3V2_Frame *frameHeader, int frameSize);
	void OnTextFrame(const std::string &frameID, const std::string &utf8FrameData);
	void OnCommentFrame(SeekableStream *pStm, const ID3V2_Frame *frameHeader, int frameSize);
	void OnUnknownFrame(SeekableStream *pStm, const ID3V2_Frame *frameHeader, int frameSize);
	void OnUnknownFrame(const std::string &frameID, const BYTE *data, int size);
    };

    STATUS ExtractV1Tag(SeekableStream *pStm, CollectFrames *collect,
			unsigned *trailer);
    STATUS ExtractV1Tag(const ID3V1_Tag *tag, CollectFrames *collect,
			unsigned *trailer);

    STATUS ExtractV2Tag(SeekableStream *pStm, unsigned *offset,
			ID3V2FrameObserver *observer);

    STATUS ExtractRidSpecific(SeekableStream *pStm, unsigned int length, unsigned int offset, 
                              unsigned int trailer, std::string *rid);

    void CheckLyrics(SeekableStream *pStm, unsigned int *trailer);

    struct FrameIDLookup {
	typedef void (CollectFrames::*PFN) (SeekableStream *pStm, const ID3V2_Frame *frameHeader, int frameSize);

	char frameID[5];      // ID3v2.3/2.4 tag
	char frameThreeCC[4]; // Corresponding ID3v2.2 tag, or """
	const char *frameDescription;
	PFN pfn;
    };

    static const FrameIDLookup frameLookup[];

 public:
    static const char *DescribeFrameID(const char *frameID);
    static const char *ConvertThreeCCtoFourCC(const char *frameID);
};

#endif /* ID3_H */
