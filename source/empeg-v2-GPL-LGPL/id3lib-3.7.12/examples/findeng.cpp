#include <id3/tag.h>
#include <id3/misc_support.h>

main(unsigned argc, char* argv[])
{
  if (argc != 2)
  {
    cout << "Usage: findeng <tagfile>" << endl;
    exit(1);
  }
  ID3_Tag tag(argv[1]);
  const ID3_Frame* frame = tag.Find(ID3FID_COMMENT, ID3FN_LANGUAGE, "eng");
  if (frame)
  {
    char* comment = ID3_GetString(frame, ID3FN_TEXT);
    cout << "*** Found english comment: " << comment << endl;
    delete [] comment;
  }
  else
  {
    cout << "*** No english comment to be found." << endl;
  }
  exit(0);
}
