/* genre.cpp
 *
 * List of ID3 genres
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3.6.1 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "genre.h"
#include <string.h>

const char *const GenreList::genres[] = {
    "Blues",		// 0
    "Classic Rock",
    "Country",
    "Dance",
    "Disco",
    "Funk",
    "Grunge",
    "Hip-Hop",
    "Jazz",
    "Metal",

    "New Age",		// 10
    "Oldies",
    "Other",
    "Pop",
    "R&B",
    "Rap",
    "Reggae",
    "Rock",
    "Techno",
    "Industrial",

    "Alternative",	// 20
    "Ska",
    "Death Metal",
    "Pranks",
    "Soundtrack",
    "Euro-Techno",
    "Ambient",
    "Trip-Hop",
    "Vocal",
    "Jazz+Funk",

    "Fusion",		// 30
    "Trance",
    "Classical",
    "Instrumental",
    "Acid",
    "House",
    "Game",
    "Sound Clip",
    "Gospel",
    "Noise",

    "AlternRock",	// 40
    "Bass",
    "Soul",
    "Punk",
    "Space",
    "Meditative",
    "Instrumental Pop",
    "Instrumental Rock",
    "Ethnic",
    "Gothic",

    "Darkwave",		// 50
    "Techno-Industrial",
    "Electronic",
    "Pop-Folk",
    "Eurodance",
    "Dream",
    "Southern Rock",
    "Comedy",
    "Cult",
    "Gangsta",

    "Top 40",		// 60
    "Christian Rap",
    "Pop/Funk",
    "Jungle",
    "Native American",
    "Cabaret",
    "New Wave",
    "Psychadelic",
    "Rave",
    "Showtunes",

    "Trailer",		// 70
    "Lo-Fi",
    "Tribal",
    "Acid Punk",
    "Acid Jazz",
    "Polka",
    "Retro",
    "Musical",
    "Rock & Roll",
    "Hard Rock",

    "Folk",		// 80
    "Folk-Rock",
    "National Folk",
    "Swing",
    "Fast Fusion",
    "Bebob",
    "Latin",
    "Revival",
    "Celtic",
    "Bluegrass",

    "Avantgarde",	// 90
    "Gothic Rock",
    "Progressive Rock",
    "Psychedelic Rock",
    "Symphonic Rock",
    "Slow Rock",
    "Big Band",
    "Chorus",
    "Easy Listening",
    "Acoustic",

    "Humour",		// 100
    "Speech",
    "Chanson",
    "Opera",
    "Chamber Music",

    "Sonata",
    "Symphony",
    "Booty Bass",
    "Primus",
    "Porn Groove",

    "Satire",		// 110
    "Slow Jam",
    "Club",
    "Tango",
    "Samba",

    "Folklore",
    "Ballad",
    "Power Ballad",
    "Rhythmic Soul",
    "Freestyle",

    "Duet",		// 120
    "Punk Rock",
    "Drum Solo",
    "Acapella",
    "Euro-House",

    "Dance Hall",
    "Goa",		// 126
    "Drum & Bass",
    "Club-House",
    "Hardcore",

    "Terror",		// 130
    "Indie",
    "Britpop",
    "Negerpunk",
    "Polsk Punk",

    "Beat",
    "Christian Gangsta",
    "Heavy Metal",
    "Black Metal",
    "Crossover",

    "Contemporary C",	// 140
    "Christian Rock",
    "Merengue",
    "Salsa",
    "Thrash Metal",
    "Anime",
    "JPop",
    "Synth Pop",	// 147

    "Sweetcorn Dub",    // 148
};

int GenreList::count = sizeof(genres)/sizeof(genres[0]);

int GenreList::GetCount()
{
    return count;
}

const char *GenreList::Lookup(int i)
{
    if (i < 0 || i >= count)
	return "";
    else
	return genres[i];
}

#ifdef WIN32
#define strcasecmp _stricmp
#endif

int GenreList::Find(const char *s)
{
    for(int i =0; i < count; i++)
    {
	if (strcasecmp(s, genres[i]) == 0)
	    return i;
    }
    return -1;
}
