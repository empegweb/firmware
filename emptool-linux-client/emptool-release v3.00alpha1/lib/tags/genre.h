/* genre.h
 *
 * List of ID3 genres
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 13-Mar-2003 18:15 rob:)
 */

#ifndef GENRE_H
#define GENRE_H 1

#include <string>
#include "empeg_tchar.h"

class GenreList
{
    static const TCHAR *const genres[];
    static int count;
    
public:
    static const TCHAR *Lookup(int);

    /** Returns the genre number for the specified genre, or -1 if not found */
    static int Find(const TCHAR *);
    static int GetCount();
};

#endif
