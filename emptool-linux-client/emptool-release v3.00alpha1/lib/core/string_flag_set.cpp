/* string_flag_set.cpp
 *
 * Access to a semicolon separated set of string flags.
 *
 * (C) 2003 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *
 */

#include "config.h"
#include "trace.h"
#include "string_flag_set.h"

StringFlagSet::StringFlagSet(const utf8_string &flag_set)
    : m_flag_set(flag_set)
{
    ASSERT(util::UTF8Classify(flag_set) == util::ASCII);

    // For the searching algorithm we need semicolons to surround all flag names.
    if (m_flag_set.length() > 0)
    {
	if (*m_flag_set.begin() != ';')
	    m_flag_set.insert(m_flag_set.begin(), ';');
	if (*m_flag_set.rbegin() != ';')
	    m_flag_set += ';';

	ASSERT(*m_flag_set.begin() == ';');
	ASSERT(*m_flag_set.rbegin() == ';');
    }
    else
	m_flag_set = ";";

    ASSERT(m_flag_set.length() != 0);
    ASSERT(m_flag_set.find(";;") == utf8_string::npos);
}

bool StringFlagSet::IsPresent(const utf8_string &flag_name) const
{
    ASSERT(m_flag_set.length() != 0);
    ASSERT(util::UTF8Classify(flag_name) == util::ASCII);
    ASSERT(flag_name.find(';') == utf8_string::npos);
    ASSERT(flag_name.length() > 0);

    utf8_string bracketed_name(";");
    bracketed_name += flag_name;
    bracketed_name += ';';

    utf8_string::size_type n = m_flag_set.find(bracketed_name);
    if (n == utf8_string::npos)
	return false;
    else
	return true;
}

void StringFlagSet::Set(const utf8_string &flag_name)
{
    ASSERT(m_flag_set.length() != 0);
    ASSERT(util::UTF8Classify(flag_name) == util::ASCII);
    ASSERT(flag_name.find(';') == utf8_string::npos);
    ASSERT(flag_name.length() > 0);


    if (!IsPresent(flag_name))
    {
	m_flag_set.append(flag_name);
	m_flag_set += ';';
	ASSERT(m_flag_set.find(";;") == utf8_string::npos);
    }
}

void StringFlagSet::Clear(const utf8_string &flag_name)
{
    ASSERT(m_flag_set.length() != 0);
    ASSERT(util::UTF8Classify(flag_name) == util::ASCII);
    ASSERT(flag_name.find(';') == utf8_string::npos);
    ASSERT(flag_name.length() > 0);

    utf8_string bracketed_name(";");
    bracketed_name += flag_name;
    bracketed_name += ';';

    utf8_string::size_type n = m_flag_set.find(bracketed_name);
    if (n != utf8_string::npos)
    {
	m_flag_set.erase(n, bracketed_name.length() - 1);
	ASSERT(m_flag_set.find(";;") == utf8_string::npos);
    }
}

utf8_string StringFlagSet::GetFlagSet() const
{
    ASSERT(m_flag_set.length() != 0);
    utf8_string::size_type first = 0;
    utf8_string::size_type last = utf8_string::npos;

    if (m_flag_set.length() > 1)
    {
	ASSERT(*m_flag_set.begin() == ';');
	first = 1;
	ASSERT(*m_flag_set.rbegin() == ';');
	last = m_flag_set.length() - 2;
    }
    else
    {
	last = 0;
    }
    return m_flag_set.substr(first, last);
}

#if defined(TEST)
int main()
{
    StringFlagSet flags("wibble;ptoing;shuffle");

    ASSERT(flags.IsPresent("wibble"));
    ASSERT(flags.IsPresent("ptoing"));
    ASSERT(flags.IsPresent("shuffle"));
    ASSERT(!flags.IsPresent("shuff"));

    flags.Set("new");
    ASSERT(flags.IsPresent("wibble"));
    ASSERT(flags.IsPresent("ptoing"));
    ASSERT(flags.IsPresent("shuffle"));
    ASSERT(flags.IsPresent("new"));
    ASSERT(!flags.IsPresent("newt"));

    ASSERT(flags.GetFlagSet() == "wibble;ptoing;shuffle;new");

    flags.Clear("shuffle");
    ASSERT(flags.IsPresent("wibble"));
    ASSERT(flags.IsPresent("ptoing"));
    ASSERT(!flags.IsPresent("shuffle"));
    ASSERT(flags.IsPresent("new"));
    ASSERT(!flags.IsPresent("newt"));

    flags.Clear("wibble");
    flags.Clear("ptoing");
    flags.Clear("new");

    ASSERT(!flags.IsPresent("wibble"));
    ASSERT(!flags.IsPresent("ptoing"));
    ASSERT(!flags.IsPresent("shuffle"));
    ASSERT(!flags.IsPresent("new"));
    ASSERT(!flags.IsPresent("newt"));

    flags.Set("blue");
    ASSERT(!flags.IsPresent("wibble"));
    ASSERT(!flags.IsPresent("ptoing"));
    ASSERT(!flags.IsPresent("shuffle"));
    ASSERT(!flags.IsPresent("new"));
    ASSERT(!flags.IsPresent("newt"));
    ASSERT(flags.IsPresent("blue"));

    ASSERT(flags.GetFlagSet() == "blue");

    StringFlagSet flags2("");
    ASSERT(!flags2.IsPresent("red"));
    flags2.Set("red");
    ASSERT(flags2.IsPresent("red"));

    ASSERT(flags2.GetFlagSet() == "red");

    return 0;
}
#endif // TEST
