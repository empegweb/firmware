/* reblocker.h
 *
 * (C) 2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 13-Mar-2003 18:15 rob:)
 */

/** Example code:
 * void DataSource::Read(char *buffer, int buffer_size, int *bytes_read)
 * {
 *	while (m_reblocker.GetSize() < buffer_size && more_data)
 *	{
 *	    m_reblocker.Append(data_packet, data_packet_size);
 *	    NextDataPacket();
 *	}
 *
 *	m_reblocker.CopyAndPack(buffer, buffer_size, bytes_read);
 * }
 */
class Reblocker
{
    char *m_pBuffer;
    int m_cbBuffer;
    int m_cbUsed;

public:
    Reblocker()
	: m_pBuffer(NULL), m_cbBuffer(0), m_cbUsed(0)
    {
    }

    ~Reblocker();
    int GetSize() const { return m_cbUsed; }

    /** Append the contents of 'p' to the buffer, possibly 
     * resizing it.  Uses the fact that realloc(NULL,...) === malloc(...)
     */
    bool Append(const char *p, int cb);
    void CopyAndPack(char *buffer, int buffer_size, int *bytes_copied);

    void Discard();
};

