/* wmainfo.cpp
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * Author:
 *   Mike Crowe <mcrowe@sonicblue.com>
 *
 * (:Empeg Source Release 1.7 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "wmainfo.h"
#include <windows.h>
#include <malloc.h>
#include "stringops.h"

// Arse, asferr.h defines this too. Luckily it defines it to mean the same thing.
#undef STATUS_SEVERITY
#include <wmsdk.h>

#define TRACE_ASFTAGS 1

namespace tags_internal
{
    WMAInfoWrapper::WMAInfoWrapper()
        : m_editor(NULL), m_info(NULL), m_profile(NULL), m_stream(0)
    {
    }

    WMAInfoWrapper::~WMAInfoWrapper()
    {
        Close();
    }

    void WMAInfoWrapper::Close()
    {
        TRACE("enter WMAInfoWrapper::Close\n");
        if (m_info)
        {
	    m_info->Release();
	    m_info = NULL;
        }
        if (m_editor)
        {
	    TRACE("Closing reader\n");
	    m_editor->Close();

	    // This is required otherwise the Release sometimes hangs.
	    Sleep(100);
	    TRACE("Losing reference on reader\n");
	    int n = m_editor->Release();
	    TRACE("There are %d other holders.\n", n);
	    m_editor = NULL;
        }
        if (m_profile)
        {
	    m_profile->Release();
	    m_profile = NULL;
        }
        TRACE("leave WMAInfoWrapper::Close\n");
    }

    HRESULT WMAInfoWrapper::Open(const std::wstring &filename)
    {
        TRACEC(TRACE_ASFTAGS, "About to create tag editor.\n");
        HRESULT hr = WMCreateEditor(&m_editor);
        TRACEC(TRACE_ASFTAGS, "Tag editor creation result: 0x%08x\n", hr);
        if (SUCCEEDED(hr))
        {
	    TRACE("Created reader object\n");

	    // We don't need to AddRef here, IWMMetadataEditor::Open will.
	    hr = m_editor->Open(filename.c_str());
	    if (SUCCEEDED(hr))
	    {
	        TRACE("Open succeeded. Result=0x%x\n", hr);
		hr = m_editor->QueryInterface(IID_IWMHeaderInfo, reinterpret_cast<void **>(&m_info));
		if (SUCCEEDED(hr))
		{
                    TRACE("Got IWMHeaderInfo interface.\n");
#ifdef _DEBUG
                    LogAttributes();
#endif
		}
	    }
	    else
	    {
	        if (hr == NS_E_PROTECTED_CONTENT)
	        {
		    TRACE("Open failed saying that the file was protected\n");
		    //protected_file = true;
	        }
	        else
	        {
		    TRACE("Open failed with some other error: 0x%x\n", hr);
	        }
	    }
        }

        if (FAILED(hr))
            Close();

        return hr;
    }

    std::wstring WMAInfoWrapper::GetAttributeString(LPCWSTR wszName)
    {
        std::wstring result;
        HRESULT hr = GetAttribute(wszName, &result);
        if (FAILED(hr))
            result.erase();

        return result;
    }

    HRESULT WMAInfoWrapper::GetAttribute(LPCWSTR wszName, std::wstring *result)
    {
        WORD cbValueLen = 0;
        WMT_ATTR_DATATYPE type;

        // First find out how long the string is.
        HRESULT hr = m_info->GetAttributeByName(&m_stream, wszName, &type, NULL, &cbValueLen);
        if (SUCCEEDED(hr) && type == WMT_TYPE_STRING)
        {
	    BYTE *pbValue = reinterpret_cast<BYTE *>(_alloca(cbValueLen + sizeof(WCHAR)));
	    hr = m_info->GetAttributeByName(&m_stream, wszName, &type, pbValue, &cbValueLen);
	    if (SUCCEEDED(hr))
	    {
                /// @todo: check if this is accidentally including the NULL terminator.
	        WCHAR *begin = reinterpret_cast<WCHAR *>(pbValue);
                *result = std::wstring(begin, cbValueLen/sizeof(WCHAR));
	    }
        }
        else
        {
            hr = TYPE_E_TYPEMISMATCH;
        }
        return hr;
    }

    DWORD WMAInfoWrapper::GetAttributeNumber(LPCWSTR wszName)
    {
        DWORD result;
        HRESULT hr = GetAttribute(wszName, &result);
        if (FAILED(hr))
            result = 0;

        return result;
    }

    HRESULT WMAInfoWrapper::GetAttribute(LPCWSTR wszName, DWORD *result)
    {
        WORD cbValueLen = 0;
        WMT_ATTR_DATATYPE type;

        // First find out how long the string is.
        HRESULT hr = m_info->GetAttributeByName(&m_stream, wszName, &type, NULL, &cbValueLen);
        if (SUCCEEDED(hr) && type == WMT_TYPE_DWORD && cbValueLen == sizeof(DWORD))
        {
	    hr = m_info->GetAttributeByName(&m_stream, wszName, &type, reinterpret_cast<BYTE *>(result), &cbValueLen);
        }
        else
        {
            hr = TYPE_E_TYPEMISMATCH;
        }
        return hr;
    }

    LONGLONG WMAInfoWrapper::GetAttributeQuad(LPCWSTR wszName)
    {
        LONGLONG result;
        HRESULT hr = GetAttribute(wszName, &result);
        if (FAILED(hr))
        {
            TRACE_WARN("Failed to get quad tag \'%ws\': 0x%08x\n", wszName, hr);
            result = 0;
        }
        return result;
    }

    HRESULT WMAInfoWrapper::GetAttribute(LPCWSTR wszName, LONGLONG *result)
    {
        WORD cbValueLen = 0;
        WMT_ATTR_DATATYPE type;

        // First find out how long the string is.
        HRESULT hr = m_info->GetAttributeByName(&m_stream, wszName, &type, NULL, &cbValueLen);
        if (FAILED(hr))
	    return hr;

	if (type == WMT_TYPE_QWORD && cbValueLen == sizeof(QWORD))
	{
	    QWORD qwValue;
	    hr = m_info->GetAttributeByName(&m_stream, wszName, &type, reinterpret_cast<BYTE *>(&qwValue), &cbValueLen);
	    if (SUCCEEDED(hr))
	    {
		*result = qwValue;
	    }
	}
	else if (type == WMT_TYPE_DWORD && cbValueLen == sizeof(DWORD))
	{
	    // It's a DWORD but we can cope.
	    DWORD dwValue;
	    hr = m_info->GetAttributeByName(&m_stream, wszName, &type, reinterpret_cast<BYTE *>(&dwValue), &cbValueLen);
	    if (SUCCEEDED(hr))
	    {
		*result = dwValue;
	    }
	}

	return hr;
    }


    bool WMAInfoWrapper::GetAttributeBoolean(LPCWSTR wszName)
    {
        bool result;
        HRESULT hr = GetAttribute(wszName, &result);
        if (FAILED(hr))
        {
            TRACE_WARN("Failed to get attribute named '%ls': 0x%08x\n", wszName, hr);
            result = false;
        }
        return result;
    }

    HRESULT WMAInfoWrapper::GetAttribute(LPCWSTR wszName, bool *result)
    {
        WORD cbValueLen = 0;
        WMT_ATTR_DATATYPE type;

        // First find out how long the string is.
        HRESULT hr = m_info->GetAttributeByName(&m_stream, wszName, &type, NULL, &cbValueLen);
        if (SUCCEEDED(hr) && type == WMT_TYPE_BOOL && cbValueLen == sizeof(BOOL))
        {
            BOOL value;
	    hr = m_info->GetAttributeByName(&m_stream, wszName, &type, reinterpret_cast<BYTE *>(&value), &cbValueLen);
	    if (SUCCEEDED(hr))
	    {
	        *result = value ? true : false;
	    }
        }
        return hr;
    }

    HRESULT WMAInfoWrapper::SetAttribute(LPCWSTR tag, const std::wstring &value)
    {
        return m_info->SetAttribute(m_stream, tag, WMT_TYPE_STRING, reinterpret_cast<const BYTE *>(value.c_str()), (value.length() + 1) * sizeof(WCHAR));
    }

    HRESULT WMAInfoWrapper::SetAttribute(LPCWSTR tag, DWORD value)
    {
        return m_info->SetAttribute(m_stream, tag, WMT_TYPE_DWORD, reinterpret_cast<const BYTE *>(&value), sizeof(DWORD));
    }

    #define LOG_ATTRIBUTE_VALUE(S, T) TRACE("Attribute('%ls')=" S " (" STRIZE(T) ")\n", wszName, *reinterpret_cast<T *>(pValue))
    #define LOG_ATTRIBUTE_PTR(S, T) TRACE("Attribute('%ls')=" S " (" STRIZE(T) ")\n", wszName, reinterpret_cast<T *>(pValue))

#ifdef _DEBUG
    void WMAInfoWrapper::LogAttributes()
    {
        HRESULT hr;
        WORD count;
        WORD stream = 0;

        hr = m_info->GetAttributeCount(0, &count);

        TRACE("This file has %hd attributes\n", count);

        for(WORD i = 0; i < count; ++i)
        {
            WORD cchNameLen = 0;
            WORD cbValue = 0;
            WMT_ATTR_DATATYPE type;

            hr = m_info->GetAttributeByIndex(i, &stream, NULL, &cchNameLen, &type, NULL, &cbValue);
            if (SUCCEEDED(hr))
            {
                // We did it, better do the reading then.
                LPWSTR wszName = new WCHAR[cchNameLen];
                LPBYTE pValue = reinterpret_cast<LPBYTE>(operator new(cbValue));

                hr = m_info->GetAttributeByIndex(i, &stream, wszName, &cchNameLen, &type, pValue, &cbValue);
                if (SUCCEEDED(hr))
                {
                    // Right, we've got it and the data, let's check the type and display it.
                    switch (type)
                    {
                    case WMT_TYPE_DWORD:
                        LOG_ATTRIBUTE_VALUE("0x%08x", DWORD);
                        break;
                    case WMT_TYPE_STRING:
                        LOG_ATTRIBUTE_PTR("'%ls'", WCHAR);
                        break;
                    case WMT_TYPE_BINARY:
                        TRACE("Attribute('%ls')=BINARY DATA\n", wszName);
                        break;
                    case WMT_TYPE_BOOL:
                        LOG_ATTRIBUTE_VALUE("%d", BOOL);
                        break;
                    case WMT_TYPE_QWORD:
                        LOG_ATTRIBUTE_VALUE("%I64d", QWORD);
                        break;
                    case WMT_TYPE_WORD:
                        LOG_ATTRIBUTE_VALUE("0x%04hd", WORD);
                        break;
                    case WMT_TYPE_GUID:
                        TRACE("Attribute('%ls')=GUID\n", wszName);
                        break;
                    default:
                        TRACE("Attribute('%ls')=Unknown type %d\n", wszName, type);
                    }
                }
                else
                {
                    TRACE("Failed to get attribute for index %hd which claimed to have a name of length %hd and a data length of %hd, hr=0x%08x\n",
                             i, cchNameLen, cbValue, hr);
                }
                delete []wszName;
                delete pValue;
            }
            else
            {
                TRACE("Failed to get length information for index %hd hr=0x%08x\n", i, hr);
            }
        }
    }
#endif

    bool WMAInfoWrapper::HasDRM()
    {
        return GetAttributeBoolean(g_wszWMUse_DRM);
    }

    DWORD WMAInfoWrapper::GetDurationMs()
    {
        QWORD result = GetAttributeQuad(g_wszWMDuration);

        // It's in 100ns units for some reason.
        result /= (10000);

        if (result > UINT_MAX)
	    return UINT_MAX;
        else
	    return static_cast<DWORD>(result);
    }

    DWORD WMAInfoWrapper::GetBitrate()
    {
        // We should really do it like this, but the WMMetaDataEditor object won't let us get at the stream :-(
#if 0
        IWMStreamConfig *pConfig;

        HRESULT hr = m_profile->GetStream(stream, &pConfig);
        if (SUCCEEDED(hr))
        {
	    DWORD dw;
            hr = pConfig->GetBitrate(&dw);
	    pConfig->Release();
	    if (SUCCEEDED(hr))
	    {
	        return dw;
	    }
        }
        return 0;
#else
        return GetAttributeNumber(g_wszWMBitrate);
#endif
    }

    HRESULT WMAInfoWrapper::Commit()
    {
        return m_editor->Flush();
    }
}

