/* dynamic_config_file_test.cpp
 *
 * Configuration file support
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 *
 * Facilities for writing to/reading from config files.  We need to support writing:
 * - UTF8 (for the player's config.ini)
 * - Code Page Native (for .pls file support)
 * - TCHAR (for the Windows Registry) - we write UTF16 ifdef(UNICODE), Code Page Native otherwise.
 *
 * The interface presented to the client code is both:
 * - UTF8 (for the player)
 * - TCHAR (for Windows)
 *
 * Section and key names are case-insensitive.
 * 
 * Complicated stuff:
 * We need to change the file as little as possible.
 * - sections should be in the same order.
 * - key=value within sections should be in the same order.
 * - comments should stay as close as possible to where they were originally.
 */

#include "config.h"
#include "trace.h"
#include "dynamic_config_file.h"

#if defined(TEST)
#include <vector>
#include "test_assert.h"
#include "empeg_tchar.h"
#include "utf8.h"
#include "stringpred.h"
#include "line_chopper.h"
#include "var_string.h"

void testConfigFileUTF8WithIntegers()
{
    // Getting a non-existant integer
    {
	ConfigFileUTF8 config;

	int value = 0x1234;
	assertFalse(config.GetInteger(_T("section"), _T("key"), &value));
	assertIntegersEqual(0x1234, value);
    }

    // Setting an integer, and then getting it again
    {
	ConfigFileUTF8 config;

	config.SetInteger(_T("section"), _T("key"), 0x5678);
	int value = 0x1234;
	assertTrue(config.GetInteger(_T("section"), _T("key"), &value));
	assertIntegersEqual(0x5678, value);
    }

    // Set an integer twice, making sure that the second value is returned.
    {
	ConfigFileUTF8 config;

	config.SetInteger(_T("section"), _T("key"), 0x5678);
	config.SetInteger(_T("section"), _T("key"), 0x9ABC);
	int value = 0x1234;
	assertTrue(config.GetInteger(_T("section"), _T("key"), &value));
	assertIntegersEqual(0x9ABC, value);
    }

    // Try a couple of different sections and keys
    {
	ConfigFileUTF8 config;

	config.SetInteger(_T("section1"), _T("key1"), 0x5678);
	config.SetInteger(_T("section1"), _T("key2"), 0x9ABC);
	config.SetInteger(_T("section2"), _T("key1"), 0xEFEF);
	config.SetInteger(_T("section2"), _T("key2"), 0xAACC);

	// Check that non-existent values still work the same.
	int value = 0x1234;
	assertFalse(config.GetInteger(_T("section"), _T("key"), &value));
	assertIntegersEqual(0x1234, value);

	assertTrue(config.GetInteger(_T("section1"), _T("key1"), &value));
	assertIntegersEqual(0x5678, value);
	assertTrue(config.GetInteger(_T("section1"), _T("key2"), &value));
	assertIntegersEqual(0x9ABC, value);
	assertTrue(config.GetInteger(_T("section2"), _T("key1"), &value));
	assertIntegersEqual(0xEFEF, value);
	assertTrue(config.GetInteger(_T("section2"), _T("key2"), &value));
	assertIntegersEqual(0xAACC, value);
    }
}

void testConfigFileUTF8WithStrings()
{
    // Do some work with strings.
    {
	ConfigFileUTF8 config;

	utf8_string s("Foo");
	assertFalse(config.GetString8(_T("section"), _T("key"), &s));
	assertStringsEqual("Foo", s);
    }

    {
	ConfigFileUTF8 config;

	config.SetString8(_T("section"), _T("key"), "Foo");
	config.SetString8(_T("section"), _T("key"), "Bar");

	utf8_string s;
	assertTrue(config.GetString8(_T("section"), _T("key"), &s));
	assertStringsEqual("Bar", s);
    }
}

void testConfigFileUTF8Loading()
{
    // Try loading it from an empty UTF8 string
    {
	ConfigFileUTF8 config;

	utf8_string test;
	config.FromString(test);

	// It should have no values in it.
	int value;
	assertFalse(config.GetInteger(_T("section"), _T("key"), &value));
    }

    // Load it from a UTF8 string with some values in it.  See if they're OK.
    {
	ConfigFileUTF8 config;

	utf8_string test("[section]\n"
			 "key=12\n"
			 "\n");
	config.FromString(test);

	int value;
	assertTrue(config.GetInteger(_T("section"), _T("key"), &value));
	assertIntegersEqual(12, value);
    }

#if 0 // doesn't work yet.
    // Load it from a string.  Make no changes.  Save it again.  Check that it's identical.
    {
	ConfigFileUTF8 config;
	utf8_string test("# This is comment 1\n"
			 "[section]\n"
			 "key=12\n"
			 "something=this\n"
			 "\n"
			 "; This is comment 2\n"
			 "[another section]\n"
			 "something=that\n"
			 "key=12\n");
	config.FromString(test);

	utf8_string s;
	assertTrue(config.GetString8(_T("another section"), _T("something"), &s));
	assertStringsEqual("that", s);
	utf8_string result;
	config.ToString(&result);

	assertStringsEqual(test, result);
    }
#endif

    // Load it from a string.  Make some changes.  Save it again.  Check that it's vaguely sane.
    {
	// i.e. that the sections are in the same order.  The new stuff is at the end of the sections.
	// The new sections are at the end of the file.
	// The comments are unchanged.  Whitespace is still where it was left.
    }

    // Try loading it from a CP native string
    {
	// This doesn't have to be the same class.
    }

    // Try loading it from a TCHAR string
    {
	// This doesn't have to be the same class.  In fact, this one's supposed to read/write the registry.
	// How to do that?
    }
}

int main(void)
{
    testConfigFileUTF8WithIntegers();
    testConfigFileUTF8WithStrings();
    testConfigFileUTF8Loading();

    return test::assertSummarise();
}
#endif
