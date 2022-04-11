/* line_chopper.h
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.4 13-Mar-2003 18:15 rob:)
 */

#ifndef LINE_CHOPPER_H
#define LINE_CHOPPER_H 1

#include <algorithm>

template <typename StringType>
class LineChopper
{
    StringType m_s;

public:
    LineChopper()
    {
    }

    LineChopper(const StringType &s)
	: m_s(s)
    {
    }

#if defined(_MSC_VER)
    class const_iterator {
#else
    class const_iterator : public std::input_iterator<StringType, ptrdiff_t> {
#endif
	const StringType m_s;
	typename StringType::const_iterator m_it;

    public:
	const_iterator(const StringType &s, const typename StringType::const_iterator &it)
	    : m_s(s), m_it(it) { }
	bool operator== (const const_iterator &rhs) const { return IsEqual(rhs); }
	bool operator!= (const const_iterator &rhs) const { return !IsEqual(rhs);}

	void operator++()
	{
	    m_it = std::find(m_it, m_s.end(), '\n');
	    if (m_it != m_s.end())
		++m_it;
	}

	StringType operator*() const
	{
	    typename StringType::const_iterator p = std::find(m_it, m_s.end(), '\n');
	    return StringType(m_it, p);
	}

    private:
	bool IsEqual(const const_iterator &rhs) const { return (m_it == rhs.m_it) && (m_s == rhs.m_s); }
    };

    const_iterator begin() const
    {
	return const_iterator(m_s, m_s.begin());
    }

    const_iterator end() const
    {
	return const_iterator(m_s, m_s.end());
    }
};

#endif /* LINE_CHOPPER_H */
