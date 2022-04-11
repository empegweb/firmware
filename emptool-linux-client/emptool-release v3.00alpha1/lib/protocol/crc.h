/* crc.h
 *
 * Class encapsulating lots of old Zmodem code for CRC16 and CRC32
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 13-Mar-2003 18:15 rob:)
 */

#ifndef _CRC_H
#define _CRC_H

class CRC
{
public:
    CRC() {}
    ~CRC() {}

    // Make both flavours of CRC of a buffer
    static unsigned short CRC16(const unsigned char *buffer, unsigned length);
    static unsigned long CRC32(const unsigned char *buffer, unsigned length);

    // Update CRC with a block of data
    static unsigned long CRC32(unsigned long crc,
			       const unsigned char *buffer, unsigned length);

private:
    static const unsigned short crc16tab[];
    static const unsigned long crc32tab[];
};

#endif
