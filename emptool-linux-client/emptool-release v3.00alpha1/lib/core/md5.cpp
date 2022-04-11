/* This code is copyright (C) Mike Crowe and non-exclusively licensed to
 * empeg Ltd to use for whatever it wishes.
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd
 * or from Mike Crowe.
 *
 * (:Empeg Source Release 1.5 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "md5.h"
#include <string>

//#define LITTLE_ENDIAN 1

#ifdef WIN32
#define X86 1
#else
#define X86 0
#endif

// This algorithm only works on bytes worth of data, not bits worth.

typedef unsigned long WORD32;

static const WORD32 T[65] =
{
	0,
	0xd76aa478UL, 0xe8c7b756UL, 0x242070dbUL, 0xc1bdceeeUL,
	0xf57c0fafUL, 0x4787c62aUL, 0xa8304613UL, 0xfd469501UL,
	0x698098d8UL, 0x8b44f7afUL, 0xffff5bb1UL, 0x895cd7beUL,
	0x6b901122UL, 0xfd987193UL, 0xa679438eUL, 0x49b40821UL,
	0xf61e2562UL, 0xc040b340UL, 0x265e5a51UL, 0xe9b6c7aaUL,
	0xd62f105dUL, 0x02441453UL, 0xd8a1e681UL, 0xe7d3fbc8UL,
	0x21e1cde6UL, 0xc33707d6UL, 0xf4d50d87UL, 0x455a14edUL,
	0xa9e3e905UL, 0xfcefa3f8UL, 0x676f02d9UL, 0x8d2a4c8aUL,
	0xfffa3942UL, 0x8771f681UL, 0x6d9d6122UL, 0xfde5380cUL,
	0xa4beea44UL, 0x4bdecfa9UL, 0xf6bb4b60UL, 0xbebfbc70UL,
	0x289b7ec6UL, 0xeaa127faUL, 0xd4ef3085UL, 0x04881d05UL,
	0xd9d4d039UL, 0xe6db99e5UL, 0x1fa27cf8UL, 0xc4ac5665UL,
	0xf4292244UL, 0x432aff97UL, 0xab9423a7UL, 0xfc93a039UL,
	0x655b59c3UL, 0x8f0ccc92UL, 0xffeff47dUL, 0x85845dd1UL,
	0x6fa87e4fUL, 0xfe2ce6e0UL, 0xa3014314UL, 0x4e0811a1UL,
	0xf7537e82UL, 0xbd3af235UL, 0x2ad7d2bbUL, 0xeb86d391UL
};

static inline WORD32 F(WORD32 X, WORD32 Y, WORD32 Z)
{
	return (X & Y) | ~X & Z;
}

static inline WORD32 G(WORD32 X, WORD32 Y, WORD32 Z)
{
	return (X & Z) | Y & ~Z;
}

static inline WORD32 H(WORD32 X, WORD32 Y, WORD32 Z)
{
	return (X ^ Y ^ Z);
}

static inline WORD32 I(WORD32 X, WORD32 Y, WORD32 Z)
{
	return (Y ^ (X | ~Z));
}

static inline WORD32 ROL(WORD32 x, WORD32 n)
{
#if X86 && defined(__GNUC__)
	asm("roll %%cl,%0" : "=r" (x) : "0" (x), "c" (n));
	return x;
#else
	return (x << n) | (x >> (32-n));
#endif
}

static inline void FF(WORD32 &a, WORD32 &b, WORD32 &c, WORD32 &d, WORD32 x, WORD32 s, WORD32 i)
{
	a = b + ROL((a + F(b, c, d) + x + i), s);
}

static inline void GG(WORD32 &a, WORD32 &b, WORD32 &c, WORD32 &d, WORD32 x, WORD32 s, WORD32 i)
{
	a = b + ROL((a + G(b, c, d) + x + i), s);
}

static inline void HH(WORD32 &a, WORD32 &b, WORD32 &c, WORD32 &d, WORD32 x, WORD32 s, WORD32 i)
{
	a = b + ROL((a + H(b, c, d) + x + i), s);
}

static inline void II(WORD32 &a, WORD32 &b, WORD32 &c, WORD32 &d, WORD32 x, WORD32 s, WORD32 i)
{
	a = b + ROL((a + I(b, c, d) + x + i), s);
}


void MessageDigest::Digest(const char *input, long length)
{
	if (length < 0)
		length = strlen(input);

	WORD32 *word_buffer;
	WORD32 buflen;

	// Padding
	long padding = 64 - ((length + 8) % 64);
	if (padding < 0)
		padding += 64;

	buflen = length + padding + 8;
	word_buffer = NEW WORD32[buflen/4];
	memcpy((void *)word_buffer, input, length);

	char *p = (char *)word_buffer + length;

	*p++ = '\x80';
	while (--padding)
		*p++ = '\x00';

	WORD32 *q = word_buffer + buflen/4 - 2;
	*q++ = length << 3;
	*q++ = length >> 29;

	WORD32 A = 0x67452301;
	WORD32 B = 0xefcdab89;
	WORD32 C = 0x98badcfe;
	WORD32 D = 0x10325476;

	WORD32 A2,B2,C2,D2;
	for(unsigned long i = 0; i < buflen/4; i+=16)
	{
		q = word_buffer + i;

		A2 = A, B2 = B, C2 = C, D2 = D;

		FF(A, B, C, D, q[ 0],  7, T[ 1]); // ABCD  0  7  1
		FF(D, A, B, C, q[ 1], 12, T[ 2]); // DABC  1 12  2
		FF(C, D, A, B, q[ 2], 17, T[ 3]); // CDAB  2 17  3
		FF(B, C, D, A, q[ 3], 22, T[ 4]); // BCDA  3 22  4
		FF(A, B, C, D, q[ 4],  7, T[ 5]); // ABCD  4  7  5
		FF(D, A, B, C, q[ 5], 12, T[ 6]); // DABC  5 12  6
		FF(C, D, A, B, q[ 6], 17, T[ 7]); // CDAB  6 17  7
		FF(B, C, D, A, q[ 7], 22, T[ 8]); // BCDA  7 22  8
		FF(A, B, C, D, q[ 8],  7, T[ 9]); // ABCD  8  7  9
		FF(D, A, B, C, q[ 9], 12, T[10]); // DABC  9 12 10
		FF(C, D, A, B, q[10], 17, T[11]); // CDAB 10 17 11
		FF(B, C, D, A, q[11], 22, T[12]); // BCDA 11 22 12
		FF(A, B, C, D, q[12],  7, T[13]); // ABCD 12  7 13
		FF(D, A, B, C, q[13], 12, T[14]); // DABC 13 12 14
		FF(C, D, A, B, q[14], 17, T[15]); // CDAB 14 17 15
		FF(B, C, D, A, q[15], 22, T[16]); // BCDA 15 22 16

		GG(A, B, C, D, q[ 1],  5, T[17]); // ABCD  1  5 17
		GG(D, A, B, C, q[ 6],  9, T[18]); // DABC  6  9 18
		GG(C, D, A, B, q[11], 14, T[19]); // CDAB 11 14 19
		GG(B, C, D, A, q[ 0], 20, T[20]); // BCDA  0 20 20
		GG(A, B, C, D, q[ 5],  5, T[21]); // ABCD  5  5 21
		GG(D, A, B, C, q[10],  9, T[22]); // DABC 10  9 22
		GG(C, D, A, B, q[15], 14, T[23]); // CDAB 15 14 23
		GG(B, C, D, A, q[ 4], 20, T[24]); // BCDA  4 20 24
		GG(A, B, C, D, q[ 9],  5, T[25]); // ABCD  9  5 25
		GG(D, A, B, C, q[14],  9, T[26]); // DABC 14  9 26
		GG(C, D, A, B, q[ 3], 14, T[27]); // CDAB  3 14 27
		GG(B, C, D, A, q[ 8], 20, T[28]); // BCDA  8 20 28
		GG(A, B, C, D, q[13],  5, T[29]); // ABCD 13  5 29
		GG(D, A, B, C, q[ 2],  9, T[30]); // DABC  2  9 30
		GG(C, D, A, B, q[ 7], 14, T[31]); // CDAB  7 14 31
		GG(B, C, D, A, q[12], 20, T[32]); // BCDA 12 20 32

		HH(A, B, C, D, q[ 5],  4, T[33]); // ABCD  5  4 33
		HH(D, A, B, C, q[ 8], 11, T[34]); // DABC  8 11 34
		HH(C, D, A, B, q[11], 16, T[35]); // CDAB 11 16 35
		HH(B, C, D, A, q[14], 23, T[36]); // BCDA 14 23 36
		HH(A, B, C, D, q[ 1],  4, T[37]); // ABCD  1  4 37
		HH(D, A, B, C, q[ 4], 11, T[38]); // DABC  4 11 38
		HH(C, D, A, B, q[ 7], 16, T[39]); // CDAB  7 16 39
		HH(B, C, D, A, q[10], 23, T[40]); // BCDA 10 23 40
		HH(A, B, C, D, q[13],  4, T[41]); // ABCD 13  4 41
		HH(D, A, B, C, q[ 0], 11, T[42]); // DABC  0 11 42
		HH(C, D, A, B, q[ 3], 16, T[43]); // CDAB  3 16 43
		HH(B, C, D, A, q[ 6], 23, T[44]); // BCDA  6 23 44
		HH(A, B, C, D, q[ 9],  4, T[45]); // ABCD  9  4 45
		HH(D, A, B, C, q[12], 11, T[46]); // DABC 12 11 46
		HH(C, D, A, B, q[15], 16, T[47]); // CDAB 15 16 47
		HH(B, C, D, A, q[ 2], 23, T[48]); // BCDA  2 23 48

		II(A, B, C, D, q[ 0],  6, T[49]); // ABCD  0  6 49
		II(D, A, B, C, q[ 7], 10, T[50]); // DABC  7 10 50
		II(C, D, A, B, q[14], 15, T[51]); // CDAB 14 15 51
		II(B, C, D, A, q[ 5], 21, T[52]); // BCDA  5 21 52
		II(A, B, C, D, q[12],  6, T[53]); // ABCD 12  6 53
		II(D, A, B, C, q[ 3], 10, T[54]); // DABC  3 10 54
		II(C, D, A, B, q[10], 15, T[55]); // CDAB 10 15 55
		II(B, C, D, A, q[ 1], 21, T[56]); // BCDA  1 21 56
		II(A, B, C, D, q[ 8],  6, T[57]); // ABCD  8  6 57
		II(D, A, B, C, q[15], 10, T[58]); // DABC 15 10 58
		II(C, D, A, B, q[ 6], 15, T[59]); // CDAB  6 15 59
		II(B, C, D, A, q[13], 21, T[60]); // BCDA 13 21 60
		II(A, B, C, D, q[ 4],  6, T[61]); // ABCD  4  6 61
		II(D, A, B, C, q[11], 10, T[62]); // DABC 11 10 62
		II(C, D, A, B, q[ 2], 15, T[63]); // CDAB  2 15 63
		II(B, C, D, A, q[ 9], 21, T[64]); // BCDA  9 21 64

		A += A2;
		B += B2;
		C += C2;
		D += D2;
	}

	delete []word_buffer;

	buffer[0] = A;
	buffer[1] = B;
	buffer[2] = C;
	buffer[3] = D;
}

#if defined(TEST)
int main(void)
{
    /** @todo Some tests, perhaps? */
    return 0;
}
#endif
