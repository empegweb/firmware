/* static_check.h
 *
 *
 * (C) 2000-2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 13-Mar-2003 18:15 rob:)
 */

#ifndef STATIC_CHECK_H
#define STATIC_CHECK_H 1

// Borrowed from Modern C++ Design pp23-26

template<int> struct CompileTimeChecker
{
    CompileTimeChecker(...);
};

template<> struct CompileTimeChecker<true> {};
#define STATIC_CHECK(expr, msg) do { class ERROR_##msg {}; \ (void)sizeof(CompileTimeChecker<(expr)>(ERROR_##msg())); } while(0)

#endif // STATIC_CHECK_H
