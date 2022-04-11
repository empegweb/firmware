/* wildcard_test.cpp
 *
 * CppUnit test for wildcard.cpp
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.4 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "wildcard.h"
#include <stdio.h>

static const struct {
    const char *txt; const char *pat; bool whether;
} tests[] = {
   { "foo", "foo", true },
   { "foo", "FOO", true },
   { "foo", "f*o", true },
   { "foo", "f*O", true },
   { "foo", "f*Oo", true },
   { "foo", "f*O**o", true },
   { "foo", "f*?", true },
   { "foo", "f*??", true },
   { "foo", "f*???", false },
   { "foo", "f*?*", true },
   { "foo", "f*??*", true },
   { "foo", "f*???*", false },
   { "foo", "f*?*?*", true },
   { "foo", "f*?*??*", false },
   { "foo", "f*??*?*", false },
   { "foo", "f*?*?*?", false },
   { "foo", "f*?*?*?*", false },
   { "foo", "*?*?*?*", true },
   { "foo", "f*O?", true },
   { "fo0", "f*o", false },
   { "fo0", "fo0*", true },
   { "fo0", "fo0*a", false },
   { "fo0*", "fo0*", true },
   { "fo0*", "fo0\\*", true },
   { "fo0",  "fo0\\*", false },
   { "fo0",  "fo[012]", true },
   { "fo[",  "fo[", true },
   { "fo[]",  "fo[]", false },
   { "fo[]",  "fo\\[]", true },
   { "fo0",  "fo[123]", false },
   { "foA",  "fo[abc]", true },
   { "foC",  "fo[abc]", true },
   { "fo[",  "fo[abc]", false }
};

#if defined(TEST)
int main(void)
{
    for ( unsigned int i=0; i < sizeof(tests)/sizeof(tests[0]); ++i )
    {
	bool result = strwild( tests[i].txt, tests[i].pat, true );

	if ( result != tests[i].whether )
	{
	    printf( "%s %s %s %s\n",
		    tests[i].txt, result ? "~" : "!~",
		    tests[i].pat,
		    (result == tests[i].whether) ? "OK" : "*** NO" );
	}
	ASSERT( result == tests[i].whether );
    }

    return 0;
}
#endif
