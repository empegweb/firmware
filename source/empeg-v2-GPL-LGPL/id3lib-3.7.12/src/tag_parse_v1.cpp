// $Id: tag_parse_v1.cpp,v 1.10 2000/07/04 22:32:22 eldamitri Exp $

// id3lib: a C++ library for creating and manipulating id3v1/v2 tags
// Copyright 1999, 2000  Scott Thomas Haug

// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2 of the License, or (at your
// option) any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
// License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// The id3lib authors encourage improvements and optimisations to be sent to
// the id3lib coordinator.  Please see the README file for details on where to
// send such submissions.  See the AUTHORS file for a list of people who have
// contributed to id3lib.  See the ChangeLog file for a list of changes to
// id3lib.  These files are distributed with id3lib at
// http://download.sourceforge.net/id3lib/

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include "tag.h"
#include "misc_support.h"
#include "utils.h"

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

size_t ParseID3v1(ID3_Tag& tag, FILE* handle)
{
  size_t tag_bytes = 0;
  if (NULL == handle)
  {
    return tag_bytes;
  }

  ID3V1_Tag tagID3v1;
    
  // posn ourselves at 128 bytes from the current position
  if (fseek(handle, 0-ID3_V1_LEN, SEEK_CUR) != 0)
  {
    return tag_bytes;
    // TODO:  This is a bad error message.  Make it more descriptive
  }
    
    
  // read the next 128 bytes in;
  if (fread(tagID3v1.sID, 1, ID3_V1_LEN_ID, handle) != ID3_V1_LEN_ID)
  {
    // TODO:  This is a bad error message.  Make it more descriptive
    return tag_bytes;
    //ID3_THROW(ID3E_NoData);
  }
    
  // check to see if it was a tag
  if (memcmp(tagID3v1.sID, "TAG", ID3_V1_LEN_ID) == 0)
  {
    // guess so, let's start checking the v2 tag for frames which are the
    // equivalent of the v1 fields.  When we come across a v1 field that has
    // no current equivalent v2 frame, we create the frame, copy the data
    // from the v1 frame and attach it to the tag
      
    // the TITLE field/frame
    if (fread(tagID3v1.sTitle, 1, ID3_V1_LEN_TITLE, handle) != ID3_V1_LEN_TITLE)
    {
      // TODO:  This is a bad error message.  Make it more descriptive
      return tag_bytes;
      //ID3_THROW(ID3E_NoData);
    }
    tagID3v1.sTitle[ID3_V1_LEN_TITLE] = '\0';
    RemoveTrailingSpaces(tagID3v1.sTitle,  ID3_V1_LEN_TITLE);
    ID3_AddTitle(&tag, tagID3v1.sTitle);
    
    // the ARTIST field/frame
    if (fread(tagID3v1.sArtist, 1, ID3_V1_LEN_ARTIST, handle) != 
        ID3_V1_LEN_ARTIST)
    {
      // TODO:  This is a bad error message.  Make it more descriptive
      return tag_bytes;
      //ID3_THROW(ID3E_NoData);
    }
    tagID3v1.sArtist[ID3_V1_LEN_ARTIST] = '\0';
    RemoveTrailingSpaces(tagID3v1.sArtist, ID3_V1_LEN_ARTIST);
    ID3_AddArtist(&tag, tagID3v1.sArtist);
  
    // the ALBUM field/frame
    if (fread(tagID3v1.sAlbum, 1, ID3_V1_LEN_ALBUM, handle) != ID3_V1_LEN_ALBUM)
    {
      // TODO:  This is a bad error message.  Make it more descriptive
      return tag_bytes;
      //ID3_THROW(ID3E_NoData);
    }
    tagID3v1.sAlbum[ID3_V1_LEN_ALBUM] = '\0';
    RemoveTrailingSpaces(tagID3v1.sAlbum,  ID3_V1_LEN_ALBUM);
    ID3_AddAlbum(&tag, tagID3v1.sAlbum);
  
    // the YEAR field/frame
    if (fread(tagID3v1.sYear, 1, ID3_V1_LEN_YEAR, handle) != ID3_V1_LEN_YEAR)
    {
      // TODO:  This is a bad error message.  Make it more descriptive
      return tag_bytes;
      //ID3_THROW(ID3E_NoData);
    }
    tagID3v1.sYear[ID3_V1_LEN_YEAR] = '\0';
    RemoveTrailingSpaces(tagID3v1.sYear,   ID3_V1_LEN_YEAR);
    ID3_AddYear(&tag, tagID3v1.sYear);
  
    // the COMMENT field/frame
    if (fread(tagID3v1.sComment, 1, ID3_V1_LEN_COMMENT, handle) !=
        ID3_V1_LEN_COMMENT)
    {
      // TODO:  This is a bad error message.  Make it more descriptive
      return tag_bytes;
      //ID3_THROW(ID3E_NoData);
    }
    tagID3v1.sComment[ID3_V1_LEN_COMMENT] = '\0';
    if ('\0' != tagID3v1.sComment[ID3_V1_LEN_COMMENT - 2] ||
        '\0' == tagID3v1.sComment[ID3_V1_LEN_COMMENT - 1])
    {
      RemoveTrailingSpaces(tagID3v1.sComment, ID3_V1_LEN_COMMENT);
    }
    else
    {
      // This is an id3v1.1 tag.  The last byte of the comment is the track
      // number.  
      RemoveTrailingSpaces(tagID3v1.sComment, ID3_V1_LEN_COMMENT - 1);
      ID3_AddTrack(&tag, tagID3v1.sComment[ID3_V1_LEN_COMMENT - 1]);
    }
    ID3_AddComment(&tag, tagID3v1.sComment, STR_V1_COMMENT_DESC);
      
    // the GENRE field/frame
    fread(&tagID3v1.ucGenre, 1, ID3_V1_LEN_GENRE, handle);
    ID3_AddGenre(&tag, tagID3v1.ucGenre);

    tag_bytes += ID3_V1_LEN;
  }
    
  return tag_bytes;
}
