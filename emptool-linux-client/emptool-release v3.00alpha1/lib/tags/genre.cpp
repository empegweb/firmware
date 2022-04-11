/* genre.cpp
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

#include "config.h"
#include "trace.h"
#include "genre.h"
#include "stringpred.h"

const TCHAR *const GenreList::genres[] = {
    _T("Blues"),		// 0
    _T("Classic Rock"),
    _T("Country"),
    _T("Dance"),
    _T("Disco"),
    _T("Funk"),
    _T("Grunge"),
    _T("Hip-Hop"),
    _T("Jazz"),
    _T("Metal"),

    _T("New Age"),		// 10
    _T("Oldies"),
    _T("Other"),
    _T("Pop"),
    _T("R&B"),
    _T("Rap"),
    _T("Reggae"),
    _T("Rock"),
    _T("Techno"),
    _T("Industrial"),

    _T("Alternative"),	// 20
    _T("Ska"),
    _T("Death Metal"),
    _T("Pranks"),
    _T("Soundtrack"),
    _T("Euro-Techno"),
    _T("Ambient"),
    _T("Trip-Hop"),
    _T("Vocal"),
    _T("Jazz+Funk"),

    _T("Fusion"),		// 30
    _T("Trance"),
    _T("Classical"),
    _T("Instrumental"),
    _T("Acid"),
    _T("House"),
    _T("Game"),
    _T("Sound Clip"),
    _T("Gospel"),
    _T("Noise"),

    _T("AlternRock"),	// 40
    _T("Bass"),
    _T("Soul"),
    _T("Punk"),
    _T("Space"),
    _T("Meditative"),
    _T("Instrumental Pop"),
    _T("Instrumental Rock"),
    _T("Ethnic"),
    _T("Gothic"),

    _T("Darkwave"),		// 50
    _T("Techno-Industrial"),
    _T("Electronic"),
    _T("Pop-Folk"),
    _T("Eurodance"),
    _T("Dream"),
    _T("Southern Rock"),
    _T("Comedy"),
    _T("Cult"),
    _T("Gangsta"),

    _T("Top 40"),		// 60
    _T("Christian Rap"),
    _T("Pop/Funk"),
    _T("Jungle"),
    _T("Native American"),
    _T("Cabaret"),
    _T("New Wave"),
    _T("Psychadelic"),
    _T("Rave"),
    _T("Showtunes"),

    _T("Trailer"),		// 70
    _T("Lo-Fi"),
    _T("Tribal"),
    _T("Acid Punk"),
    _T("Acid Jazz"),
    _T("Polka"),
    _T("Retro"),
    _T("Musical"),
    _T("Rock & Roll"),
    _T("Hard Rock"),

    _T("Folk"),		// 80
    _T("Folk-Rock"),
    _T("National Folk"),
    _T("Swing"),
    _T("Fast Fusion"),
    _T("Bebob"),
    _T("Latin"),
    _T("Revival"),
    _T("Celtic"),
    _T("Bluegrass"),

    _T("Avantgarde"),	// 90
    _T("Gothic Rock"),
    _T("Progressive Rock"),
    _T("Psychedelic Rock"),
    _T("Symphonic Rock"),
    _T("Slow Rock"),
    _T("Big Band"),
    _T("Chorus"),
    _T("Easy Listening"),
    _T("Acoustic"),

    _T("Humour"),		// 100
    _T("Speech"),
    _T("Chanson"),
    _T("Opera"),
    _T("Chamber Music"),

    _T("Sonata"),
    _T("Symphony"),
    _T("Booty Bass"),
    _T("Primus"),
    _T("Porn Groove"),

    _T("Satire"),		// 110
    _T("Slow Jam"),
    _T("Club"),
    _T("Tango"),
    _T("Samba"),

    _T("Folklore"),
    _T("Ballad"),
    _T("Power Ballad"),
    _T("Rhythmic Soul"),
    _T("Freestyle"),

    _T("Duet"),		// 120
    _T("Punk Rock"),
    _T("Drum Solo"),
    _T("Acapella"),
    _T("Euro-House"),

    _T("Dance Hall"),
    _T("Goa"),		// 126
    _T("Drum & Bass"),
    _T("Club-House"),
    _T("Hardcore"),

    _T("Terror"),		// 130
    _T("Indie"),
    _T("Britpop"),
    _T("Negerpunk"),
    _T("Polsk Punk"),

    _T("Beat"),
    _T("Christian Gangsta"),
    _T("Heavy Metal"),
    _T("Black Metal"),
    _T("Crossover"),

    _T("Contemporary C"),	// 140
    _T("Christian Rock"),
    _T("Merengue"),
    _T("Salsa"),
    _T("Thrash Metal"),
    _T("Anime"),
    _T("JPop"),
    _T("Synth Pop"),	// 147
    _T("Sweetcorn Dub"),     // 148
};

int GenreList::count = sizeof(genres)/sizeof(genres[0]);

int GenreList::GetCount()
{
    ASSERT(count == 149);
    return count;
}

const TCHAR *GenreList::Lookup(int i)
{
    ASSERT(count == 149);

    if (i < 0 || i >= count)
	return _T("");
    else
	return genres[i];
}

int GenreList::Find(const TCHAR *s)
{
    ASSERT(count == 149);

    for(int i =0; i < count; i++)
    {
	stringpred::IgnoreCaseEq eq;

	if (eq(s, genres[i]))
	    return i;
    }
    return -1;
}
