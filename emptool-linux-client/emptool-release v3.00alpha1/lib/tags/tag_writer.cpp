/* tag_writer.cpp
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.4 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "tag_writer.h"
#include "tag_errors.h"

#ifdef WIN32
#define ENABLE_ASF 1
#else
#define ENABLE_ASF 0
#endif

#include "id3_writer.h"
#if ENABLE_ASF
#include "asf_writer.h"
#endif

#include "stringpred.h"
#include "file.h"

namespace tags {

static const struct TagWriterLookup { 
    const TCHAR *ext;
    STATUS (*pCreate)(const tstring& filename, TagWriter**);
} writers[] = {
    { _T("mp2"), &ID3TagWriter::Create },
    { _T("mp3"), &ID3TagWriter::Create },
#if ENABLE_ASF
    { _T("wma"), &ASFTagWriter::Create },
    { _T("asf"), &ASFTagWriter::Create },
#endif // ENABLE_ASF
    { NULL, NULL }
};

STATUS TagWriter::Create(const tstring& filename, 
			 const tstring& suffix,
			 TagWriter **ppWriter)
{
    stringpred::IgnoreCaseEq eq;
    tstring ext(suffix);
    if (ext.empty())
	ext = util::GetFileExtension(filename);

    for (const TagWriterLookup *p = writers; p->ext != NULL; ++p)
    {
        TRACE("Trying for match of \'%s\' against \'%s\'\n", ext.c_str(), p->ext);
	if (eq(ext, p->ext))
	    return p->pCreate(filename, ppWriter);
    }

    TRACE("Unable to find tag writer for suffix \'%s\'\n", ext.c_str());
    return TAG_E_UNSUPPORTED;
}

}; // namespace tags
