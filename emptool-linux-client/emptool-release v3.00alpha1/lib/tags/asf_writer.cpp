/* asf_writer.cpp
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * Author:
 *   Mike Crowe <mcrowe@sonicblue.com>
 *
 * (:Empeg Source Release 1.9 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "asf_writer.h"

#ifdef WIN32
#include <windows.h>
#include <malloc.h>
#include "wmainfo.h"
#include "utf8.h"

#undef STATUS_SEVERITY
#include <wmsdk.h>
#define TRACE_ASFTAGS 1

namespace tags
{
    using namespace tags_internal;

    class ASFTagWriterImpl : public ASFTagWriter
    {
        enum TagType
        {
            STRING = 0,
            NUMBER = 1,
	    NUMBER_ZERO_BASED = 2, // Some fields are broken and start at zero rather than one
        };
        struct TagTranslationTable
        {
            const char *empeg_tag_name;
            LPCWSTR asf_tag_name;
            TagType asf_tag_type;
        };
        static const TagTranslationTable m_translation_table[];

        tstring m_filename;
        std::map<std::string, std::wstring> m_tags;

        ASFTagWriterImpl(const tstring &filename)
            : m_filename(filename)
        {
        }

    public:
        virtual ~ASFTagWriterImpl();

        static STATUS Create(const tstring& filename, TagWriter **ppWriter);

        // Being a TagWriter
        STATUS Write();
        STATUS SetTag(const char *tagname, const UTF8CHAR *tagValue);
    };

    /// @todo: DWORD ones: g_wszWMTrack (track number),  

    const ASFTagWriterImpl::TagTranslationTable ASFTagWriterImpl::m_translation_table[] =
    {
        { "title", L"Title", STRING },
        { "artist", L"Author", STRING },
        { "source", L"WM/AlbumTitle", STRING },
        { "genre", L"WM/Genre", STRING },
        { "comment", L"Description", STRING },
        { "rid", L"SONICblue/rid", STRING },
        { "year", L"WM/Year", STRING },
        { "tracknr", L"WM/Track", NUMBER_ZERO_BASED },
        { "tracknr", L"WM/TrackNumber", STRING },
        { NULL, NULL }
    };

    ASFTagWriterImpl::~ASFTagWriterImpl()
    {
    }

    inline STATUS ASFTagWriterImpl::Create(const tstring &filename, TagWriter **ppWriter)
    {
        ASFTagWriterImpl *p = NEW ASFTagWriterImpl(filename);
        if (p)
        {
            *ppWriter = p;
            return S_OK;
        }
        else
            return E_OUTOFMEMORY;
    }

    STATUS ASFTagWriterImpl::Write()
    {
        WMAInfoWrapper fi;
        HRESULT hr = fi.Open(util::UTF16FromT(m_filename));
        if (SUCCEEDED(hr))
        {
            for(std::map<std::string, std::wstring>::const_iterator i = m_tags.begin(); i != m_tags.end(); ++i)
            {
		const TagTranslationTable *t = m_translation_table;
		bool found = false;
		while (t->empeg_tag_name)
		{
		    if (stricmp(t->empeg_tag_name, i->first.c_str()) == 0)
		    {
			TRACE("Setting empeg_tag:%s, asf_tag:%ls, value:%ls\n",
			    i->first.c_str(), t->asf_tag_name, i->second.c_str());
			found = true;

			switch(t->asf_tag_type)
			{
			case STRING:
			    hr = fi.SetAttribute(t->asf_tag_name, i->second);
			    break;
			case NUMBER:
			case NUMBER_ZERO_BASED:
			    {
				// First convert the string to a number.
				DWORD dw = wcstoul(i->second.c_str(), NULL, 0);

				if (t->asf_tag_type == NUMBER_ZERO_BASED)
				    --dw;
				hr = fi.SetAttribute(t->asf_tag_name, dw);
				break;
			    }
			default:
			    ASSERT(false);
			}
			if (FAILED(hr))
			{
			    TRACE_WARN("Failed to set tag \'%s\' to \'%s\', hr=0x%08x\n", i->first.c_str(), i->second.c_str(), hr);
			    break;
			}
		    }
		    ++t;
		}
                
		if (!found)
                {
                    TRACE_WARN("No ASF translation of empeg tag \'%s\'\n", i->first.c_str());
                }
            }

            if (SUCCEEDED(hr))
            {
                hr = fi.Commit();
            }

            fi.Close();
        }
        else
        {
            TRACE_WARN("Failed to open: 0x%08x\n", hr);
        }

	// "Read-only file" must be this special error to get the special error message
	if (hr == 0xC00D0018)
	    hr = MakeErrnoStatus(EACCES);

        return hr;
    }

    STATUS ASFTagWriterImpl::SetTag(const char *tag, const UTF8CHAR *value)
    {
        m_tags[tag] = util::UTF16FromUTF8(value);
        return S_OK;
    }

    STATUS ASFTagWriter::Create(const tstring &filename, TagWriter **ppWriter)
    {
        return ASFTagWriterImpl::Create(filename, ppWriter);
    }
}
#endif