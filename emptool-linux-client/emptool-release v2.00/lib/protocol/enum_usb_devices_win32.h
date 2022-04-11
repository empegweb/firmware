/* enum_usb_devices_win32.h
 *
 * (C) 1998-2000 empeg ltd, http://www.empeg.com
 *
 * (:Empeg Source Release 1.3 01-Apr-2003 18:52 rob:)
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 */

#if !defined(AFX_EMPEGUSBDEVICES_H__99C65D60_0D82_11D3_9129_00104B62B7D4__INCLUDED_)
#define AFX_EMPEGUSBDEVICES_H__99C65D60_0D82_11D3_9129_00104B62B7D4__INCLUDED_

#pragma warning(disable : 4786)

#include <vector>
#include <string>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// We have to be rather sneaky here since some of the symbols we're using
// aren't available on NT4.0 or Windows 95. We have to really dynamically
// link with the symbols.

class CEmpegUsbDevices  
{
    typedef std::vector<std::string> StringVector;
    StringVector m_devices;

    void FindDevices();
    void AppendDevice(const std::string &str);

public:
    typedef StringVector::const_iterator const_iterator;

    CEmpegUsbDevices();
    virtual ~CEmpegUsbDevices();

    const_iterator begin() const { return m_devices.begin(); }
    const_iterator end() const { return m_devices.end(); }
    int GetCount() const { return m_devices.size(); }

    std::string operator[](int i) const
    {
#ifdef ASSERT
	ASSERT(i >= 0);
	ASSERT(i < GetCount());
#endif
	return m_devices[i];
    }

    std::string GetDummy() const;
    static bool IsUsbSupported();
};

#endif // !defined(AFX_EMPEGUSBDEVICES_H__99C65D60_0D82_11D3_9129_00104B62B7D4__INCLUDED_)
