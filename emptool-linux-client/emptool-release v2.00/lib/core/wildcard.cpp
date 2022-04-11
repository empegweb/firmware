/* wildcard.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.7 01-Apr-2003 18:52 rob:)
 */

#include <string.h>
#include <ctype.h>

#include "wildcard.h"

#if 0
// This code unfortunately fails many of the test cases (plus it uses loads
// of stack for fixed-size buffers, ewwww)
bool strwild(const char *text, const char *pattern, bool caseless)
{
    char *star = NULL;

    /* Check for problem/fast cases */
    if((!pattern) || (!text)) return false;
  
    if((strlen(pattern) == 0) && (strlen(text) != 0)) return false;
    if(!strcmp(pattern, "*")) return true;

    if((star = strchr(pattern, '*')) != NULL) {
	char p1[1024];
	char p2[1024];

	/* 0 or more chars to match ... */
	strncpy(p1, pattern, star - pattern);
	p1[star-pattern] = '\0';
	strncpy(p2, text, star - pattern);
	p2[star-pattern] = '\0';
	if(!strwild(p1, p2, caseless)) return false;
	if(!strcmp(star, "*")) return true;
	text = text + (star - pattern);
	pattern = star + 1;
	while(*text != '\0') {
	    if(strwild(pattern, text, caseless)) return true;
	    text++;
	}
	return false;
    }
    else {
	/* easy case */
	if(strlen(pattern) != strlen(text)) return false;
	while(*pattern != '\0') {
	    if(caseless) {
		if((tolower(*pattern) != tolower(*text)) && (*pattern != '?')) return false;
	    }
	    else {
		if((*pattern != *text) && (*pattern != '?')) return false;
	    }
	    pattern++;
	    text++;
	}
	return true;
    }
}
#endif

bool strwild( const char *text, const char *pattern, bool caseless )
{
    const char *end; // used in character-class matching

    for (;;)
    {
	if ( *pattern == '\0' )
	    return (*text == '\0');
	else if ( *pattern == '?' )
	{
	    if ( *text )
	    {
		pattern++;
		text++;
	    }
	    else
		return false;
	}
	else if ( *pattern == '[' && (end=strchr(pattern+1,']')) != NULL )
	{
	    /* Character class */
	    const char *search = pattern+1;
	    for (; search < end; search++)
	    {
		if ( caseless ? ( tolower(*search) == tolower(*text) )
		              : *search == *text )
		    break;
	    }

	    if ( search == end )
	    {
		/* Text character wasn't in character class */
		return false;
	    }
	    text++;
	    pattern = end+1;
	}
	else if ( *pattern == '*' )
	{
	    while ( *pattern == '*' )
		pattern++;

	    while ( *text )
	    {
		if ( strwild( text, pattern, caseless ) )
		    return true;
		text++;
	    }
	    return *pattern == '\0';
	}
	else if ( *pattern == '\\' )
	{
	    pattern++;
	    if ( !*pattern )
		return false;

	    if ( *pattern != *text )
		return false;

	    pattern++;
	    text++;
	}
	else
	{
	    /* non-magic, match literal */
	    if ( caseless )
	    {
		if ( tolower(*pattern) != tolower(*text) )
		    return false;
	    }
	    else
	    {
		if ( *pattern != *text )
		    return false;
	    }
	    pattern++;
	    text++;
	}
    }
}

#ifdef TEST

#include <stdio.h>

const struct {
    char *txt; char *pat; bool whether;
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

int main()
{
    bool ok = true;
    for ( unsigned int i=0; i < sizeof(tests)/sizeof(tests[0]); ++i )
    {
	bool result = strwild( tests[i].txt, tests[i].pat, true );

	printf( "%s %s %s %s\n",
		tests[i].txt, result ? "~" : "!~",
		tests[i].pat, (result == tests[i].whether) ? "OK" : "*** NO" );
	if ( result != tests[i].whether )
	    ok = false;
    }
    printf( ok ? "PASS\n" : "FAIL\n" );
    return ok ? 0 : 1;
}

#endif
