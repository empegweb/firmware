/* pstring.cpp
 *
 * Pascal-style strings
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.6 01-Apr-2003 18:52 rob:)
 */

#include "pstring.h"

inline static unsigned char fast_tolower(unsigned char c)
{
    static const unsigned char lowercase_conv[256] = {
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
	0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
	0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
	0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
	0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,
	0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,
	0x78,0x79,0x7a,0x5b,0x5c,0x5d,0x5e,0x5f,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,
	0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,
	0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
	0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,
	0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,
	0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,
	0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,
	0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,
	0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
	0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,
	0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
	0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,
	0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,
    };

    return lowercase_conv[c];
}

int pstring::cmp(const pstring &other_str) const
{
    const unsigned char *d = m_data;
    const unsigned char *o = other_str.value();
    unsigned char len = *d++;
    unsigned char olen = *o++;
    int lendiff = (int) len - (int) olen;
    int left = lendiff > 0 ? olen : len;
    int diff;
    while(left) {
	diff = ((int) *d) - ((int) *o);
	if(diff) return diff;
	d++;
	o++;
	left --;
    }
    return lendiff;
}

int pstring::cmp(const char *other) const
{
    const unsigned char *d = m_data;
    const unsigned char *o = (const unsigned char *) other;
    unsigned char len = *d;
    if(!len) {
	if(!*other) return 0;
	else return -1;
    }
    d++;
    int diff;
    while(len) {
	diff = ((int) *d) - ((int) *o);
	if(diff) return diff;
	if(!*o) return 1;
	o++;
	d++;
	len --;
    }
    return 0;
}

int pstring::ncmp(const pstring &other_str, size_t nlen) const
{
    const unsigned char *d = m_data;
    const unsigned char *o = other_str.value();
    unsigned char len = *d++;
    unsigned char olen = *o++;
    int lendiff = (int) len - (int) olen;
    int from = lendiff > 0 ? olen : len;
    if(from > (int) nlen) from = nlen;
    int left = from;
    int diff;
    while(left) {
	diff = ((int) *d) - ((int) *o);
	if(diff) return diff;
	d++;
	o++;
	left --;
    }
    if(from >= (int) nlen) return lendiff;
    else return 0;
}

int pstring::ncmp(const char *other, size_t nlen) const
{
    const unsigned char *d = m_data;
    const unsigned char *o = (const unsigned char *) other;
    unsigned char len = *d;
    if(nlen < len) len = nlen;
    if(!len) {
	if(!*other) return 0;
	else return -1;
    }
    d++;
    int diff;
    while(len) {
	diff = ((int) *d) - ((int) *o);
	if(diff) return diff;
	if(!*o) return 1;
	o++;
	d++;
	len --;
    }
    return 0;
}

int pstring::casecmp(const pstring &other_str) const
{
    const unsigned char *d = m_data;
    const unsigned char *o = other_str.value();
    unsigned char len = *d++;
    unsigned char olen = *o++;
    int lendiff = (int) len - (int) olen;
    int left = lendiff > 0 ? olen : len;
    int diff;
    unsigned char dc, oc;
    while(left) {
	dc = fast_tolower(*d);
	oc = fast_tolower(*o);
	diff = ((int) dc) - ((int) oc);
	if(diff) return diff;
	d++;
	o++;
	left --;
    }
    return lendiff;
}

int pstring::casecmp(const char *other) const
{
    const unsigned char *d = m_data;
    const unsigned char *o = (const unsigned char *) other;
    unsigned char len = *d;
    if(!len) {
	if(!*other) return 0;
	else return -1;
    }
    d++;
    unsigned char dc, oc;
    int diff;
    while(len) {
	dc = fast_tolower(*d);
	oc = fast_tolower(*o);
	diff = ((int) dc) - ((int) oc);
	if(diff) return diff;
	if(!*o) return 1;
	o++;
	d++;
	len --;
    }
    return 0;
}

int pstring::ncasecmp(const pstring &other_str, size_t nlen) const
{
    const unsigned char *d = m_data;
    const unsigned char *o = other_str.value();
    unsigned char len = *d++;
    unsigned char olen = *o++;
    int lendiff = (int) len - (int) olen;
    int from = lendiff > 0 ? olen : len;
    if(from > (int) nlen) from = nlen;
    int left = from;
    int diff;
    unsigned char dc, oc;
    while(left) {
	dc = fast_tolower(*d);
	oc = fast_tolower(*o);
	diff = ((int) dc) - ((int) oc);
	if(diff) return diff;
	d++;
	o++;
	left --;
    }
    if(from >= (int) nlen) return lendiff;
    else return 0;
}

int pstring::ncasecmp(const char *other, size_t nlen) const
{
    const unsigned char *d = m_data;
    const unsigned char *o = (const unsigned char *) other;
    unsigned char len = *d;
    if(nlen < len) len = nlen;
    if(!len) {
	if(!*other) return 0;
	else return -1;
    }
    d++;
    unsigned char dc, oc;
    int diff;
    while(len) {
	dc = fast_tolower(*d);
	oc = fast_tolower(*o);
	diff = ((int) dc) - ((int) oc);
	if(diff) return diff;
	if(!*o) return 1;
	o++;
	d++;
	len --;
    }
    return 0;
}
