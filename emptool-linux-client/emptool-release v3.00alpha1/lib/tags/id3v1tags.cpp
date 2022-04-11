/* id3v1tags.cpp
 *
 * (C) 2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * Updates Id3V1 tags in mp3 files
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 */

#include "id3v1tags.h"
#include "tags/genre.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
 
/** @todo Make the stuff in lib/tags writable, and then make this use it
 * (Potentially use this as the base of that functionality).
 */
void Id3V1Tags::SetGenre(const std::string &genre)
{
    m_genre = (unsigned char) GenreList::Find(genre.c_str());
}

STATUS Id3V1Tags::UpdateTags(const char *filename)
{
    int mp3_fd = open (filename, O_RDWR);
    
    if (mp3_fd == -1)
    {
        TRACE_WARN ("failed to open %s for reading\n", filename);
        return MakeErrnoStatus();
    }
    
    off_t newpos = lseek(mp3_fd, (off_t) -128, SEEK_END);
    if (newpos == (off_t) -1)	// Will rarely fail
    {
        close(mp3_fd);
        return MakeErrnoStatus();
    }    

    /** @todo Use STATIC_CHECK for this. */
    ASSERT(sizeof (struct ID3V1_Tag) == 128);

    STATUS status = S_OK;
    
    struct ID3V1_Tag tagblk;
    ssize_t result = read(mp3_fd, (char*) &tagblk, sizeof (tagblk));
    if (result == sizeof (tagblk))
    {
    }
    else if (result == -1)
        status = MakeErrnoStatus();
    else
        status = MakeErrnoStatus(EFAULT);

    if (SUCCEEDED(status))
    {
        if (memcmp (tagblk.signature, "TAG", 3) == 0)
            newpos = lseek(mp3_fd, (off_t) -128, SEEK_END);     // If tag is there then move back to it
        else
            newpos = lseek(mp3_fd, 0, SEEK_END);        // If tag not there then move to the end (this isn't really required!)
        if (newpos == (off_t) -1)
            status = MakeErrnoStatus();                
    }
    
    if (SUCCEEDED(status))
    {
        memset((char*) &tagblk, 0, sizeof (tagblk));
        strncpy (tagblk.signature, "TAG", sizeof (tagblk.signature));        
        strncpy (tagblk.title, m_title.c_str(), sizeof (tagblk.title));
        strncpy (tagblk.artist, m_artist.c_str(), sizeof (tagblk.artist));
        strncpy (tagblk.album, m_album.c_str(), sizeof (tagblk.album));
        strncpy (tagblk.year, m_year.c_str(), sizeof (tagblk.year));
        tagblk.genre = m_genre;
        if (m_track > 0)                                // Write in id3v1.1 form if we have a track number
        {            
            strncpy (tagblk.comment, m_comment.c_str(), sizeof (tagblk.comment) - 2);
            tagblk.comment[ sizeof (tagblk.comment) - 1 ] = m_track;
        }
        else
            strncpy (tagblk.comment, m_comment.c_str(), sizeof (tagblk.comment));        

        ssize_t result = write(mp3_fd, (char*) &tagblk, sizeof (tagblk));
        if (result == sizeof (tagblk))
        {
        }
        else if (result == -1)
            status = MakeErrnoStatus();
        else
            status = MakeErrnoStatus(EFAULT);
    }
    
    close (mp3_fd);
    
    return status;
}
