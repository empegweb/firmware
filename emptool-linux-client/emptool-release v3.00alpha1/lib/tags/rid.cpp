/* rid.cpp
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.7 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "rid.h"
#include "empeg_time.h"
#include "md5.h"
#include "empeg_error.h"
#include <stdio.h>

#define TRACE_RID 0

static const unsigned int RID_CHUNK = 65536;

static STATUS CalculateRidPart(SeekableStream *stm,
			       unsigned int startpos, unsigned int endpos,
			       unsigned char *md5sum)
{
    MessageDigest md5;
    unsigned char *buffer = NEW unsigned char[RID_CHUNK];

    STATUS st = stm->SeekAbsolute(startpos);
    if (FAILED(st))
    {
	delete[] buffer;
	return st;
    }

    ASSERT((endpos-startpos) <= RID_CHUNK);

    unsigned int bytes_read;
    st = stm->Read(buffer, endpos-startpos, &bytes_read);
    if (FAILED(st))
    {
	delete[] buffer;
	return st;
    }

    ASSERT(bytes_read == (endpos-startpos));

    md5.Digest((const char*)buffer, bytes_read);
    memcpy(md5sum, md5.ResultBuffer(), 16);

    delete[] buffer;
    return S_OK;
}

/** Calculate a unique ID for the interesting section of the file
 *
 * Currently:
 *    . MD5 the first 64K
 *    . MD5 the final 64K
 *    . MD5 the middle 64K
 *    . EOR the three results together
 *    . represent as big hex number
 *
 * ...with sensible behavior in the degenerate (<64K) case
 */
STATUS CalculateRid(SeekableStream *stm,
		    unsigned int startpos, unsigned int endpos,
		    std::string *resultptr)
{
    ASSERT(startpos <= endpos);

#if DEBUG>0
    Time start_time = Time::Now();
#endif

    unsigned char rid[16];
    STATUS st;

    if ((endpos-startpos) <= RID_CHUNK)
    {
	/* Not big enough to do the whole thing: just md5 it all */
	st = CalculateRidPart(stm, startpos, endpos, rid);
    }
    else
    {
	/* 3-way MD5 (to catch songs that start the same as other songs but
	 * are different, without the overhead of md5ing the whole thing)
	 */
	unsigned char ridpart[16];
	
	st = CalculateRidPart(stm, startpos, startpos+RID_CHUNK, rid);

	if (SUCCEEDED(st))
	{
	    st = CalculateRidPart(stm, endpos-RID_CHUNK, endpos, ridpart);
	    for (unsigned int i=0; i<16; i++)
		rid[i] ^= ridpart[i];
	}

	if (SUCCEEDED(st))
	{
	    unsigned int halfway = (startpos + (endpos-RID_CHUNK)) /2;
	    st = CalculateRidPart(stm, halfway, halfway+RID_CHUNK, ridpart);
	    for (unsigned int i=0; i<16; i++)
		rid[i] ^= ridpart[i];
	}
    }

    if (SUCCEEDED(st))
    {
	std::string result;
	result.reserve(32);
	for (unsigned int i=0; i<16; i++)
	{
	    char byte[3];
	    sprintf(byte,"%02x", rid[i]);
	    result += byte;
	}

	*resultptr = result;

#if DEBUG>0
	Time end_time = Time::Now();

	TRACEC(TRACE_RID, "Rid is %s (%dus)\n", result.c_str(),
	       (end_time - start_time).GetAsMicroseconds());
#endif
    }

    return st;
}
