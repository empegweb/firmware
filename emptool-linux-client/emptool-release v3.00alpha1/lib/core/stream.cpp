/* stream.cpp 
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 13-Mar-2003 18:15 rob:)
 *
 */

#include "config.h"
#include "trace.h"
#include "stream.h"
#include "empeg_error.h"

/** Blocks until all received or error */
STATUS Stream::EnsureRead(void *vbuffer, unsigned int total)
{
    unsigned int nbytes;
    unsigned char *buffer = (unsigned char*)vbuffer;

    STATUS st = Read(buffer, total, &nbytes);
    if (st != S_OK)
    {
	// A timeout *before* a packet is OK; a timeout mid-packet is
	// catastrophic
	if (st == E_TIMEOUT)
	    st = S_FALSE;
	return st;
    }
    if (nbytes == 0)
	return S_FALSE;

    while (nbytes < total)
    {
	unsigned int nread;
	st = Read(buffer + nbytes, total - nbytes, &nread);
	if (st == S_FALSE)
	    return E_ENDOFFILE;

	if (FAILED(st))
	    return st;

	if (nread == 0)
	    return E_ENDOFFILE;

	ASSERT(nread != 0);
	nbytes += nread;
    }

    return S_OK;
}

/** Blocks until all gone or error */
STATUS Stream::EnsureWrite(const void *vbuffer, unsigned int total)
{
    unsigned int nbytes;
    const unsigned char *buffer = (const unsigned char*) vbuffer;

    STATUS st = Write(buffer, total, &nbytes);
    if (FAILED(st))
	return st;

    while (nbytes < total)
    {
	unsigned int nwritten;
	st = Write(buffer + nbytes, total - nbytes, &nwritten);
	if (FAILED(st))
	    return st;

	ASSERT(nwritten != 0);
	nbytes += nwritten;
    }

    return S_OK;
}

#if defined(TEST)
int main(void)
{
    /** @todo Some tests, perhaps? */
    return 0;
}
#endif
