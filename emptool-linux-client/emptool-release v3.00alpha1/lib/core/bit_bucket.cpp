/* bit_bucket.cpp
 *
 * (C) 2003 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "bit_bucket.h"

BitBucketMemoryIn::BitBucketMemoryIn(const unsigned char *buf, unsigned bytes)
    : m_buf(buf), m_bytes(bytes), m_byte(0), m_offset(0), m_eof(false)
{
    if(m_bytes)
    {
	m_byte = *m_buf++;
	--m_bytes;
    }
    else
	m_eof = true;
}

unsigned BitBucketMemoryIn::GetBit()
{
    ASSERT(!IsEof());
    unsigned bit = (m_byte >> m_offset) & 1;
    NextBit();
    return bit;
}

unsigned BitBucketMemoryIn::GetBits(unsigned bits)
{
    unsigned word = 0;
    for(unsigned i = 0; i < bits; ++i)
	word |= GetBit() << i;
    return word;
}

unsigned BitBucketMemoryIn::GetByte()
{
    return GetBits(8);
}

void BitBucketMemoryIn::NextBit()
{
    if(++m_offset == 8)
    {
	m_offset = 0;
	if(m_bytes)
	{
	    m_byte = *m_buf++;
	    --m_bytes;
	}
	else
	    m_eof = true;
    }
}

bool BitBucketMemoryIn::IsEof()
{
    return m_eof;
}



BitBucketFileOut::BitBucketFileOut(FILE *str)
    : m_str(str), m_byte(0), m_offset(0)
{
}

void BitBucketFileOut::PutBit(unsigned bit)
{
    bit = bit ? 1 : 0;
    m_byte |= bit << m_offset;
    if(++m_offset == 8)
	Flush();
}

void BitBucketFileOut::PutBits(unsigned word, unsigned bits)
{
    for(unsigned i = 0; i < bits; ++i)
	PutBit((word >> i) & 1);
}
    
void BitBucketFileOut::PutByte(unsigned byte)
{
    PutBits(byte, 8);
}

void BitBucketFileOut::Flush()
{
    fputc(m_byte, m_str);
    m_byte = 0;
    m_offset = 0;
}

#if defined(TEST)
int main(void)
{
    const char *hello = "Hello there this is quite obviously a test\n";
    unsigned bytes = strlen(hello);
    BitBucketMemoryIn in((const unsigned char *) hello, bytes);
    BitBucketFileOut out(stdout);

    unsigned left = bytes * 8;
    while(left)
    {
	unsigned n = (random() % 32) + 1; // 1..32 inclusive
	if(n > left)
	    n = left;
	unsigned bits = in.GetBits(n);
	out.PutBits(bits, n);
	left -= n;
    }
    out.Flush();
    
    return 0;
}
#endif
