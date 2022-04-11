// $Id: tag_parse_musicmatch.cpp,v 1.3 2000/07/04 22:24:56 eldamitri Exp $

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
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "tag.h"
#include "utils.h"
#include "misc_support.h"

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

size_t MM_ParseNum(const char* arr, size_t num_bytes)
{
  size_t val = 0;
  for (size_t i = 0; i < num_bytes; i++)
  {
    val += (static_cast<size_t>(0xFF & arr[i]) << (i * 8));
  }
  return val;
}

ID3_Frame* MM_ParseTextField(FILE* handle, ID3_FrameID id, const char* desc = "")
{
  char size_bytes[2];
  size_t size = 0;
  fread(size_bytes, sizeof(char), 2, handle);
  size = MM_ParseNum(size_bytes, 2);
  ID3_Frame* frame = NULL;
  if (size > 0)
  {
    char* text = new char[size + 1];
    text[size] = '\0';
    fread(text, sizeof(char), size, handle);
    if (ID3FID_SONGLEN == id)
    {
      size_t len = ID3_TimeToSeconds(text, size) * 1000;
      size_t digits = 1;
      for (size_t max = 10; max < len; digits++, max *= 10)
      {
        continue;
      }
      char* len_str = new char[digits + 1];
      sprintf(len_str, "%lu", len);
      delete [] text;
      text = len_str;
    }
    else
    {
      size = ID3_CRLFtoLF(text, size);
      text[size] = '\0';
    }
    frame = new ID3_Frame(id);
    if (frame)
    {
      if (frame->Contains(ID3FN_TEXT))
      {
        frame->Field(ID3FN_TEXT).Set(text);
      }
      else if (frame->Contains(ID3FN_URL))
      {
        frame->Field(ID3FN_URL).Set(text);
      }
      if (frame->Contains(ID3FN_LANGUAGE))
      {
        frame->Field(ID3FN_LANGUAGE).Set("XXX");
      }
      if (frame->Contains(ID3FN_DESCRIPTION))
      {
        frame->Field(ID3FN_DESCRIPTION).Set(desc);
      }
    }
    delete [] text;
  }
  return frame;
}

size_t ParseMusicMatch(ID3_Tag& tag, FILE* handle)
{
  size_t tag_bytes = 0;
  if (NULL == handle)
  {
    // TODO: log this
    return tag_bytes;
    // ID3_THROW(ID3E_NoData);
  }
  
  char id_buffer[48];
  
  fseek(handle, -48, SEEK_CUR);
  fread(id_buffer, 1, 48, handle);

  // first check for an ID3v1 tag
  if (memcmp(id_buffer, "Brava Software Inc.             ", 32) == 0)
  {
    char offset_buffer[20];
    char img_ext[5];
    img_ext[4] = '\0';
    
    long tag_end = ftell(handle);
    long tag_beg = tag_end;

    fseek(handle, -68, SEEK_CUR);
    fread(offset_buffer, 1, 20, handle);

    char buff[5];
    buff[4] = 0;
    
          
    for (int i = 0; i < 5; i++)
    {
      size_t offset = MM_ParseNum(&offset_buffer[i*4], 4);

      fseek(handle, offset - 1 + tag.GetPrependedBytes(), SEEK_SET);

      if (0 == i)
      {
        tag_beg = ftell(handle);
        fseek(handle, -256, SEEK_CUR);
        char sync[8];
        fread(sync, sizeof(char), 8, handle);
        if (memcmp(sync, "18273645", 8) == 0)
        {
          fseek(handle, -8, SEEK_CUR);
          tag_beg = ftell(handle);
          fseek(handle, offset - 1 + tag.GetPrependedBytes(), SEEK_SET);
        }
      }
      
      fread(buff, 1, 4, handle);
      if (memcmp(buff, "\0\0\0\0", 4) == 0)
      {
        continue;
      }
      switch (i)
      {
        case 0:
        {
          RemoveTrailingSpaces(buff, 4);
          memcpy(img_ext, buff, 4);

          break;
        }
        case 1:
        {
          size_t img_size = MM_ParseNum(buff, 4);
          uchar* img_data = new uchar[img_size];
          fread(img_data, sizeof(uchar), img_size, handle);
          ID3_Frame* frame = new ID3_Frame(ID3FID_PICTURE);
          if (frame)
          {
            char mimetype[11];
            strcpy(mimetype, "image/");
            strcat(mimetype, img_ext);
            frame->Field(ID3FN_MIMETYPE).Set(mimetype);
            frame->Field(ID3FN_IMAGEFORMAT).Set("");
            frame->Field(ID3FN_PICTURETYPE).Set(static_cast<size_t>(0));
            frame->Field(ID3FN_DESCRIPTION).Set("");
            frame->Field(ID3FN_DATA).Set(img_data, img_size);
            tag.AttachFrame(frame);
          }
          break;
        }
        case 2:
        case 3:
        {
          // don't know what to do with these...
          break;
        }
        case 4:
        {
          ID3_Frame* frame;
          fseek(handle, offset - 1 + tag.GetPrependedBytes(), SEEK_SET);
          
          tag.AttachFrame(MM_ParseTextField(handle, ID3FID_TITLE));
          tag.AttachFrame(MM_ParseTextField(handle, ID3FID_ALBUM));
          tag.AttachFrame(MM_ParseTextField(handle, ID3FID_LEADARTIST));
          tag.AttachFrame(MM_ParseTextField(handle, ID3FID_CONTENTTYPE));
          tag.AttachFrame(MM_ParseTextField(handle, ID3FID_COMMENT, 
                                              "MusicMatch_Tempo"));
          tag.AttachFrame(MM_ParseTextField(handle, ID3FID_COMMENT, 
                                              "MusicMatch_Mood"));
          tag.AttachFrame(MM_ParseTextField(handle, ID3FID_COMMENT, 
                                              "MusicMatch_Situation"));
          tag.AttachFrame(MM_ParseTextField(handle, ID3FID_COMMENT, 
                                              "MusicMatch_Preference"));
          tag.AttachFrame(MM_ParseTextField(handle, ID3FID_SONGLEN));

          // 12 bytes?
          fseek(handle, 12, SEEK_CUR);

          tag.AttachFrame(MM_ParseTextField(handle, ID3FID_COMMENT, 
                                              "MusicMatch_Path"));
          tag.AttachFrame(MM_ParseTextField(handle, ID3FID_COMMENT, 
                                              "MusicMatch_Serial"));

          // 2 bytes for track
          char trk_bytes[2];
          fread(trk_bytes, sizeof(char), 2, handle);
          size_t trk_num = MM_ParseNum(trk_bytes, 2);
          if (trk_num > 0)
          {
            char trk_str[5];
            sprintf(trk_str, "%lu", trk_num);
            frame = new ID3_Frame(ID3FID_TRACKNUM);
            if (frame)
            {
              frame->Field(ID3FN_TEXT).Set(trk_str);
              tag.AttachFrame(frame);
            }
          }

          tag.AttachFrame(MM_ParseTextField(handle, ID3FID_COMMENT, 
                                              "MusicMatch_Notes"));
          tag.AttachFrame(MM_ParseTextField(handle, ID3FID_COMMENT, 
                                              "MusicMatch_Bio"));
          tag.AttachFrame(MM_ParseTextField(handle, ID3FID_UNSYNCEDLYRICS));
          tag.AttachFrame(MM_ParseTextField(handle, ID3FID_WWWARTIST));
          tag.AttachFrame(MM_ParseTextField(handle, ID3FID_WWWCOMMERCIALINFO));

          // email?
          break;
        }
      }
    }

    tag_bytes += (tag_end - tag_beg);
  }

  return tag_bytes;
}
