/* line_chopper_test.cpp
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 13-Mar-2003 18:15 rob:)
 */

#include "line_chopper.h"
#include <string>
#include <vector>
#include "test_assert.h"
//#include <wstring>

#if defined(TEST)
void testLineChopper()
{
    // Check it with an empty string.
    {
	LineChopper<std::string> chop;
	assertTrue(chop.begin() == chop.end());
    }

    // Check it with a bunch of lines.
    {
	LineChopper<std::string> chop("one\ntwo\nthree\n");

	LineChopper<std::string>::const_iterator i = chop.begin();
	assertTrue(i != chop.end());
	assertStringsEqual("one", *i);
	++i;
	assertTrue(i != chop.end());
	assertStringsEqual("two", *i);
	++i;
	assertTrue(i != chop.end());
	assertStringsEqual("three", *i);
	++i;
	assertTrue(i == chop.end());
    }

    // Check it with std::copy
    {
	LineChopper<std::string> chop("one\ntwo\nthree\n");

	std::vector<std::string> lines;
	std::copy(chop.begin(), chop.end(), std::back_inserter(lines));

	assertStringsEqual("one", lines[0]);
	assertStringsEqual("two", lines[1]);
	assertStringsEqual("three", lines[2]);
	assertTrue(lines.size() == 3);
    }

    // Check it with blank lines -- they should be preserved.
    {
	LineChopper<std::string> chop("one\n\nthree\n");

	std::vector<std::string> lines;
	std::copy(chop.begin(), chop.end(), std::back_inserter(lines));

	assertStringsEqual("one", lines[0]);
	assertStringsEqual("x", lines[1]);
	assertStringsEqual("three", lines[2]);
	assertTrue(lines.size() == 3);
    }

    // Check it without a blank line at the end.
    {
	LineChopper<std::string> chop("zero\none\ntwo\nthree");

	std::vector<std::string> lines;
	std::copy(chop.begin(), chop.end(), std::back_inserter(lines));

	assertStringsEqual("zero", lines[0]);
	assertStringsEqual("one", lines[1]);
	assertStringsEqual("two", lines[2]);
	assertStringsEqual("three", lines[3]);
	assertTrue(lines.size() == 4);
    }

    // Check it with a UTF8 string.
    {
	LineChopper<std::string> chop("zero\none\ntwo\nthree");

	std::vector<std::string> lines;
	std::copy(chop.begin(), chop.end(), std::back_inserter(lines));

	assertStringsEqual("zero", lines[0]);
	assertStringsEqual("one", lines[1]);
	assertStringsEqual("two", lines[2]);
	assertStringsEqual("three", lines[3]);
	assertTrue(lines.size() == 4);
    }

    // Check it with a wchar_t string.
    {
	LineChopper<std::wstring> chop(L"zero\none\ntwo\nthree");

	std::vector<std::wstring> lines;
	std::copy(chop.begin(), chop.end(), std::back_inserter(lines));

	assertStringsEqual(L"zero", lines[0]);
	assertStringsEqual(L"one", lines[1]);
	assertStringsEqual(L"two", lines[2]);
	assertStringsEqual(L"three", lines[3]);
	assertTrue(lines.size() == 4);
    }
}

int main(void)
{
    testLineChopper();
}
#endif
