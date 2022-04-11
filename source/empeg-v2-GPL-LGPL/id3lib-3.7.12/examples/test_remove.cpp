// $Id: test_remove.cpp,v 1.5 2000/07/07 23:01:52 eldamitri Exp $

#include <iostream.h>
#include <id3/tag.h>
#include <id3/misc_support.h>

typedef const char* LPCTSTR;

/* CSharedTag is a wrapper I made for some features I need */
/* LPCTSTR means const char * */
size_t RemoveFrame(ID3_Tag& pTag, ID3_FrameID fID, LPCTSTR sDescription)
{
  size_t nCount = 0;
  const ID3_Frame * frame = NULL;

  do {
    if (!sDescription)
      frame = pTag.Find(fID);
    else
      frame = pTag.Find(fID, ID3FN_DESCRIPTION, sDescription);

    if (frame) {
      /* pTag is an ID3_Tag */
      delete pTag.RemoveFrame(frame);
      nCount++;
    }
  } while (frame != NULL);

  return nCount;
}

int main( int argc, char *argv[])
{
  try
  {
    ID3_Tag tag;
    ID3_Frame frame;

    if (argc == 2)
    {
      tag.Link(argv[1]);
      cerr << "removed " << RemoveFrame(tag, ID3FID_COMMENT, "") << " descriptionless comment frames" << endl;
      tag.Update();
      
    }
    else
    {
      tag.Link("test-remove.tag");
      tag.Strip(ID3TT_ALL);
      tag.Clear();
      
      frame.SetID(ID3FID_TITLE);
      frame.Field(ID3FN_TEXT).Set("Test title");
      tag.AddFrame(frame);
      
      frame.SetID(ID3FID_COMPOSER);
      frame.Field(ID3FN_TEXT).Set("Test composer");
      tag.AddFrame(frame);
      
      frame.SetID(ID3FID_BAND);
      frame.Field(ID3FN_TEXT).Set("Test band");
      tag.AddFrame(frame);
      
      frame.SetID(ID3FID_CONDUCTOR);
      frame.Field(ID3FN_TEXT).Set("Test conductor");
      tag.AddFrame(frame);
      
      frame.SetID(ID3FID_COMMENT);
      frame.Field(ID3FN_LANGUAGE).Set("eng");
      frame.Field(ID3FN_TEXT).Set("Test comment");
      frame.Field(ID3FN_DESCRIPTION).Set("A Description");
      tag.AddFrame(frame);

      frame.SetID(ID3FID_COMMENT);
      frame.Field(ID3FN_LANGUAGE).Set("eng");
      frame.Field(ID3FN_TEXT).Set("Test comment 2");
      frame.Field(ID3FN_DESCRIPTION).Set("");
      tag.AddFrame(frame);

      tag.SetPadding(false);
      tag.Update(ID3TT_ID3V2);

      cerr << "removed " << ID3_RemoveArtists(&tag) << " artist frames" << endl;
      tag.Update();
      cerr << "removed " << ID3_RemoveTitles(&tag) << " title frames" << endl;
      tag.Update();
      cerr << "removed " << RemoveFrame(tag, ID3FID_COMMENT, "") << " descriptionless comment frames" << endl;
      tag.Update();
    }
  }   
  catch(const ID3_Error& err)
  {
    cout << endl;
    cout << err.GetErrorFile() << " (" << err.GetErrorLine() << "): "
         << err.GetErrorType() << ": " << err.GetErrorDesc() << endl;
  }

  return 0;
}
