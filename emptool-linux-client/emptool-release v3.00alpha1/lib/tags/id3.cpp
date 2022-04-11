/* id3.cpp
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.52 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#define TRACE_TAGS 0
#include "id3.h"
#include "hex.h"
#include "stringpred.h"
#include "file.h"
#include "genre.h"
#include "mp3_frame_info.h"
#include "utf8.h"
#include "rid.h"
#include <stdio.h>
#include "filename_to_title.h"
#include "empeg_endian.h"

/** Change this to 1 to test UTF8 support
 * NOTE! this is not a long-term solution! Very soon *all* builds will have
 * UTF8 enabled!
 */
#define ENABLE_UTF8 1

STATUS ID3TagExtractor::Create(const tstring &filename, TagExtractor **pp)
{
    ID3TagExtractor *p = NEW ID3TagExtractor(filename);
    if (!p)
	return E_OUTOFMEMORY;

    *pp = p;
    return S_OK;
}

/** Create something that'll get kicked with each tag as it's found.
 * Once we've collected them all, we can sift through looking for the interesting ones.
 */
STATUS ID3TagExtractor::ExtractTags(SeekableStream *pStm, TagExtractorObserver *pObserver)
{
    pObserver->OnExtractTag("codec", "mp3");
    
    CollectFrames collect;
    m_collect = &collect;
    
    // Prime it with a default title...
    collect.OnTextFrame("0TIT", util::UTF8FromT(TranslateFilenameToTitle(m_filename).c_str()));

    unsigned trailer = 0;
    ExtractV1Tag(pStm, &collect, &trailer);

    // Even if we don't parse Lyrics frames we should exclude them from the rid
    CheckLyrics(pStm, &trailer);

    unsigned offset = 0;
    ID3V2TagParser::Parse(pStm, this, &offset);

    pObserver->OnExtractTag("offset", offset);
    pObserver->OnExtractTag("trailer", trailer);

    // Now we've collected the frames, extract the relevant details...

    struct FrameMapping {
	enum { MAX_MAPPINGS = 6, };
	const char *tagName;
	const char *frameIDs[MAX_MAPPINGS];
    };

    /** This loop seems a little inside-out.  We should be scooting along the
     * tags we've found, looking for the equivalent empeg tag.
     *
     * However, that leaves us with broken priorities.  So we will do it this
     * way.
     *
     * Note: If you extend the list of possible ID3V2 tags, check that you've:
     * (1) got a NULL at the end.
     * (2) not got more than MAX_MAPPINGS (including the NULL) in the list.
     */
    static const FrameMapping frameMapping[] = {
	{ "title",  { "TIT2", "1TIT", "0TIT",			NULL, }, },
	{ "artist", { "TPE1", "TPE2", "TPE3", "TCOM", "1ART",	NULL, }, },
	{ "source", { "TALB", "1ALB",				NULL, }, },
	{ "year",   { "TYER", "1YER", /* deprecated -- should use TDRC */ NULL, }, },
	{ "comment",{ "COMM", "1COM",				NULL, }, },
	{ "genre",  { "TCON", "1CON",				NULL, }, },
	{ "tracknr",{ "TRCK", "1TRK", "1TNR",    		NULL, }, },
	{ "file_id",{ "TRCK", "1TRK", "1TNR",			NULL, }, }, // Intentional duplicate - keep in line with "tracknr"
	{ "subtitle",  { "TIT3",                                NULL, }, },
	{ "composer",  { "TCOM",                                NULL, }, },
	{ "conductor", { "TPE3",                                NULL, }, },
	{ "remixed",   { "TPE4",                                NULL, }, },
	{ "partofset", { "TPOS",                                NULL, }, },
	{ "partname",  { "TSST",                                NULL, }, },
	{ "isrc",      { "TSRC",                                NULL, }, },
	{ "originalartist", { "TOPE",                           NULL, }, },
	{ "lyricist",  { "TEXT",                                NULL, }, },
	{ "bpm",       { "TBPM",                                NULL, }, },
	{ "key",       { "TKEY",                                NULL, }, },
	{ "language",  { "TLAN",                                NULL, }, },
	{ "sourcemedia", { "TMED",                              NULL, }, },
	{ "mood",      { "TMOO",                                NULL, }, },
	{ "sourcesort", { "TSOA",                               NULL, }, },
	{ "artistsort", { "TSOP",                               NULL, }, },
	{ "titlesort", { "TSOT" ,                               NULL, }, },
	{ NULL, { NULL, }, },
    };

    for (const FrameMapping *m = frameMapping; m->tagName; ++m)
    {
	for (const char *const *q = m->frameIDs; *q; ++q)
	{
	    const char *p = *q;
	    StringMap::const_iterator i = collect.find(p);
	    if (i != collect.end())
	    {
		pObserver->OnExtractTag(m->tagName, i->second.c_str());
		break;
	    }
	}
    }

    // Now calculate the unique ID
    unsigned int length;
    if (SUCCEEDED(pStm->Length(&length)))
    {
	std::string rid;
    STATUS st = ExtractRidSpecific(pStm, length, offset, trailer, &rid);
	if (FAILED(st))
	    return st;
	pObserver->OnExtractTag("rid", rid.c_str());
    }

    FrameInfoExtract extract(pStm);
    return extract.Extract(offset, pObserver);
}

STATUS ID3TagExtractor::ExtractRid(SeekableStream *pStm, std::string *rid)
{
    STATUS status;
    unsigned int length;
    if (FAILED(status = pStm->Length(&length)))
        return status;

    unsigned trailer = 0;
    ExtractV1Tag(pStm, NULL, &trailer);

    // Even if we don't parse Lyrics frames we should exclude them from the rid
    CheckLyrics(pStm, &trailer);

    unsigned offset = 0;
    ID3V2_Tag header;
    ID3V2TagParser::ParseHeader(pStm, &header, &offset);

    return ExtractRidSpecific(pStm, length, offset, trailer, rid);
}

STATUS ID3TagExtractor::ExtractRidSpecific(SeekableStream *pStm, unsigned int length, unsigned int offset, 
                                           unsigned int trailer, std::string *rid)
{
    return CalculateRid(pStm, offset, length-trailer, rid);
}

void ID3TagExtractor::CheckLyrics(SeekableStream *pStm, unsigned int *trailer)
{
    unsigned int length;
    if (FAILED(pStm->Length(&length)))
	return;
    if ((length-*trailer) < 15)
	return;

    char bytes[15];
    pStm->SeekAbsolute(length-*trailer-15);
    unsigned int bytesread;
    pStm->Read(bytes, 15, &bytesread);
    if (strncmp(bytes+6,"LYRICS",6))
	return;

    TRACE_WARN("File contains (uninterpreted) Lyrics tag\n");
    unsigned int lyrics_len = strtol(bytes,NULL, 10);

    if (*trailer+lyrics_len+15 >= length)
	return;

    unsigned int newpos = length-*trailer-lyrics_len-15;
    pStm->SeekAbsolute(newpos);
    pStm->Read(bytes, 11, &bytesread);
    if (strncmp(bytes, "LYRICSBEGIN", 11))
    {
	TRACE_WARN("Lyrics tag corrupt\n");
	return;
    }
    
    TRACEC(TRACE_TAGS, "%d bytes of Lyrics3 confirmed\n", lyrics_len);
    *trailer += lyrics_len;
}

void ID3TagExtractor::CollectFrames::OnUnknownFrame(SeekableStream *pStm,
						    const ID3V2_Frame *frameHeader, int frameSize)
{
    UNUSED(pStm);
    UNUSED(frameSize);
    
    std::string frameID(frameHeader->frameID, 4);

#if 0
    const BYTE *data = reinterpret_cast<const BYTE *>(frameHeader + 1);
    OnUnknownFrame(frameID, data, frameSize);
#endif
}

void ID3TagExtractor::CollectFrames::OnUnknownFrame(const std::string &frameID,
						    const BYTE *data, int size)
{
    UNUSED(frameID);
    UNUSED(data);
    UNUSED(size);
//    printf("  %s : %d byte(s)\n", frameID.c_str(), size);
//    PrintHex(data, size);
}

void ID3TagExtractor::CollectFrames::OnCommentFrame(SeekableStream *pStm,
						 const ID3V2_Frame *frameHeader, int frameSize)
{
    std::string frameID(frameHeader->frameID, 4);

    if (frameSize < 5)
    {
	TRACEC(TRACE_TAGS, "  Short frame!\n");
	return;
    }

    // It's a text frame, so we'll treat it as a char array.
    char *buffer = NEW char[frameSize];

    unsigned bytesRead;
    pStm->Read(buffer, frameSize, &bytesRead);

    const char *textInformation = buffer;
    char textEncoding = *textInformation++;
    UNUSED(textEncoding);
    TRACEC(TRACE_TAGS, "  Text encoding: %d (%s)\n", textEncoding,
	   DescribeTextEncoding(textEncoding));

    /** @todo I've no idea what format the language-encoding is in.
     * This reads it as 3 little-endian bytes.
     */
    UINT32 language = 0;
    language = *textInformation++;
    language |= (*textInformation++) << 8;
    language |= (*textInformation++) << 16;

    // Immediately after that comes a "short content description".  We'll ignore it.
    // It's zero-terminated.
    while (textInformation < buffer + frameSize)
    {
	if (*textInformation == 0)
	    break;

	++textInformation;
    }

    // The rest of the data is the comment itself.
    ++textInformation;
    if (textInformation < buffer + frameSize)
    {
	int bytesRemaining = (buffer + frameSize) - textInformation;
	std::string comment(stringpred::FromFixedBuffer(textInformation, bytesRemaining));
	OnTextFrame(frameID, comment);
    }

    delete[] buffer;
}

/** Replace any '\xFF\x00' sequences with '\xFF' */
static void DeUnsynchronise(char *buffer, int *pFrameSize)
{
    char *readptr = buffer;
    char *writeptr = buffer;
    int size = *pFrameSize;

    while (readptr < buffer + size)
    {
	*writeptr = *readptr;
	writeptr++;
	if (*readptr == '\xFF' && readptr[1] == '\0')
	    readptr += 2;
	else
	    readptr++;
    }
    *pFrameSize = writeptr - buffer;
}

// Ick.  A global.
bool g_shouldTreatLatin1AsLocal;

void TreatLatin1AsLocal(bool b)
{
    g_shouldTreatLatin1AsLocal = b;
}

bool ShouldTreatLatin1AsLocal()
{
    return g_shouldTreatLatin1AsLocal;
}

void ID3TagExtractor::CollectFrames::OnTextFrame(SeekableStream *pStm,
						 const ID3V2_Frame *frameHeader, int frameSize)
{
    std::string frameID(frameHeader->frameID, 4);

    // It's a text frame...
    if (frameSize < 1)
    {
	TRACEC(TRACE_TAGS, "  Short frame!\n");
	return;
    }

    // It's a text frame, so we can treat it as a char array.
    char *buffer = NEW char[frameSize];

    unsigned bytesRead;
    pStm->Read(buffer, frameSize, &bytesRead);

    DeUnsynchronise(buffer, &frameSize);
    
    const char *textInformation = buffer;
    char textEncoding = *textInformation++;
    TRACEC(TRACE_TAGS, "  Text encoding: %d (%s)\n", textEncoding,
	   DescribeTextEncoding(textEncoding));
    
    // Decode the text frame, and add it to the list.
    if (textEncoding == ID3V2_TEXT_ENCODING_ISO8859_1)
    {
#if ENABLE_UTF8
	// subtract 1 to cater for the text-encoding byte.
	std::string s(textInformation, frameSize - 1);
	if (!s.empty())
	{
	    if (ShouldTreatLatin1AsLocal())	// sigh...
		OnTextFrame(frameID, util::UTF8FromSystem(s));
	    else
		OnTextFrame(frameID, util::UTF8FromLatin1(s));
	}
    }
    else if (textEncoding == ID3V2_TEXT_ENCODING_UNICODE)
    {
	const UTF16CHAR *tmp = reinterpret_cast<const UTF16CHAR *>(textInformation);
	utf16_string s(tmp, (frameSize - 1) / sizeof(UTF16CHAR));
	if (!s.empty())
	    OnTextFrame(frameID, util::UTF8FromUTF16(s));
    }
    else if (textEncoding == ID3V2_TEXT_ENCODING_UNICODE_BE)
    {
	const UTF16CHAR *tmp = reinterpret_cast<const UTF16CHAR *>(textInformation);
	utf16_string s(tmp, (frameSize - 1) / sizeof(UTF16CHAR));

	if (!s.empty())
	{
	    // s is big-endian; add reversed BOM if we're little-endian
#if __BYTE_ORDER == __LITTLE_ENDIAN
	    s = ((UTF16CHAR)0xFEFF) + s;
#endif
	    OnTextFrame(frameID, util::UTF8FromUTF16(s));
	}
    }
    else if (textEncoding == ID3V2_TEXT_ENCODING_UTF8)
    {
#endif
	std::string s(textInformation, frameSize - 1);
	if (!s.empty())
	    OnTextFrame(frameID, s);
    }
    else
    {
	TRACEC(TRACE_TAGS, "  Unknown Text Encoding!\n");
    }

    delete[] buffer;
}

void ID3TagExtractor::CollectFrames::OnTextFrame(const std::string &frameID,
						 const utf8_string &frameData)
{
//    printf("  %s : %s\n", frameID.c_str(), frameData.c_str());

    /// @todo Fix this icky special-case for genre.
    if (frameID == "TCON")
    {
	utf8_string genre(frameData);

	int genre_num = -1;
	if (sscanf(genre.c_str(), "(%d)", &genre_num) == 1)
	    genre = util::UTF8FromT(GenreList::Lookup(genre_num));

	if (!genre.empty())
	    insert(std::make_pair(frameID, genre));
    }
    else if (frameID == "1YER")     // We don't like zero years - so check before adding
    {
	int year = 0;
	if ((sscanf(frameData.c_str(), "%d", &year) == 1) &&
            (year > 0))
	    insert(std::make_pair(frameID, frameData));
    }
    else
    {
	insert(std::make_pair(frameID, frameData));
    }
}
