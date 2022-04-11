/* empeg_bitrate.h
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

#ifndef EMPEG_BITRATE_H
#define EMPEG_BITRATE_H 1

#include <string>
#include "empeg_error.h"

/** An easy way to convert between empeg "fs128" format bitrate
 ** specifiers and its constituent parts. The benefit of this is
 ** that it is perfectly safe and _will_not_crash_ unlike other
 ** arbitrary bits of code that are scattered around which
 ** are supposed to do the same thing but die horribly on empty
 ** strings.
 **/
class EmpegBitrate
{
    std::string m_string;

public:
    explicit EmpegBitrate(const std::string &t)
	: m_string(t) {}
    EmpegBitrate() {}

    std::string ToString() const { return m_string; }

    STATUS GetVBR(bool *vbr) const;
    STATUS GetChannels(unsigned *channels) const;
    STATUS GetBitsPerSecond(unsigned *bps) const;

    bool IsVBR() const;
    unsigned GetChannels() const;
    unsigned GetBitsPerSecond() const;

    void SetVBR(bool b);
    void SetChannels(unsigned c);
    void SetBitsPerSecond(unsigned bps);
};

inline bool EmpegBitrate::IsVBR() const
{
    bool vbr;
    if (SUCCEEDED(GetVBR(&vbr)))
	return vbr;
    else
	return false;
}

inline unsigned EmpegBitrate::GetChannels() const
{
    unsigned channels;
    if (SUCCEEDED(GetChannels(&channels)))
	return channels;
    else
	return 0;
}

inline unsigned EmpegBitrate::GetBitsPerSecond() const
{
    unsigned bps;
    if (SUCCEEDED(GetBitsPerSecond(&bps)))
	return bps;
    else
	return 0;
}

#endif // EMPEG_BITRATE_H


