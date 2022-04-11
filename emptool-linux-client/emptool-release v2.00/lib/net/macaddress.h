/* macaddress.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.13 01-Apr-2003 18:52 rob:)
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 */

#ifndef MACADDRESS_H
#define MACADDRESS_H 1

#include <stdio.h>
#include <string.h>
#include <string>

class MACAddress
{
    BYTE b[6];

    void FromString(const char*s)
    {
        int i[6];
	sscanf(s, "%x:%x:%x:%x:%x:%x", &i[0], &i[1], &i[2], &i[3], &i[4], &i[5]);
        for ( int j=0; j<6; j++ )
            b[j] = i[j];
    }

public:
    MACAddress(BYTE u, BYTE v, BYTE w, BYTE x, BYTE y, BYTE z)
    {
	b[0] = u;
	b[1] = v;
	b[2] = w;
	b[3] = x;
	b[4] = y;
	b[5] = z;
    }
    MACAddress(unsigned cb, const BYTE *p)
    {
#ifndef WIN32
        using std::min;
#endif
	memset(b, 0, 6);
	memcpy(b, p, min(6U, cb));
    }
    explicit MACAddress(const std::string &s)
    {
	FromString(s.c_str());
    }
    explicit MACAddress(const char *s)
    {
	FromString(s);
    }

    std::string ToString(char sep = ':') const
    {
	char buffer[24];
	sprintf(buffer, "%02x%c%02x%c%02x%c%02x%c%02x%c%02x",
	        b[0], sep, b[1], sep, b[2], sep, b[3], sep, b[4], sep, b[5]);
	return std::string(buffer);
    }

    int Compare(const MACAddress &other) const
    {
	return memcmp(b, other.b, sizeof(b));
    }
    bool operator==(const MACAddress &other) const
    {
	return Compare(other) == 0;
    }
    bool operator<(const MACAddress &other) const
    {
	return Compare(other) < 0;
    }

#ifdef WIN32
    bool IsPPP() const
    {
	BYTE prefix[4] = { '\x44', '\x45', '\x53', '\x54', };
	return memcmp(b, prefix, 4) == 0;
    }
#endif
};

#endif
