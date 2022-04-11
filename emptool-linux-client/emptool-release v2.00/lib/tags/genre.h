/* genre.h
 *
 * List of ID3 genres
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 01-Apr-2003 18:52 rob:)
 */

#ifndef GENRE_H
#define GENRE_H 1

class GenreList
{
    static const char *const genres[];
    static int count;
    
public:
    static const char *Lookup(int);
    static int Find(const char *);
    static int GetCount();
};

#endif
