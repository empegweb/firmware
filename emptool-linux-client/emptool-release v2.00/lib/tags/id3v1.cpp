/* id3v1.cpp
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.9 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#define TRACE_TAGS 0
#include "id3.h"
#include "tag_errors.h"
#include "stringpred.h"
#include "genre.h"
#include <stdio.h>

STATUS ID3TagExtractor::ExtractV1Tag(SeekableStream *pStm, CollectFrames *collect)
{
    // Go looking for ID3v1 tags...
    unsigned fileSize;
    STATUS status = pStm->Length(&fileSize);
    if (FAILED(status))
	return status;

    if (fileSize < sizeof(ID3V1_Tag))
	return TAG_E_ID3_TOOSHORT;	// Too short to contain a V1 tag

    ID3V1_Tag v1;

    if (FAILED(status = pStm->SeekAbsolute(fileSize - sizeof(ID3V1_Tag))))
	return status;
    
    unsigned bytesRead;
    if (FAILED(status = pStm->Read(&v1, sizeof(ID3V1_Tag), &bytesRead)))
	return status;

    if (bytesRead != sizeof(ID3V1_Tag))
	return TAG_E_ID3_TOOSHORT;	// Failed to read enough data for the V1 tag

    return ExtractV1Tag(&v1, collect);
}

static std::string GetID3V1String(const char *pBuffer, size_t cbBuffer)
{
    typedef std::string::size_type size_type;
    const size_type npos = std::string::npos;

    std::string r(stringpred::FromFixedBuffer(pBuffer, cbBuffer));
    
    size_type pos = r.find_last_not_of(' ');
    if (pos == npos)
	return std::string();

    return std::string(r.begin(), r.begin() + pos + 1);
}

/** This is flagged by 'TAG' 128 bytes short of the file.
 */
STATUS ID3TagExtractor::ExtractV1Tag(const ID3V1_Tag *tag, CollectFrames *collect)
{
    ASSERT(GetID3V1String("1                             ", 30) == "1");
    ASSERT(GetID3V1String("1234567890123456789           ", 30) == "1234567890123456789");
    ASSERT(GetID3V1String("123456789012345678901234567890", 30) == "123456789012345678901234567890");
    ASSERT(GetID3V1String("                              ", 30) == "");
    ASSERT(GetID3V1String("     6789                     ", 30) == "     6789");
    ASSERT(GetID3V1String("     6789\0                   ", 30) == "     6789");

    if (memcmp(tag->signature, "TAG", 3) != 0)
	return TAG_E_ID3_NOTV1;	// Doesn't look like ID3V1 tag

    std::string title(GetID3V1String(tag->title, sizeof(tag->title)));
    if (!title.empty())
	collect->OnTextFrame("1TIT", title);

    std::string artist(GetID3V1String(tag->artist, sizeof(tag->artist)));
    if (!artist.empty())
	collect->OnTextFrame("1ART", artist);

    std::string album(GetID3V1String(tag->album, sizeof(tag->album)));
    if (!album.empty())
	collect->OnTextFrame("1ALB", album);

    std::string year(GetID3V1String(tag->year, sizeof(tag->year)));
    if (!year.empty())
	collect->OnTextFrame("1YER", year);

    std::string comment(GetID3V1String(tag->comment, sizeof(tag->comment)));
    if (!comment.empty())
	collect->OnTextFrame("1COM", comment);

    /* ID3v1.1 states that comment[28] should be zero, and comment[29] should be the track number.
     * If comment[28] isn't zero, we'll assume that we don't have a ID3v1.1 tag.
     */
    if (tag->comment[28] == 0 && tag->comment[29] != 0)
    {
	BYTE tracknr = tag->comment[29];

	char temp[12];
	sprintf(temp, "%d", tracknr);
	collect->OnTextFrame("1TNR", temp);
    }
    
    std::string genre(GenreList::Lookup(tag->genre));
    if (!genre.empty())
	collect->OnTextFrame("1CON", genre);

    return S_OK;
}

