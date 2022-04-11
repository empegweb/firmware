/* readtags.cpp
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "tag_extractor.h"

void dump(const char *filename)
{
    TagExtractToMap tem;
    TagExtractor *te;
    STATUS st = TagExtractor::Create(filename, "", &te);
    if (FAILED(st))
    {
	fprintf(stderr, "%s: create error 0x%08x\n", filename, PrintableStatus(st));
	exit(1);
    }
    st = te->ExtractTags(filename, &tem);
    if (FAILED(st))
    {
	fprintf(stderr, "%s: extract error 0x%08x\n", filename, PrintableStatus(st));
	exit(1);
    }
    
    for (TagExtractToMap::const_iterator i = tem.begin(); i != tem.end(); ++i)
    {
	printf("%s: %s='%s'\n", filename, i->first.c_str(), i->second.c_str());
    }
}

int main(int argc, char *argv[])
{
    argv++;

    while (*argv)
	dump(*argv++);

    return 0;
}
