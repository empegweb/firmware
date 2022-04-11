/* empeg_bitrate.cpp
 *
 * Easy conversion of empeg format bitrates to numbers.
 *
 * (C) 2003 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.4 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *
 */

#include "config.h"
#include "empeg_bitrate.h"
#include "var_string.h"

STATUS EmpegBitrate::GetVBR(bool *vbr) const
{
    if (m_string.length() > 2)
    {
	switch (tolower(m_string[0]))
	{
	case 'v':
	    *vbr = true;
	    return S_OK;
	
	case 'f':
	    *vbr = false;
	    return S_OK;
	}
    }
    return E_CORRUPTED;
}

STATUS EmpegBitrate::GetChannels(unsigned *channels) const
{
    if (m_string.length() > 2)
    {
	switch (tolower(m_string[1]))
	{
	case 'm':
	    *channels = 1;
	    return S_OK;

	case 's':
	    *channels = 2;
	    return S_OK;

	};
    }

    return E_CORRUPTED;
}

STATUS EmpegBitrate::GetBitsPerSecond(unsigned *bps) const
{
    if (m_string.length() > 2)
    {
	char *end;
	*bps = strtoul(m_string.c_str() + 2, &end, 0) * 1000;
	if (end == m_string.c_str() + 2)
	    return E_CORRUPTED;
	else
	    return S_OK;
    }
    else
	return E_CORRUPTED;
}

void EmpegBitrate::SetVBR(bool vbr)
{
    const char v = (vbr ? 'v' : 'f');

    if (m_string.length() >= 1)
    {
	m_string[0] = v;
    }
    else
    {
	m_string = v;
    }
}

void EmpegBitrate::SetChannels(unsigned channels)
{
    char c;
    switch (channels)
    {
    case 1:
	c = 'm';
	break;

    case 2:
	c = 's';
	break;

    default:
	c = '?';
	break;
    }

    while (m_string.length() < 2)
	m_string += '?';

    m_string[1] = c;
}

void EmpegBitrate::SetBitsPerSecond(unsigned int bps)
{
    std::string b = VarString::Printf("%u", bps / 1000);
    if (m_string.length() < 2)
    {
	while (m_string.length() < 2)
	    m_string += '?';
    }
    else
    {
	m_string.erase(m_string.begin() + 2, m_string.end());
    }

    m_string += b;
}

#if defined(TEST)

inline void ASSERTEQ(unsigned lhs, unsigned rhs)
{
    ASSERT_EX(lhs == rhs, "lhs=%u, rhs=%u\n", lhs, rhs);
}

int main()
{
    bool b;
    unsigned n;
    
    EmpegBitrate br1("fs128");
    ASSERT(SUCCEEDED(br1.GetVBR(&b)));
    ASSERTEQ(br1.IsVBR(), false);
    ASSERTEQ(br1.GetChannels(), 2);
    ASSERTEQ(br1.GetBitsPerSecond(), 128000);

    EmpegBitrate br2("vm320");
    ASSERT(SUCCEEDED(br2.GetVBR(&b)));
    ASSERT(br2.IsVBR() == true);
    ASSERT(br2.GetChannels() == 1);
    ASSERT(br2.GetBitsPerSecond() == 320000);

    EmpegBitrate br3("???");
    
    ASSERT(FAILED(br3.GetVBR(&b)));
    ASSERT(FAILED(br3.GetChannels(&n)));
    ASSERT(FAILED(br3.GetBitsPerSecond(&n)));
    
    EmpegBitrate br4;    
    ASSERT(FAILED(br4.GetVBR(&b)));
    ASSERT(FAILED(br4.GetChannels(&n)));
    ASSERT(FAILED(br4.GetBitsPerSecond(&n)));
    
    br1.SetVBR(true);
    br1.SetChannels(1);
    br1.SetBitsPerSecond(64000);
    ASSERT(SUCCEEDED(br1.GetVBR(&b)));
    ASSERT(br1.IsVBR() == true);
    ASSERTEQ(br1.GetChannels(), 1);
    ASSERT(br1.GetBitsPerSecond() == 64000);
    ASSERT(br1.ToString() == "vm64");

    br3.SetVBR(false);
    br3.SetChannels(2);
    br3.SetBitsPerSecond(192000);
    ASSERT(SUCCEEDED(br3.GetVBR(&b)));
    ASSERT(br3.IsVBR() == false);
    ASSERT(br3.GetChannels() == 2);
    ASSERT(br3.GetBitsPerSecond() == 192000);
    ASSERT(br3.ToString() == "fs192");

    br4.SetBitsPerSecond(24000);
    br4.SetVBR(true);
    br4.SetChannels(1);
    ASSERT(SUCCEEDED(br4.GetVBR(&b)));
    ASSERT(br4.IsVBR() == true);
    ASSERT(br4.GetChannels() == 1);
    ASSERT(br4.GetBitsPerSecond() == 24000);
    ASSERT(br4.ToString() == "vm24");

    EmpegBitrate br5;    
    br5.SetBitsPerSecond(48000);
    br5.SetChannels(2);
    br5.SetVBR(false);
    ASSERT(SUCCEEDED(br5.GetVBR(&b)));
    ASSERT(br5.IsVBR() == false);
    ASSERT(br5.GetChannels() == 2);
    ASSERT(br5.GetBitsPerSecond() == 48000);
    ASSERT(br5.ToString() == "fs48");

    EmpegBitrate br6;
    br6.SetBitsPerSecond(224000);
    br6.SetChannels(2);
    br6.SetVBR(true);
    ASSERT(SUCCEEDED(br6.GetVBR(&b)));
    ASSERT(br6.IsVBR() == true);
    ASSERT(br6.GetChannels() == 2);
    ASSERT(br6.GetBitsPerSecond() == 224000);
    ASSERT(br6.ToString() == "vs224");

    return 0;
}

#endif // TEST
