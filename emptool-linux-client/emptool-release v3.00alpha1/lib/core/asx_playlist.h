/* asx_playlist.h
 *
 * Windows Media Player XML playlists
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 13-Mar-2003 18:15 rob:)
 */

#ifndef ASX_PLAYLIST_H
#define ASX_PLAYLIST_H

class Stream;

#include <vector>
#include <string>

#ifndef EMPEG_STATUS_H
#include "empeg_status.h"
#endif

class ASXPlaylist
{
    typedef std::vector<std::string> entries_t;
    entries_t m_entries;
    std::string m_title;

 public:
    STATUS FromStream(Stream*);

    typedef entries_t::const_iterator const_iterator;
    const_iterator begin() const { return m_entries.begin(); }
    const_iterator end() const { return m_entries.end(); }

    std::string GetTitle() const { return m_title; }
};

#endif
