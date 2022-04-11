/* bit_bucket.h
 *
 * (C) 2003 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 */

#ifndef BIT_BUCKET_H
#define BIT_BUCKET_H	1

#include <stdio.h>

class BitBucketMemoryIn
{
    const unsigned char *m_buf;
    unsigned m_bytes;
    unsigned char m_byte;
    unsigned m_offset;
    bool m_eof;
    void NextBit();
public:
    BitBucketMemoryIn(const unsigned char *buf, unsigned bytes);
    unsigned GetBit();
    unsigned GetBits(unsigned bits);
    unsigned GetByte();
    bool IsEof();
};

class BitBucketFileOut
{
    FILE *m_str;
    unsigned char m_byte;
    unsigned m_offset;
public:
    BitBucketFileOut(FILE *str);
    void PutBit(unsigned bit);
    void PutBits(unsigned word, unsigned bits);
    void PutByte(unsigned byte);
    void Flush();
};

#endif
