/* rle_encoder.h
 *
 * (C) 2003 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 13-Mar-2003 18:15 rob:)
 */

#ifndef RLE_ENCODER_H
#define RLE_ENCODER_H	1

#include <list>

/* Hideously inefficient generic Run Length Encoder
 *
 * Usage:
 *
 *     RleEncoder<unsigned> encoder;
 *
 *     while(not_eof)
 *         encoder.Append(the_next_value);
 *     encoder.Flush();
 *
 *     for(RleEncoder::iterator it = encoder.begin(); it!=encoder.end(); ++it)
 *         printf("%u %u\n", it->count, it->value);
 *
 *     encoder.Reset();
 *     ...
 */

template <class T>
class RleEncoder
{
public:
    struct RleCode
    {
	unsigned count;
	T value;
	inline RleCode(unsigned c, T v) : count(c), value(v) { }
    };
    typedef std::list<RleCode> RleCodeList;
    typedef RleCodeList::const_iterator const_iterator;

private:
    unsigned m_count, m_max_count;
    T m_last_value;
    RleCodeList m_codes;
    
public:
    RleEncoder(unsigned max_count) : m_count(0), m_max_count(max_count) { }
    void Reset();
    void Append(const T &value);
    void Flush();
    inline const_iterator begin() const { return m_codes.begin(); }
    inline const_iterator end() const { return m_codes.end(); }
};

template <class T>
void RleEncoder<T>::Reset()
{
    m_count = 0;
    m_last_value = T();
    m_codes = RleCodeList();
}

template <class T>
void RleEncoder<T>::Append(const T &value)
{
    if(m_count)
    {
	if(m_count >= m_max_count || value != m_last_value)
	{
	    m_codes.push_back(RleCode(m_count, m_last_value));
	    m_last_value = value;
	    m_count = 1;
	}
	else
	    ++m_count;
    }
    else
    {
	m_last_value = value;
	m_count = 1;
    }
}

template <class T>
void RleEncoder<T>::Flush()
{
    if(m_count)
    {
	m_codes.push_back(RleCode(m_count, m_last_value));
	m_count = 0;
    }
}

#endif
