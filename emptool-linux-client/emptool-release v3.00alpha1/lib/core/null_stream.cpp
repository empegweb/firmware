/* null_stream.cpp
 *
 * Like /dev/null (writes go nowhere) except that reads error
 *
 * (C) 2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "empeg_error.h"
#include "stream.h"

STATUS NullStream::Read(void * /*buffer*/, unsigned /*bytesToRead*/, unsigned * /*pBytesRead*/)
{
    return E_NOTIMPL;
}

STATUS NullStream::Write(const void * /*buffer*/, unsigned bytesToWrite, unsigned * pBytesWritten)
{
    *pBytesWritten = bytesToWrite;

    return S_OK;
}

#if defined(TEST)
int main(void)
{
    /** @todo Some tests, perhaps? */
    return 0;
}
#endif
