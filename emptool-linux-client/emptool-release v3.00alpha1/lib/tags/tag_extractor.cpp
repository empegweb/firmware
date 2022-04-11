/* tag_extractor.cpp
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.20 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "tag_extractor.h"
#include "tag_errors.h"
#include "id3.h"
#include "asf.h"
#include "wav.h"
#include "ogg.h"
#include "stringpred.h"
#include "file_stream.h"
#include "file.h"

TagExtractor::~TagExtractor()
{
}

struct TagExtractorLookup {
    const TCHAR * ext;
    STATUS (*pCreate) (const tstring &filename, TagExtractor **pp);
};

static const TagExtractorLookup extractors[] = {
    // MP2/MP3 files
    { _T("mp2"), ID3TagExtractor::Create, },
    { _T("mp3"), ID3TagExtractor::Create, },

    // WMA files
    { _T("asf"), ASFTagExtractor::Create, },
    { _T("wma"), ASFTagExtractor::Create, },

    // WAV files
    { _T("wav"), WAVTagExtractor::Create, },

    // Ogg Vorbis files
    { _T("ogg"), tags::OggTagExtractor::Create, },

    { NULL, NULL, },
};

/** If you pass an empty string for the suffix, we'll get it from the filename */
STATUS TagExtractor::Create(const tstring &filename, const tstring &suffix, TagExtractor **ppExtractor)
{
    stringpred::IgnoreCaseEq eq;
    tstring ext(suffix);
    if (ext.empty())
	ext = util::GetFileExtension(filename);

    for (const TagExtractorLookup *p = extractors; p->ext != NULL; ++p)
    {
	if (eq(ext, p->ext))
	    return p->pCreate(filename, ppExtractor);
    }

    return TAG_E_UNSUPPORTED;
}

STATUS TagExtractor::ExtractTags(const tstring &filename,
				 TagExtractorObserver *pObserver)
{
    FileStream stm;

    STATUS status;
    if (FAILED(status = stm.Open(filename, FileStream::FLAG_READ)))
	return status;

    status = ExtractTags(&stm, pObserver);

    stm.Close();

    return status;
}

STATUS TagExtractor::ExtractRid(const tstring &filename, std::string *rid)
{
    FileStream stm;

    STATUS status;
    if (FAILED(status = stm.Open(filename, FileStream::FLAG_READ)))
        return status;

    status = ExtractRid(&stm, rid);

    stm.Close();

    return status;
}
