/* pstring.h
 *
 * Pascal-style strings (count+data)
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.11 13-Mar-2003 18:15 rob:)
 */

#ifndef PSTRING_H
#define PSTRING_H

#include <string>

class pstring
{
    unsigned char *m_data;
 public:
    inline explicit pstring(unsigned char *data)
	: m_data(data) { }

    inline explicit pstring(const unsigned char *data)
	: m_data(const_cast<unsigned char *>(data)) { }
    
    inline char &operator[](size_t pos) { return (char &) m_data[pos+1]; }
    inline const char &operator[](size_t pos) const { return (const char &) m_data[pos+1]; }
    inline int size() const { return (int) *m_data; }
    inline unsigned char *value() const { return m_data; }

    inline std::string tostring() const {
	return std::string((const char *) m_data+1, (size_t) *m_data);
    }
    
    int cmp(const pstring &other_str) const;
    int cmp(const char *other) const;
    int ncmp(const pstring &other_str, size_t len) const;
    int ncmp(const char *other, size_t len) const;
    int casecmp(const pstring &other_str) const;
    int casecmp(const char *other) const;
    int ncasecmp(const pstring &other_str, size_t len) const;
    int ncasecmp(const char *other, size_t len) const;

    bool caseeq(const pstring& other) const
	{
	    if ( m_data[0] != other.m_data[0] )
		return false;
	    return casecmp(other) == 0;
	}
};

inline bool operator==(const pstring &s1, const pstring &s2)
{
    return s1.cmp(s2) == 0;
}

inline bool operator!=(const pstring &s1, const pstring &s2)
{
    return s1.cmp(s2) != 0;
}

inline bool operator<(const pstring &s1, const pstring &s2)
{
    return s1.cmp(s2) < 0;
}

inline bool operator<=(const pstring &s1, const pstring &s2)
{
    return s1.cmp(s2) <= 0;
}

inline bool operator>(const pstring &s1, const pstring &s2)
{
    return s1.cmp(s2) > 0;
}

inline bool operator>=(const pstring &s1, const pstring &s2)
{
    return s1.cmp(s2) >= 0;
}

#endif
