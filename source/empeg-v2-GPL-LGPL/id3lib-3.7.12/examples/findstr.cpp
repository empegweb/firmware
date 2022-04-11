#include <id3/tag.h>
#include <id3/misc_support.h>

main(unsigned argc, char* argv[])
{
  if (argc != 2)
  {
    cout << "Usage: findstr <tagfile>" << endl;
    exit(1);
  }
  ID3_Tag tag(argv[1]);
  ID3_Frame* first = NULL, *frame = NULL;
  while(frame = tag.Find(ID3FID_COMMENT, ID3FN_DESCRIPTION, ""))
  {
    if (frame == first)
    {
      break;
    }
    if (first == NULL)
    {
      first = frame;
    }
    char* comment = ID3_GetString(frame, ID3FN_TEXT);
    cout << "*** Found comment w/o description: " << comment << endl;
    delete [] comment;
  }
  exit(0);
}
