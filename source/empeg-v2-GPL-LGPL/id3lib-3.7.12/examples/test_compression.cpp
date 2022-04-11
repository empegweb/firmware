// $Id: test_compression.cpp,v 1.4 2000/06/23 00:33:45 eldamitri Exp $

#include <iostream.h>
#include <id3/tag.h>

int main( int argc, char *argv[])
{
  try
  {
    ID3_Tag tag;
    ID3_Frame frame;

    tag.Link("test-230-compressed.tag");
    tag.Strip(ID3TT_ALL);
    tag.Clear();

    frame.SetID(ID3FID_USERTEXT);
    frame.Field(ID3FN_DESCRIPTION).Set("compression example");
    frame.Field(ID3FN_TEXT).Set("This sample user text frame came from an ID3v2-3.0 tag.  The frame has the 'compression' bit set in it's frame header.  This is the new method for compressing frames, which supercedes the 2.01 Compressed Data Metaframe.");
    frame.SetCompression(true);
    tag.AddFrame(frame);

    tag.SetPadding(false);
    tag.SetUnsync(false);
    tag.Update(ID3TT_ID3V2);
  }   
  catch(const ID3_Error& err)
  {
    cout << endl;
    cout << err.GetErrorFile() << " (" << err.GetErrorLine() << "): "
         << err.GetErrorType() << ": " << err.GetErrorDesc() << endl;
  }

  return 0;
}
