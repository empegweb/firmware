/* tag_extractor.cpp
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.17 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "tag_extractor.h"
#include "tag_errors.h"
#include "id3.h"
#include "asf.h"
#include "wav.h"
#include "stringpred.h"
#include "file_stream.h"
#include "file.h"

TagExtractor::~TagExtractor()
{
}

struct TagExtractorLookup {
    const char *ext;
    STATUS (*pCreate) (const std::string &filename, TagExtractor **pp);
};

static TagExtractorLookup extractors[] = {
    // MP2/MP3 files
    { "mp2", ID3TagExtractor::Create, },
    { "mp3", ID3TagExtractor::Create, },

    // WMA files
    { "asf", ASFTagExtractor::Create, },
    { "wma", ASFTagExtractor::Create, },

    // WAV files
    { "wav", WAVTagExtractor::Create, },

    { NULL, NULL, },
};

/** If you pass an empty string for the suffix, we'll get it from the filename
 */
STATUS TagExtractor::Create(const std::string &filename, const std::string &suffix, TagExtractor **ppExtractor)
{
    stringpred::IgnoreCaseEq eq;
    std::string ext(suffix);
    if (ext.empty())
	ext = util::GetFileExtension(filename);

    for (TagExtractorLookup *p = extractors; p->ext != NULL; ++p)
    {
	if (eq(ext, p->ext))
	    return p->pCreate(filename, ppExtractor);
    }

    return TAG_E_UNSUPPORTED;
}

/** Phew, snuck this one under the Radar. */
STATUS TagExtractor::ExtractTags(const std::string &filename,
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
