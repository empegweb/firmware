/* reblocker.cpp
 *
 * (C) 2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "reblocker.h"

Reblocker::~Reblocker()
{
    free(m_pBuffer);
}

/** Append the contents of 'p' to the buffer, possibly 
* resizing it.  Uses the fact that realloc(NULL,...) === malloc(...)
*/
bool Reblocker::Append(const char *p, int cb)
{
    if (!m_pBuffer || m_cbUsed + cb > m_cbBuffer)
    {
	char *temp = (char *)realloc(m_pBuffer, m_cbBuffer + cb);
	if (!temp)
	    return false;
	
	m_pBuffer = temp;
	m_cbBuffer = m_cbBuffer + cb;
    }
    
    memcpy(m_pBuffer + m_cbUsed, p, cb);
    m_cbUsed += cb;
    
    return true;
}

void Reblocker::CopyAndPack(char *buffer, int buffer_size, int *bytes_copied)
{
    int bytes_to_copy = buffer_size;
    if (m_cbUsed < buffer_size)
	bytes_to_copy = m_cbUsed;
    
    memcpy(buffer, m_pBuffer, bytes_to_copy);
    *bytes_copied = bytes_to_copy;
    
    memmove(m_pBuffer, m_pBuffer + bytes_to_copy, m_cbUsed - bytes_to_copy);
    m_cbUsed -= bytes_to_copy;
}

void Reblocker::Discard()
{
    free(m_pBuffer);
    m_pBuffer = NULL;
    m_cbBuffer = 0;
    m_cbUsed = 0;
}

#ifdef TEST
class DataSource
{
    const char * const *m_input_data;
    int m_input_count;
    int m_input_iterator;
    Reblocker m_reblocker;

public:
    DataSource(const char * const *input_data, int input_count)
	: m_input_data(input_data), m_input_count(input_count), m_input_iterator(0)
    {
    }

    void Read(char *buffer, int buffer_size, int *bytes_read);
};

int main(void)
{
    const char * const small_data[] = {
	"abc", "def", "ghi", "jkl", "mno", "pqr", "stu", "vwx", "yz",
    };

    const char * const large_data[] = {
	"abcdefg", "hijklmn", "opqrstu", "vwxyz",
    };

    // BLOCK: Reblock small packets into bigger packets.
    {
	DataSource ds(small_data, sizeof(small_data) / sizeof(small_data[0]));

	int output_count = 0;
	int bytes_read;
	do
	{
	    char buffer[7];
	    ds.Read(buffer, 7, &bytes_read);

	    ASSERT(bytes_read == (int)strlen(large_data[output_count]));
	    ASSERT(strncmp(large_data[output_count], buffer, bytes_read) == 0);
	    ++output_count;
	} while(bytes_read == 7);
    }

    // BLOCK: Reblock large packets into smaller packets.
    {
	DataSource ds(large_data, sizeof(large_data) / sizeof(large_data[0]));

	int output_count = 0;
	int bytes_read;
	do
	{
	    char buffer[3];
	    ds.Read(buffer, 3, &bytes_read);

	    ASSERT(bytes_read == (int)strlen(small_data[output_count]));
	    ASSERT(strncmp(small_data[output_count], buffer, bytes_read) == 0);
	    ++output_count;
	} while(bytes_read == 3);
    }

    return 0;
}

void DataSource::Read(char *buffer, int buffer_size, int *bytes_read)
{
    while (m_reblocker.GetSize() < buffer_size && m_input_iterator < m_input_count)
    {
	const char *packet = m_input_data[m_input_iterator];
	m_reblocker.Append(packet, strlen(packet));
	++m_input_iterator;
    }

    m_reblocker.CopyAndPack(buffer, buffer_size, bytes_read);
}
#endif /* TEST */
