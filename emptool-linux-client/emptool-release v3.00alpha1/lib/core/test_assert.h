/* test_assert.h
 *
 * Simple test framework for C++.  Output assert failures to the screen, and to the Visual C++ debugger,
 * in a format suitable for pressing F4.
 *
 * This code is in the public domain.  Do whatever you want with it.
 *    -- Roger Lipscombe, 2002/08/06
 *
 * (:Empeg Source Release 1.4 13-Mar-2003 18:15 rob:)
 */

#include <string>

#define assertTrue(expr) \
    (void)((expr) || \
	test::assertFailed("assertTrue failed: " #expr, __FILE__, __LINE__), 0)

#define assertFalse(expr) \
    (void)(!(expr) || \
	test::assertFailed("assertFalse failed: " #expr, __FILE__, __LINE__), 0)

#define assertStringsEqual(expected, actual) \
    (void)((test::stringsCompareEqual((expected), (actual))) || \
	test::assertStringsEqualFailed(expected, actual, __FILE__, __LINE__), 0)

#define assertIntegersEqual(expected, actual) \
    (void)(((expected) == (actual)) || \
	test::assertIntegersEqualFailed(expected, actual, __FILE__, __LINE__), 0)

#define assertBooleansEqual(expected, actual) \
    (void)(((expected) == (actual)) || \
	test::assertBooleansEqualFailed(expected, actual, __FILE__, __LINE__), 0)

#define assertStringsNotEqual(expected, actual) \
    (void)((!test::stringsCompareEqual((expected), (actual))) || \
	test::assertStringsEqualFailed(expected, actual, __FILE__, __LINE__), 0)

namespace test {

int assertFailed(const char *msg, const char *file, int line);
int assertStringsEqualFailed(const char *expected, const char *actual, const char *file, int line);
int assertStringsEqualFailed(const wchar_t *expected, const wchar_t *actual, const char *file, int line);
int assertIntegersEqualFailed(int expected, int actual, const char *file, int line);
int assertBooleansEqualFailed(int expected, int actual, const char *file, int line);
int assertSummarise();

// Pointer comparison is *not* the correct way to compare strings.  Use a helper function instead.
bool stringsCompareEqual(const char *lhs, const char *rhs);
bool stringsCompareEqual(const wchar_t *lhs, const wchar_t *rhs);

// Inline helpers for std::string comparison.
inline int assertStringsEqualFailed(const std::string &expected, const std::string &actual, const char *file, int line)
    { return assertStringsEqualFailed(expected.c_str(), actual.c_str(), file, line); }

#ifndef ECOS
inline int assertStringsEqualFailed(const std::wstring &expected, const std::wstring &actual, const char *file, int line)
    { return assertStringsEqualFailed(expected.c_str(), actual.c_str(), file, line); }
#endif

inline bool stringsCompareEqual(const std::string &lhs, const std::string &rhs)
    { return stringsCompareEqual(lhs.c_str(), rhs.c_str()); }

#ifndef ECOS
inline bool stringsCompareEqual(const std::wstring &lhs, const std::wstring &rhs)
    { return stringsCompareEqual(lhs.c_str(), rhs.c_str()); }
#endif

}; // namespace test
