/* id3v1tags.h
 *
 * (C) 2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * Updates Id3V1 tags in mp3 files
 *
 * @bug:
 * !!!! DEPRECATED.  Jupiter still uses it, though.  It should use the other
 * stuff from lib/tags.
 *
 * (:Empeg Source Release 1.1 13-Mar-2003 18:15 rob:)
 */

#include "empeg_error.h"

#include "tags/id3v1_format.h"

#include <string>

class Id3V1Tags
{
private:
    bool m_valid;
    std::string m_title;
    std::string m_artist;
    std::string m_album;
    std::string m_year;
    std::string m_comment;
    unsigned char m_track;
    unsigned char m_genre;
    
public:
    Id3V1Tags() :
        m_valid(false),
        m_title(""),
        m_artist(""),
        m_album(""),
        m_year(""),
        m_comment(""),
        m_track(0),
        m_genre(0)
    {
    }

    void SetTitle(const std::string &title) { m_title = title; }
    void SetArtist(const std::string &artist) { m_artist = artist; }
    void SetAlbum(const std::string &album) { m_album = album; }
    void SetYear(const std::string &year) { m_year = year; }
    void SetComment(const std::string &comment) { m_comment = comment; }
    void SetTrack(unsigned track) { m_track = (unsigned char) track; }
    void SetGenre(const std::string &genre);
    
    STATUS UpdateTags(const char *filename);
};
