// $Id: demo_convert.cpp,v 1.7 2000/07/01 07:47:59 eldamitri Exp $
//
//  The authors have released ID3Lib as Public Domain (PD) and claim no
//  copyright, patent or other intellectual property protection in this work.
//  This means that it may be modified, redistributed and used in commercial
//  and non-commercial software and hardware without restrictions.  ID3Lib is
//  distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
//  express or implied.
//  
//  The ID3Lib authors encourage improvements and optimisations to be sent to
//  the ID3Lib coordinator, currently Dirk Mahoney (dirk@id3.org).  Approved
//  submissions may be altered, and will be included and released under these
//  terms.

#include <iostream.h>
#include <id3/tag.h>
#include <getopt.h>

#define VERSION_NUMBER "$Revision: 1.7 $"

void PrintUsage(char *sName)
{
  cout << "Usage: " << sName << " [OPTION]... [FILE]..." << endl;
  cout << "Converts between id3v1 and id3v2 tags of an mp3 file." << endl;
  cout << endl;
  cout << "  -1, --v1tag     Render only the id3v1 tag" << endl;
  cout << "  -2, --v2tag     Render only the id3v2 tag" << endl;
  cout << "  -s, --strip     Strip, rather than render, the tags" << endl;
  cout << "  -p, --padding   Use padding in the tag" << endl;
  cout << "  -h, --help      Display this help and exit" << endl;
  cout << "  -v, --version   Display version information and exit" << endl;
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
  {
    if (VERSION_NUMBER[nIndex] == ' ') 
    {
      break;
    }
  }
  nIndex++;
  for (; nIndex < strlen (VERSION_NUMBER); nIndex++)
  {
    if (VERSION_NUMBER[nIndex] == ' ') 
    {
      break;
    }
    cout << VERSION_NUMBER[nIndex];
  }
  cout << endl;
  cout << "Uses " << ID3LIB_FULL_NAME << endl << endl;

  cout << "This program converts and strips ID3v1/1.1 and Lyrics3 v2.0" << endl;
  cout << "tags to ID3v2 tags." << endl << endl;
}

void DisplayTags(ostream &os, luint nTags)
{
  if (!((nTags & ID3TT_ID3V1) || (nTags & ID3TT_ID3V2)))
  {
    os << "no tag";
  }
  if (nTags & ID3TT_ID3V1)
  {
    os << "v1";
  }
  if ((nTags & ID3TT_ID3V1) && (nTags & ID3TT_ID3V2))
  {
    os << " and ";
  }
  if (nTags & ID3TT_ID3V2)
  {
    os << "v2";
  }
}

int main( unsigned int argc, char *argv[])
{
  flags_t ulFlag = ID3TT_ALL;
  int iOpt;
  bool bError = false;
  bool bStrip = false;
  bool bPadding = false;
  while (true)
  {
    int option_index = 0;
    int iLongOpt = 0;
    static struct option long_options[] = 
    { 
      { "v1tag",   no_argument, &iLongOpt, '1' },
      { "v2tag",   no_argument, &iLongOpt, '2' },
      { "version", no_argument, &iLongOpt, 'v' },
      { "help",    no_argument, &iLongOpt, 'h' },
      { "strip",   no_argument, &iLongOpt, 's' },
      { "padding",   no_argument, &iLongOpt, 'p' },
      { 0, 0, 0, 0 }
    };
    iOpt = getopt_long (argc, argv, "12svhp", long_options, &option_index);

    if (iOpt == -1)
      break;

    if (iOpt == 0) iOpt = iLongOpt;

    switch (iOpt)
    {
      case '1': ulFlag = ID3TT_ID3V1;   break;
      case '2': ulFlag = ID3TT_ID3V2;   break;
      case 's': bStrip = true;          break;
      case 'v': PrintVersion(argv[0]);  exit (0);
      case 'h': PrintUsage(argv[0]);    exit (0);
      case 'p': bPadding = true;        break;
      case '?': bError = true;          break;
      default:
        cout << "?? getopt returned character code " << iOpt << " ??" << endl;
    }
  }
  if (bError)
  {
    cout << "Try `" << argv[0] << " --help' for more information." << endl;
  }
  else 
    for (size_t nIndex = optind; nIndex < argc; nIndex++)
    {
      try
      {
        ID3_Tag myTag;

        if (bStrip)
        {
          cout << "Stripping ";
        }
        else
        {
          cout << "Converting ";
        }
        cout << argv[nIndex] << ": ";

        myTag.Clear();
        myTag.Link(argv[nIndex], ID3TT_ALL);
        myTag.SetPadding(bPadding);

        cout << "attempting ";
        DisplayTags(cout, ulFlag);
        luint nTags;

        if (bStrip)
        {
          nTags = myTag.Strip(ulFlag);
          cout << ", stripped ";
        }
        else
        {
          nTags = myTag.Update(ulFlag);
          cout << ", converted ";
        }

        DisplayTags(cout, nTags);
        cout << endl;
      }

      catch(ID3_Error err)
      {
        cout << endl;
        cout << err.GetErrorFile() << " (" << err.GetErrorLine() << "): "
             << err.GetErrorType() << ": " << err.GetErrorDesc() << endl;
        exit(1);
      }
    }

  return 0;
}


