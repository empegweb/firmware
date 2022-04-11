/* utf_test.cpp
 *
 * Testing UTF8 support
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.8 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "utf8.h"
#include <locale.h>
#include "stringpred.h"
#include "stringops.h"

using namespace util;

void AssertEqual(const std::string &left, const std::string &right)
{
    if (left != right)
    {
	TRACE("Left string: \'%s\'\n", left.c_str());
	TRACE("Right string: \'%s\'\n", right.c_str());
	TRACE_HEX("Left string\n", left.begin(), left.end());
	TRACE_HEX("Right string\n", right.begin(), right.end());
	ASSERT(false);
    }
}

void AssertEqual(const std::wstring &left, const std::wstring &right)
{
    if (left != right)
    {
	TRACE_HEX("Left string\n", left.begin(), left.end());
	TRACE_HEX("Right string\n", right.begin(), right.end());
	ASSERT(false);
    }
}

#if WCHAR_MAX > USHRT_MAX
void AssertEqual(const utf16_string &left, const utf16_string &right)
{
    if (left != right)
    {
	TRACE_HEX("Left string\n", left.begin(), left.end());
	TRACE_HEX("Right string\n", right.begin(), right.end());
	ASSERT(false);
    }
}
#endif

void strncpy_term_filename_utf8_test(const char *input, size_t len, const char *expected_result)
{
    char *buffer = static_cast<char *>(empeg_alloca(len + 1));
    memset(buffer, 42, len+1);

    stringops::strncpy_term_filename_utf8(buffer, input, len);
    ASSERT(buffer[len] == 42);
    ASSERT(buffer[strlen(buffer)+1] == 42);
    AssertEqual(buffer, expected_result);
}

void UTF8ValidSubStringLengthTest(const char *s, size_t max_len, size_t expected_result)
{
    size_t result = UTF8ValidSubStringLength(s, max_len);
    ASSERT_EX(result == expected_result, "result = %u, expected = %u\n",
	      result, expected_result);
}

void UTF8TruncateTest(const char *s, size_t max_len, const char *expected_result)
{
    char *buffer = static_cast<char *>(empeg_alloca(max_len + 1));
    buffer[max_len] = 42;

    stringops::strncpy_term_utf8(buffer, s, max_len);
    ASSERT(buffer[max_len] == 42);
    AssertEqual(buffer, expected_result);
}

int main(void)
{
    // We must run these tests in a known locale
    setlocale(LC_ALL, "en_GB");
    
    ASSERT(UTF8FromLatin1("foo") == "foo");
    ASSERT(Latin1FromUTF8("foo") == "foo");
    ASSERT(UTF8Classify("foo") == ASCII);

    // e-acute == U+00E9
    ASSERT(UTF8FromLatin1("r\xE9glisse") == "r\xC3\xA9glisse");
    ASSERT(UTF8FromUTF16(reinterpret_cast<const UTF16CHAR *>("r\0\xE9\0g\0l\0i\0s\0s\0e\0")) == "r\xC3\xA9glisse");
    ASSERT(UTF8Classify("r\xC3\xA9glisse") == ISO_8859_1);
    ASSERT(Latin1FromUTF8("r\xC3\xA9glisse") == "r\xE9glisse");

    // U+6CB3 (see O'Reilly, "CJKV", p194)
    UTF16CHAR test16[] = { 0x6cb3, 0x0 };
    UTF16CHAR test16bom1[] = { 0xfffe, 0xb36c, 0x0 };
    UTF16CHAR test16bom2[] = { 0xfeff, 0x6cb3, 0x0 };    
    ASSERT(UTF8FromUTF16(test16) == "\xE6\xB2\xB3");
    ASSERT(UTF8FromUTF16(test16bom1) == "\xE6\xB2\xB3"); // reverse BOM!
    ASSERT(UTF8FromUTF16(test16bom2) == "\xE6\xB2\xB3"); // reverse BOM!
    ASSERT(UTF8FromUTF16(reinterpret_cast<const UTF16CHAR *>("\xFE\xFF\x6C\xB3\0")) == "\xE6\xB2\xB3");
    ASSERT(UTF8FromUTF16(reinterpret_cast<const UTF16CHAR *>("\xFF\xFE\xB3\x6C\0")) == "\xE6\xB2\xB3"); // reverse BOM!
    ASSERT(UTF8Classify("\xE6\xB2\xB3") == FULL_UNICODE);
    AssertEqual(Latin1FromUTF8("\xE6\xB2\xB3"), "\xBF"); // Unknown char
    AssertEqual(UTF16FromUTF8("\xE6\xB2\xB3"), reinterpret_cast<const UTF16CHAR *>("\xB3\x6C\0")); // with BOM!
    AssertEqual(UTF16EnsureBOM(UTF16FromUTF8("\xE6\xB2\xB3")), reinterpret_cast<const UTF16CHAR *>("\xFF\xFE\xB3\x6C\0")); // with BOM!

    ASSERT(ValidUTF8("foo"));
    ASSERT(ValidUTF8("r\xC3\xA9glisse"));
    ASSERT(ValidUTF8("\xE6\xB2\xB3"));
    ASSERT(!ValidUTF8("r\xE9glisse")); // Random top-bit-set char
    ASSERT(!ValidUTF8("r\x88glisse")); // Random continuation char
    ASSERT(!ValidUTF8("r\xC3")); // Sequence-start at end

    // These must be run in a sensible locale to work
    AssertEqual(UTF8FromSystem("A"), "A");
    AssertEqual(UTF8FromSystem("\xE9"), "\xC3\xA9");

    wchar_t wide_bit[] = { 'A', 'B', 'C', (unsigned char)'\xE9', 0 };
    AssertEqual(WideFromSystem("ABC\xE9"), wide_bit);
    AssertEqual(SystemFromWide(wide_bit), "ABC\xE9");

    ASSERT(UTF8Collate("ABC", "ABC") == 0);
    ASSERT(UTF8Collate("bcd", "ABC") == 1);
    ASSERT(UTF8Collate("ABC", "bcd") == -1);
    ASSERT(UTF8Collate("BCD", "abc") == 1);
    ASSERT(UTF8Collate("abc", "BCD") == -1);
    ASSERT(UTF8Collate("adx", "a\xC3\xA9x") == -1); // d < e-acute < f
    ASSERT(UTF8Collate("a\xC3\xA9x", "afx") == -1);
    ASSERT(UTF8Collate("aex", "a\xC3\xA9x") == -1); // e < e-acute < f
    ASSERT(UTF8Collate("aey", "a\xC3\xA9x") == 1);
    
// Wide ones we can't do now    
//    ASSERT(UTF8FromWide(L"\x6CB3") == "\xE6\xB2\xB3");
//    ASSERT(UTF8FromWide(L"\xFFFE\xB36C") == "\xE6\xB2\xB3"); // reverse BOM!
//    ASSERT(UTF8FromWide(L"\xFFFE\xB36C") == "\xE6\xB2\xB3"); // reverse BOM!
//    ASSERT(UTF8FromWide(L"r\xE9glisse") == "r\xC3\xA9glisse");

    /// @todo: Put some more complex multi-byte characters in here.
    UTF8ValidSubStringLengthTest("\xC3\xA9\xC3\xA9.mp3", 64, 8);
    UTF8ValidSubStringLengthTest("\xC3\xA9\xC3\xA9.mp3", 8, 8);
    UTF8ValidSubStringLengthTest("\xC3\xA9\xC3\xA9.mp3", 7, 7);
    UTF8ValidSubStringLengthTest("\xC3\xA9\xC3\xA9", 4, 4);
    UTF8ValidSubStringLengthTest("\xC3\xA9\xC3\xA9", 3, 2);
    UTF8ValidSubStringLengthTest("\xC3\xA9\xC3\xA9", 2, 2);
    UTF8ValidSubStringLengthTest("\xC3\xA9\xC3\xA9", 1, 0);
    UTF8ValidSubStringLengthTest("\xC3\xA9\xC3\xA9", 0, 0);
    UTF8ValidSubStringLengthTest("\xE6\xB2\xB3", 3, 3);
    UTF8ValidSubStringLengthTest("\xE6\xB2\xB3", 2, 0);
    UTF8ValidSubStringLengthTest("\xE6\xB2\xB3", 1, 0);

    // This should be moved to stringops_test if it ever gets written.

    UTF8TruncateTest("\xC3\xA9\xC3\xA9", 64, "\xC3\xA9\xC3\xA9");
    UTF8TruncateTest("\xC3\xA9!\xC3\xA9", 6, "\xC3\xA9!\xC3\xA9");
    UTF8TruncateTest("\xC3\xA9!\xC3\xA9", 5, "\xC3\xA9!");
    UTF8TruncateTest("\xC3\xA9\xC3\xA9", 5, "\xC3\xA9\xC3\xA9");
    UTF8TruncateTest("\xC3\xA9\xC3\xA9", 4, "\xC3\xA9");
    UTF8TruncateTest("\xC3\xA9\xC3\xA9", 3, "\xC3\xA9");
    UTF8TruncateTest("\xC3\xA9\xC3\xA9", 2, "");
    UTF8TruncateTest("\xC3\xA9!\xC3\xA9", 6, "\xC3\xA9!\xC3\xA9");
    UTF8TruncateTest("\xC3\xA9!\xC3\xA9", 5, "\xC3\xA9!");
    UTF8TruncateTest("\xC3\xA9!\xC3\xA9", 4, "\xC3\xA9!");
    UTF8TruncateTest("\xC3\xA9!\xC3\xA9", 3, "\xC3\xA9");

    strncpy_term_filename_utf8_test("\xC3\xA9\xC3\xA9.mp3", 64, "\xC3\xA9\xC3\xA9.mp3");
    strncpy_term_filename_utf8_test("\xC3\xA9\xC3\xA9.mp3", 9, "\xC3\xA9\xC3\xA9.mp3");
    strncpy_term_filename_utf8_test("\xC3\xA9\xC3\xA9.mp3", 8, "\xC3\xA9.mp3");
    strncpy_term_filename_utf8_test("abcd.mp3", 4, "a.m");
    strncpy_term_filename_utf8_test("abcd.m\xC3\xA9\xC3\xA9", 6, "a.m\xC3\xA9");
    strncpy_term_filename_utf8_test("abcd.m\xC3\xA9\xC3\xA9", 5, "ab.m");
    return 0;
}

