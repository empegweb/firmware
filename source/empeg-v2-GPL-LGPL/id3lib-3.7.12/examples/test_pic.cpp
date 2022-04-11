// $Id: test_pic.cpp,v 1.5 2000/06/23 00:35:07 eldamitri Exp $

#include <iostream.h>
#include <id3/tag.h>

int main( int argc, char *argv[])
{
  try
  {
    ID3_Tag tag;
    ID3_Frame frame;

    tag.Link("test-230-picture.tag");
    tag.Strip(ID3TT_ALL);
    tag.Clear();

    frame.SetID(ID3FID_TITLE);
    frame.Field(ID3FN_TEXT).Set("Aquarium");
    tag.AddFrame(frame);

    frame.SetID(ID3FID_CONTENTGROUP);
    frame.Field(ID3FN_TEXT).Set("Short fraction of 'Carnival of the Animals: A Grand Zoological Fantasy'");
    tag.AddFrame(frame);

    frame.SetID(ID3FID_COMPOSER);
    frame.Field(ID3FN_TEXT).Set("Camille Saint-Saëns");
    tag.AddFrame(frame);

    frame.SetID(ID3FID_BAND);
    frame.Field(ID3FN_TEXT).Set("Slovakia Radio Symphony Orchestra");
    tag.AddFrame(frame);

    frame.SetID(ID3FID_CONDUCTOR);
    frame.Field(ID3FN_TEXT).Set("Ondrej Lenárd");
    tag.AddFrame(frame);

    frame.SetID(ID3FID_COPYRIGHT);
    frame.Field(ID3FN_TEXT).Set("1996 HNH international Ltd.");
    tag.AddFrame(frame);

    frame.SetID(ID3FID_CONTENTTYPE);
    frame.Field(ID3FN_TEXT).Set("(32)");
    tag.AddFrame(frame);

    frame.SetID(ID3FID_INVOLVEDPEOPLE);
    frame.Field(ID3FN_TEXT).Add("Producer");
    frame.Field(ID3FN_TEXT).Add("Martin Sauer");
    frame.Field(ID3FN_TEXT).Add("Piano");
    frame.Field(ID3FN_TEXT).Add("Peter Toperczer");
    tag.AddFrame(frame);

    frame.SetID(ID3FID_PICTURE);
    frame.Field(ID3FN_MIMETYPE).Set("image/jpeg");
    frame.Field(ID3FN_PICTURETYPE).Set(11);
    frame.Field(ID3FN_DESCRIPTION).Set("B/W picture of Saint-Saëns");
    frame.Field(ID3FN_DATA).FromFile("composer.jpg");
    tag.AddFrame(frame);

    tag.SetPadding(false);
    tag.SetUnsync(true);
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
