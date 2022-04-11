/* trace_buffer.cpp
 *
 * Typesafe tracing 
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * Authors:
 *   Peter Hartley <peter@empeg.com>
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 */

#include "trace.h"
#include "var_string.h"
#include "utf8.h"

#ifdef EMPEG_ENABLE_TRACE

TraceBuffer& TraceBuffer::operator<<(int i)
{
    m_buffer += VarString::Printf("%d", i);
    return *this;
}

TraceBuffer& TraceBuffer::operator<<(const char *s)
{
    m_buffer += s;
    return *this;
}

TraceBuffer& TraceBuffer::operator<<(const wchar_t *w)
{
    std::string s;
    while (*w)
    {
	unsigned int c = (unsigned int)(*w++);

	if (c < 256 && c != 127)
	    s += (char)c;
	else
	    s += VarString::Printf("\\x%x", c);
    }
    m_buffer += s;
    return *this;
}

TraceBuffer::~TraceBuffer()
{
    std::string s(m_buffer);

    if (m_emit)
	empeg_trace(m_file, m_line, "%s", s.c_str());
}

#endif // EMPEG_ENABLE_TRACE

#ifdef TEST
int main()
{
    TRACER << "Hello " << 3 << L" there\x3e00\n";
    TRACERC(0) << "Not there\n";
    TRACERC(1) << "Hello " << 3 << L" there\n";
    return 0;
}
#endif
