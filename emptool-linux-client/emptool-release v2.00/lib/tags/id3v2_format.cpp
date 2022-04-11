/* id3v2.cpp
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.7 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "id3v2_format.h"

const char *DescribeTextEncoding(BYTE textEncoding)
{
    switch (textEncoding)
    {
    case ID3V2_TEXT_ENCODING_ISO8859_1: return "ISO-8859-1";
    case ID3V2_TEXT_ENCODING_UNICODE: return "Unicode";
    }

    return "-unknown-";
}
