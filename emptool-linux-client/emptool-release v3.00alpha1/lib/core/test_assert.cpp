/* test_assert.cpp
 *
 * Simple test framework for C++.  Output assert failures to the screen, and to the Visual C++ debugger,
 * in a format suitable for pressing F4.
 *
 * This code is in the public domain.  Do whatever you want with it.
 *    -- Roger Lipscombe, 2002/08/06
 *
 * (:Empeg Source Release 1.9 13-Mar-2003 18:15 rob:)
 */

#include "test_assert.h"
#include <stdio.h>
#include "empeg_tchar.h"

#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define empeg_snprintf _snprintf
#else
#define OutputDebugString(X) do { } while(0)
#define OutputDebugStringA(X) do { } while(0)
#define empeg_snprintf snprintf
#endif

#define MAX_BUFFER 1024

namespace test {

static int assertFailures = 0;

static void outputString(const char *buffer)
{
    fprintf(stderr, "%s\n", buffer);
    OutputDebugStringA(buffer);
    OutputDebugStringA("\r\n");
}

#if defined(WIN32)
#define ASSERT_PREFIX_FORMAT "%s(%d) : "
#else
#define ASSERT_PREFIX_FORMAT "%s:%d: "
#endif

int assertFailed(const char *msg, const char *file, int line)
{
    char buffer[MAX_BUFFER];
    empeg_snprintf(buffer, MAX_BUFFER, ASSERT_PREFIX_FORMAT "%s", file, line, msg);

    outputString(buffer);
    ++assertFailures;
    return 0;
}

int assertStringsEqualFailed(const char *expected, const char *actual,
			     const char *file, int line)
{
    char buffer[MAX_BUFFER];
    empeg_snprintf(buffer, MAX_BUFFER,
		   ASSERT_PREFIX_FORMAT "assertStringsEqual failed:"
		   " expected: \"%hs\", actual: \"%hs\"",
		   file, line,
		   expected, actual);

    outputString(buffer);
    ++assertFailures;
    return 0;
}

int assertStringsEqualFailed(const wchar_t *expected, const wchar_t *actual,
			     const char *file, int line)
{
    char buffer[MAX_BUFFER];
    empeg_snprintf(buffer, MAX_BUFFER,
		   ASSERT_PREFIX_FORMAT "assertStringsEqual failed:"
		   " expected: \"%ls\", actual: \"%ls\"",
		   file, line,
		   expected, actual);

    outputString(buffer);
    ++assertFailures;
    return 0;
}

int assertIntegersEqualFailed(int expected, int actual, const char *file, int line)
{
    char buffer[MAX_BUFFER];
    empeg_snprintf(buffer, MAX_BUFFER, ASSERT_PREFIX_FORMAT "expected: %d, actual: %d",
		   file, line,
		   expected, actual);

    outputString(buffer);
    ++assertFailures;
    return 0;
}

int assertBooleansEqualFailed(int expected, int actual, const char *file, int line)
{
    char buffer[MAX_BUFFER];
    empeg_snprintf(buffer, MAX_BUFFER, ASSERT_PREFIX_FORMAT "expected: %s, actual: %s",
		   file, line,
		   expected ? "true" : "false",
		   actual ? "true" : "false");

    outputString(buffer);
    ++assertFailures;
    return 0;
}

int assertSummarise()
{
    if (assertFailures == 0)
    {
	outputString("All tests passed.");
	return 0;
    }
    else
    {
	char buffer[MAX_BUFFER];
	empeg_snprintf(buffer, MAX_BUFFER, "%d assert(s) triggered.  Tests failed.",
		       assertFailures);
	outputString(buffer);
	return 2;
    }
}

bool stringsCompareEqual(const char *lhs, const char *rhs)
{
    return strcmp(lhs, rhs) == 0;
}

#ifndef ECOS
bool stringsCompareEqual(const wchar_t *lhs, const wchar_t *rhs)
{
    return wcscmp(lhs, rhs) == 0;
}
#endif

}; // test

#if defined(TEST)
int main(void)
{
    // Some brief exercises for the assertion framework:
    {
	assertTrue(true);
	assertFalse(false);
	assertTrue(true || false);
	assertFalse(true && false);
    
	assertStringsEqual("this", "this");
	assertStringsNotEqual("this", "that");

	const char *p = "this";
	const char *q = "thisthis";
	q += 4;
	assertStringsEqual(p, q);

	std::string pp(p);
	std::string qq(q);
	assertStringsEqual(pp, qq);
    }

    // Some brief failure exercises for the assertion framework.
    // These tests are supposed to fail.  Set TEST_ASSERT_FAILURES to 1
    // if you want to check that you've not broken the assertions.
    {
#define TEST_ASSERT_FAILURES 0
#if TEST_ASSERT_FAILURES
	assertFalse(true);
	assertTrue(false);
	assertFalse(true || false);
	assertTrue(true && false);
    
	assertStringsNotEqual("this", "this");
	assertStringsEqual("this", "that");

	const char *p = "this";
	const char *q = "thisthis";
	q += 4;
	assertStringsNotEqual(p, q);

	std::string pp(p);
	std::string qq(q);
	assertStringsNotEqual(pp, qq);
#endif // TEST_ASSERT_FAILURES
    }
    
    return 0;
}
#endif
