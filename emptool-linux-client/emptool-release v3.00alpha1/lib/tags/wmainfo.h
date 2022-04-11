/* wmainfo.h
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * Author:
 *   Mike Crowe <mcrowe@sonicblue.com>
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 */

#ifndef WMAINFO_H
#define WMAINFO_H 1

#include <string>
#include <windows.h>
// #include <wmsdk.h>

struct IWMMetadataEditor;
struct IWMHeaderInfo;
struct IWMProfile;

namespace tags_internal
{
    class WMAInfoWrapper
    {
        IWMMetadataEditor *m_editor;
        IWMHeaderInfo *m_info;
        IWMProfile *m_profile;

        WORD m_stream;

    public:
        WMAInfoWrapper();
        ~WMAInfoWrapper();
        HRESULT Open(const std::wstring &filename);
        HRESULT Commit();
        void Close();

        DWORD GetBitrate();
        bool HasDRM();
        DWORD GetDurationMs();
        bool IsProtected();

        void LogAttributes();

        HRESULT SetAttribute(LPCWSTR name, const std::wstring &value);
        HRESULT SetAttribute(LPCWSTR name, DWORD value);
        HRESULT SetAttribute(LPCWSTR name, bool value);
        HRESULT SetAttribute(LPCWSTR name, LONGLONG value);
        HRESULT GetAttribute(LPCWSTR name, std::wstring *value);
        HRESULT GetAttribute(LPCWSTR name, DWORD *value);
        HRESULT GetAttribute(LPCWSTR name, bool *value);
        HRESULT GetAttribute(LPCWSTR name, LONGLONG *value);

        // These functions return the default value if things fail.
        std::wstring GetAttributeString(LPCWSTR);
        DWORD GetAttributeNumber(LPCWSTR);
        LONGLONG GetAttributeQuad(LPCWSTR);
        bool GetAttributeBoolean(LPCWSTR);

    };
}

#endif
