// $Id: demo_tag.cpp,v 1.6 2000/05/29 02:19:43 eldamitri Exp $

#include <iostream.h>
#include <getopt.h>
#include <stdlib.h>

#include <id3/tag.h>
#include <id3/misc_support.h>

#define VERSION_NUMBER "$Revision: 1.6 $"

void PrintUsage(char *sName)
{
  cout << "Usage: " << sName << " [OPTION]... [FILE]..." << endl;
  cout << "Tags an mp3 file with id3v1 and/or id3v2 tags." << endl;
  cout << endl;
  cout << "  -1, --v1tag              Render only the id3v1 tag" << endl;
  cout << "  -2, --v2tag              Render only the id3v2 tag" << endl;
  cout << "  -h, --help               Display this help and exit" << endl;
  cout << "  -v, --version            Display version information and exit" << endl;
  cout << "  -a, --artist  \"ARTIST\"   Set the artist information" << endl;
  cout << "  -s, --song    \"SONG\"     Set the song title information" << endl;
  cout << "  -A, --album   \"ALBUM\"    Set the album title information" << endl;
  cout << "  -c, --comment \"COMMENT\"  Set the comment infromation" << endl;
  cout << "  -C, --desc \"DESCRIPTION\" Set the comment description" << endl;
  cout << "  -g, --genre   num        Set the genre number" << endl;
  cout << "  -y, --year    num        Set the year" << endl;
  cout << "  -t, --track   num        Set the track number" << endl;
  cout << "  -T, --total   num        Set the total number of tracks on the album" << endl;
  cout << endl;
  cout << "Will render both types of tag by default.  Only the last" << endl
       << "tag type indicated in the option list will be used.  Non-" << endl
       << "rendered will remain unchanged in the original file.  Will" << endl
       << "also parse and convert Lyrics3 v2.0 frames, but will not" << endl
       << "render them." << endl;
}

void PrintVersion(char *sName)
{
  size_t nIndex;
  cout << sName << " ";
  for (nIndex = 0; nIndex < strlen(VERSION_NUMBER); nIndex++)
    if (VERSION_NUMBER[nIndex] == ' ') break;
  nIndex++;
  for (; nIndex < strlen (VERSION_NUMBER); nIndex++)
  {
    if (VERSION_NUMBER[nIndex] == ' ') break;
    cout << VERSION_NUMBER[nIndex];
  }
  cout << endl;
  cout << "Uses " << ID3LIB_FULL_NAME << endl << endl;

  cout << "This program tags mp3 files with ID3v1/1.1 and/or id3v2 tags" << endl;
}

void DisplayTags(ostream &os, luint nTags)
{
  if (!((nTags & ID3TT_ID3V1) || (nTags & ID3TT_ID3V2)))
    os << "no tag";
  if (nTags & ID3TT_ID3V1)
    os << "v1";
  if ((nTags & ID3TT_ID3V1) && (nTags & ID3TT_ID3V2))
    os << " and ";
  if (nTags & ID3TT_ID3V2)
    os << "v2";
}

int main( unsigned int argc, char *argv[])
{
  int ulFlag = ID3TT_ID3;
  int iOpt;
  bool bError = false;
  char
    *sArtist  = NULL,
    *sAlbum   = NULL,
    *sSong    = NULL,
    *sGenre   = NULL,
    *sYear    = NULL,
    *sComment = NULL,
    *sDesc    = NULL,
    *sTrack   = NULL,
    *sTotal   = NULL;
  size_t 
    nTrack    = 0,
    nTotal    = 0,
    nGenre    = 0;
  while (true)
  {
    int option_index = 0;
    int iLongOpt = 0;
    static struct option long_options[] = 
    { 
      { "v1tag",   no_argument,       &iLongOpt, '1' },
      { "v2tag",   no_argument,       &iLongOpt, '2' },
      { "version", no_argument,       &iLongOpt, 'v' },
      { "help",    no_argument,       &iLongOpt, 'h' },
      { "artist",  required_argument, &iLongOpt, 'a' },
      { "album",   required_argument, &iLongOpt, 'A' },
      { "song",    required_argument, &iLongOpt, 's' },
      { "genre",   required_argument, &iLongOpt, 'g' },
      { "year",    required_argument, &iLongOpt, 'y' },
      { "comment", required_argument, &iLongOpt, 'c' },
      { "desc",    required_argument, &iLongOpt, 'C' },
      { "track",   required_argument, &iLongOpt, 't' },
      { "total",   required_argument, &iLongOpt, 'T' },
      { 0, 0, 0, 0 }
    };
    iOpt = getopt_long (argc, argv, "12vha:A:s:g:y:c:t:T:C:",
                        long_options, &option_index);

    if (iOpt == -1)
      break;

    if (iOpt == 0) iOpt = iLongOpt;

    switch (iOpt)
    {
      case '1': ulFlag   = ID3TT_ID3V1;      break;
      case '2': ulFlag   = ID3TT_ID3V2;      break;
      case 'a': sArtist  = optarg;      break;
      case 'A': sAlbum   = optarg;      break;
      case 's': sSong    = optarg;      break;
      case 'g': sGenre   = optarg;      break;
      case 'y': sYear    = optarg;      break;
      case 'c': sComment = optarg;      break;
      case 'C': sDesc    = optarg;      break;
      case 't': sTrack   = optarg;      break;
      case 'T': sTotal   = optarg;      break;
      case '?': bError   = true;        break;
      case 'v': PrintVersion(argv[0]);  exit (0);
      case 'h': PrintUsage(argv[0]);    exit (0);
      default:
        cout << "?? getopt returned character code " << iOpt << " ??" << endl;
    }
  }
  if (bError)
  {
    cout << "Try `" << argv[0] << " --help' for more information." << endl;
  }
  else 
  {
    char *sEmpty = "";
    if (sArtist)
    {
      cout << "+++ Artist  = " << sArtist << endl;
    }
    if (sAlbum)
    {
      cout << "+++ Album   = " << sAlbum << endl;
    }
    if (sSong)
    {
      cout << "+++ Song    = " << sSong << endl;
    }
    if (sYear)
    {
      cout << "+++ Year    = " << sYear << endl;
    }
    if (sComment)
    {
      cout << "+++ Comment = " << sComment << endl;
      if (!sDesc)
      {
        sDesc = sEmpty;
      }
      cout << "+++ Comment Description" << endl;
      cout << "            = " << sDesc << endl;
    }
    if (sGenre)
    {
      nGenre = atoi(sGenre);
      cout << "+++ Genre   = " << nGenre << endl;
    }
    if (sTrack)
    {
      nTrack = atoi(sTrack);
      if (nTrack > 0)
      {
        cout << "+++ Track   = " << nTrack;
      }
    }
    if (sTotal)
    {
      nTotal = atoi(sTotal);
      if (nTrack > 0 && nTotal > 0)
      {
        cout << "+++ Total   = " << sTotal << endl;
      }
    }
    for (size_t nIndex = optind; nIndex < argc; nIndex++)
    {
      try
      {
        ID3_Tag myTag;

        cout << "Tagging " << argv[nIndex] << ": ";

        myTag.Link(argv[nIndex]);

        cout << "attempting ";
        DisplayTags(cout, ulFlag);

        if (sArtist)
        {
          ID3_AddArtist(&myTag, sArtist, true);
        }
        if (sAlbum)
        {
          ID3_AddAlbum(&myTag, sAlbum, true);
        }
        if (sSong)
        {
          ID3_AddTitle(&myTag, sSong, true);
        }
        if (sYear)
        {
          ID3_AddYear(&myTag, sYear, true);
        }
        if (sComment)
        {
          ID3_AddComment(&myTag, sComment, sDesc, true);
        }
        if (nGenre > 0)
        {
          ID3_AddGenre(&myTag, nGenre, true);
        }
        if (nTrack > 0)
        {
          ID3_AddTrack(&myTag, nTrack, nTotal, true);
        }
        luint nTags = myTag.Update(ulFlag);
        cout << ", tagged ";

        DisplayTags(cout, nTags);

        cout << endl;
      }
      
      catch(ID3_Error err)
      {
        cout << endl;
        cout << err.GetErrorFile() << " (" << err.GetErrorLine() << "): "
             << err.GetErrorType() << ": " << err.GetErrorDesc() << endl;
      }
    }
  }

  return 0;
}
