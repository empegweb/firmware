// $Id: misc_support.cpp,v 1.18 2000/07/07 23:13:10 eldamitri Exp $

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

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "misc_support.h"

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

char *ID3_GetString(const ID3_Frame *frame, ID3_FieldID fldName, size_t nIndex)
{
  char *text = NULL;
  if (NULL != frame)
  {
    size_t nText = frame->Field(fldName).Size();
    text = new char[nText + 1];
    try 
    {
      frame->Field(fldName).Get(text, nText, nIndex);
    }
    catch (ID3_Error &)
    {
      delete [] text;
      return NULL;
    }
    text[nText] = '\0';
  }
  return text;
}

char *ID3_GetArtist(const ID3_Tag *tag)
{
  char *sArtist = NULL;
  if (NULL == tag)
  {
    return sArtist;
  }

  ID3_Frame *frame = NULL;
  if ((frame = tag->Find(ID3FID_LEADARTIST)) ||
      (frame = tag->Find(ID3FID_BAND))       ||
      (frame = tag->Find(ID3FID_CONDUCTOR))  ||
      (frame = tag->Find(ID3FID_COMPOSER)))
  {
    sArtist = ID3_GetString(frame, ID3FN_TEXT);
  }
  return sArtist;
}

ID3_Frame* ID3_AddArtist(ID3_Tag *tag, const char *text, bool replace)
{
  ID3_Frame* frame = NULL;
  if (NULL != tag && NULL != text && strlen(text) > 0)
  {
    if (replace)
    {
      ID3_RemoveArtists(tag);
    }
    if (replace ||
        (tag->Find(ID3FID_LEADARTIST) == NULL &&
         tag->Find(ID3FID_BAND)       == NULL &&
         tag->Find(ID3FID_CONDUCTOR)  == NULL &&
         tag->Find(ID3FID_COMPOSER)   == NULL))
    {
      frame = new ID3_Frame(ID3FID_LEADARTIST);
      if (frame)
      {
        frame->Field(ID3FN_TEXT) = text;
        tag->AttachFrame(frame);
      }
    }
  }

  return frame;
}

size_t ID3_RemoveArtists(ID3_Tag *tag)
{
  size_t num_removed = 0;
  ID3_Frame *frame = NULL;

  if (NULL == tag)
  {
    return num_removed;
  }

  while ((frame = tag->Find(ID3FID_LEADARTIST)))
  {
    frame = tag->RemoveFrame(frame);
    delete frame;
    num_removed++;
  }
  while ((frame = tag->Find(ID3FID_BAND)))
  {
    frame = tag->RemoveFrame(frame);
    delete frame;
    num_removed++;
  }
  while ((frame = tag->Find(ID3FID_CONDUCTOR)))
  {
    frame = tag->RemoveFrame(frame);
    delete frame;
    num_removed++;
  }
  while ((frame = tag->Find(ID3FID_COMPOSER)))
  {
    frame = tag->RemoveFrame(frame);
    delete frame;
    num_removed++;
  }

  return num_removed;
}

char *ID3_GetAlbum(const ID3_Tag *tag)
{
  char *sAlbum = NULL;
  if (NULL == tag)
  {
    return sAlbum;
  }

  ID3_Frame *frame = tag->Find(ID3FID_ALBUM);
  if (frame != NULL)
  {
    sAlbum = ID3_GetString(frame, ID3FN_TEXT);
  }
  return sAlbum;
}

ID3_Frame* ID3_AddAlbum(ID3_Tag *tag, const char *text, bool replace)
{
  ID3_Frame* frame = NULL;
  if (NULL != tag && NULL != text && strlen(text) > 0)
  {
    if (replace)
    {
      ID3_RemoveAlbums(tag);
    }
    if (replace || tag->Find(ID3FID_ALBUM) == NULL)
    {
      frame = new ID3_Frame(ID3FID_ALBUM);
      if (frame)
      {
        frame->Field(ID3FN_TEXT) = text;
        tag->AttachFrame(frame);
      }
    }
  }
  
  return frame;
}

size_t ID3_RemoveAlbums(ID3_Tag *tag)
{
  size_t num_removed = 0;
  ID3_Frame *frame = NULL;

  if (NULL == tag)
  {
    return num_removed;
  }

  while ((frame = tag->Find(ID3FID_ALBUM)))
  {
    frame = tag->RemoveFrame(frame);
    delete frame;
    num_removed++;
  }

  return num_removed;
}

char *ID3_GetTitle(const ID3_Tag *tag)
{
  char *sTitle = NULL;
  if (NULL == tag)
  {
    return sTitle;
  }

  ID3_Frame *frame = tag->Find(ID3FID_TITLE);
  if (frame != NULL)
  {
    sTitle = ID3_GetString(frame, ID3FN_TEXT);
  }
  return sTitle;
}

ID3_Frame* ID3_AddTitle(ID3_Tag *tag, const char *text, bool replace)
{
  ID3_Frame* frame = NULL;
  if (NULL != tag && NULL != text && strlen(text) > 0)
  {
    if (replace)
    {
      ID3_RemoveTitles(tag);
    }
    if (replace || tag->Find(ID3FID_TITLE) == NULL)
    {
      frame = new ID3_Frame(ID3FID_TITLE);
      if (frame)
      {
        frame->Field(ID3FN_TEXT) = text;
        tag->AttachFrame(frame);
      }
    }
  }
  
  return frame;
}

size_t ID3_RemoveTitles(ID3_Tag *tag)
{
  size_t num_removed = 0;
  ID3_Frame *frame = NULL;

  if (NULL == tag)
  {
    return num_removed;
  }

  while ((frame = tag->Find(ID3FID_TITLE)))
  {
    frame = tag->RemoveFrame(frame);
    delete frame;
    num_removed++;
  }

  return num_removed;
}

char *ID3_GetYear(const ID3_Tag *tag)
{
  char *sYear = NULL;
  if (NULL == tag)
  {
    return sYear;
  }

  ID3_Frame *frame = tag->Find(ID3FID_YEAR);
  if (frame != NULL)
  {
    sYear = ID3_GetString(frame, ID3FN_TEXT);
  }
  return sYear;
}

ID3_Frame* ID3_AddYear(ID3_Tag *tag, const char *text, bool replace)
{
  ID3_Frame* frame = NULL;
  if (NULL != tag && NULL != text && strlen(text) > 0)
  {
    if (replace)
    {
      ID3_RemoveYears(tag);
    }
    if (replace || tag->Find(ID3FID_YEAR) == NULL)
    {
      frame = new ID3_Frame(ID3FID_YEAR);
      if (NULL != frame)
      {
        frame->Field(ID3FN_TEXT) = text;
        tag->AttachFrame(frame);
      }
    }
  }
  
  return frame;
}

size_t ID3_RemoveYears(ID3_Tag *tag)
{
  size_t num_removed = 0;
  ID3_Frame *frame = NULL;

  if (NULL == tag)
  {
    return num_removed;
  }

  while ((frame = tag->Find(ID3FID_YEAR)))
  {
    frame = tag->RemoveFrame(frame);
    delete frame;
    num_removed++;
  }

  return num_removed;
}

char *ID3_GetComment(const ID3_Tag *tag, const char* desc)
{
  char *comment = NULL;
  if (NULL == tag)
  {
    return comment;
  }

  ID3_Frame* frame = NULL;
  if (desc)
  {
    frame = tag->Find(ID3FID_COMMENT, ID3FN_DESCRIPTION, desc);
  }
  else
  {
    frame = tag->Find(ID3FID_COMMENT);
  }

  if (frame)
  {
    comment = ID3_GetString(frame, ID3FN_TEXT);
  }
  return comment;
}

ID3_Frame* ID3_AddComment(ID3_Tag *tag, const char *text, bool replace)
{
  return ID3_AddComment(tag, text, "", replace);
}

ID3_Frame* ID3_AddComment(ID3_Tag *tag, const char *text,
                          const char *desc, bool replace)
{
  return ID3_AddComment(tag, text, desc, "XXX", replace);
}

ID3_Frame* ID3_AddComment(ID3_Tag *tag, const char *text,
                          const char *desc, const char* lang, bool replace)
{
  ID3_Frame* frame = NULL;
  if (NULL != tag  &&
      NULL != text &&
      NULL != desc && 
      strlen(text) > 0)
  {
    bool bAdd = true;
    if (replace)
    {
      ID3_RemoveComments(tag, desc);
    }
    else
    {
      // See if there is already a comment with this description
      for (size_t nCount = 0; nCount < tag->NumFrames(); nCount++)
      {
        frame = tag->GetFrameNum(nCount);
        if (frame->GetID() == ID3FID_COMMENT)
        {
          char *tmp_desc = ID3_GetString(frame, ID3FN_DESCRIPTION);
          if (strcmp(tmp_desc, desc) == 0)
          {
            bAdd = false;
          }
          delete [] tmp_desc;
          if (!bAdd)
          {
            break;
          }
        }
      }
    }
    if (bAdd)
    {
      frame = new ID3_Frame(ID3FID_COMMENT);
      if (NULL != frame)
      {
        frame->Field(ID3FN_LANGUAGE) = lang;
        frame->Field(ID3FN_DESCRIPTION) = desc;
        frame->Field(ID3FN_TEXT) = text;
        tag->AttachFrame(frame);
      }
    }
  }
  return frame;
}

// Remove all comments with the given description (remove all comments if
// desc is NULL)
size_t ID3_RemoveComments(ID3_Tag *tag, const char *desc)
{
  size_t num_removed = 0;

  if (NULL == tag)
  {
    return num_removed;
  }

  for (size_t nCount = 0; nCount < tag->NumFrames(); nCount++)
  {
    ID3_Frame *frame = tag->GetFrameNum(nCount);
    if (frame->GetID() == ID3FID_COMMENT)
    {
      bool remove = false;
      // A null description means remove all comments
      if (NULL == desc)
      {
        remove = true;
      }
      else
      {
        // See if the description we have matches the description of the 
        // current comment.  If so, set the "remove the comment" flag to true.
        char *tmp_desc = ID3_GetString(frame, ID3FN_DESCRIPTION);
        remove = (strcmp(tmp_desc, desc) == 0);
        delete [] tmp_desc;
      }
      if (remove)
      {
        frame = tag->RemoveFrame(frame);
        delete frame;
        num_removed++;
      }
    }
  }

  return num_removed;
}

char *ID3_GetTrack(const ID3_Tag *tag)
{
  char *sTrack = NULL;
  if (NULL == tag)
  {
    return sTrack;
  }

  ID3_Frame *frame = tag->Find(ID3FID_TRACKNUM);
  if (frame != NULL)
  {
    sTrack = ID3_GetString(frame, ID3FN_TEXT);
  }
  return sTrack;
}

size_t ID3_GetTrackNum(const ID3_Tag *tag)
{
  char *sTrack = ID3_GetTrack(tag);
  size_t nTrack = 0;
  if (NULL != sTrack)
  {
    nTrack = atoi(sTrack);
    delete [] sTrack;
  }
  return nTrack;
}

ID3_Frame* ID3_AddTrack(ID3_Tag *tag, uchar trk, uchar ttl, bool replace)
{
  ID3_Frame* frame = NULL;
  if (NULL != tag && trk > 0)
  {
    if (replace)
    {
      ID3_RemoveTracks(tag);
    }
    if (replace || NULL == tag->Find(ID3FID_TRACKNUM))
    {
      frame = new ID3_Frame(ID3FID_TRACKNUM);
      if (frame)
      {
        char *sTrack = NULL;
        if (0 == ttl)
        {
          sTrack = new char[4];
          sprintf(sTrack, "%lu", (luint) trk);
        }
        else
        {
          sTrack = new char[8];
          sprintf(sTrack, "%lu/%lu", (luint) trk, (luint) ttl);
        }
        
        frame->Field(ID3FN_TEXT) = sTrack;
        tag->AttachFrame(frame);

        delete [] sTrack;
      }
    }
  }
  
  return frame;
}

size_t ID3_RemoveTracks(ID3_Tag *tag)
{
  size_t num_removed = 0;
  ID3_Frame *frame = NULL;

  if (NULL == tag)
  {
    return num_removed;
  }

  while ((frame = tag->Find(ID3FID_TRACKNUM)))
  {
    frame = tag->RemoveFrame(frame);
    delete frame;
    num_removed++;
  }

  return num_removed;
}

char *ID3_GetGenre(const ID3_Tag *tag)
{
  char *sGenre = NULL;
  if (NULL == tag)
  {
    return sGenre;
  }

  ID3_Frame *frame = tag->Find(ID3FID_CONTENTTYPE);
  if (frame != NULL)
  {
    sGenre = ID3_GetString(frame, ID3FN_TEXT);
  }

  return sGenre;
}

size_t ID3_GetGenreNum(const ID3_Tag *tag)
{
  char *sGenre = ID3_GetGenre(tag);
  size_t ulGenre = 0xFF;
  if (NULL == sGenre)
  {
    return ulGenre;
  }

  // If the genre string begins with "(ddd)", where "ddd" is a number, then 
  // "ddd" is the genre number---get it
  if (sGenre[0] == '(')
  {
    char *pCur = &sGenre[1];
    while (isdigit(*pCur))
    {
      pCur++;
    }
    if (*pCur == ')')
    {
      // if the genre number is greater than 255, its invalid.
      ulGenre = MIN(0xFF, atoi(&sGenre[1]));
    }
  }

  delete [] sGenre;
  return ulGenre;
}

ID3_Frame* ID3_AddGenre(ID3_Tag *tag, size_t genre, bool replace)
{
  ID3_Frame* frame = NULL;
  if (NULL != tag && 0xFF != genre)
  {
    if (replace)
    {
      ID3_RemoveGenres(tag);
    }
    if (replace || NULL == tag->Find(ID3FID_CONTENTTYPE))
    {
      frame = new ID3_Frame(ID3FID_CONTENTTYPE);
      if (NULL != frame)
      {
        char sGenre[6];
        sprintf(sGenre, "(%lu)", (luint) genre);

        frame->Field(ID3FN_TEXT) = sGenre;
        tag->AttachFrame(frame);
      }
    }
  }
  
  return frame;
}

size_t ID3_RemoveGenres(ID3_Tag *tag)
{
  size_t num_removed = 0;
  ID3_Frame *frame = NULL;

  if (NULL == tag)
  {
    return num_removed;
  }

  while ((frame = tag->Find(ID3FID_CONTENTTYPE)))
  {
    frame = tag->RemoveFrame(frame);
    delete frame;
    num_removed++;
  }

  return num_removed;
}

char *ID3_GetLyrics(const ID3_Tag *tag)
{
  char *sLyrics = NULL;
  if (NULL == tag)
  {
    return sLyrics;
  }

  ID3_Frame *frame = tag->Find(ID3FID_UNSYNCEDLYRICS);
  if (frame != NULL)
  {
    sLyrics = ID3_GetString(frame, ID3FN_TEXT);
  }
  return sLyrics;
}

ID3_Frame* ID3_AddLyrics(ID3_Tag *tag, const char *text, bool replace)
{
  return ID3_AddLyrics(tag, text, "", replace);
}

ID3_Frame* ID3_AddLyrics(ID3_Tag *tag, const char *text, const char* desc, 
                         bool replace)
{
  return ID3_AddLyrics(tag, text, desc, "XXX", replace);
}

ID3_Frame* ID3_AddLyrics(ID3_Tag *tag, const char *text, const char* desc,
                         const char* lang, bool replace)
{
  ID3_Frame* frame = NULL;
  if (NULL != tag && strlen(text) > 0)
  {
    if (replace)
    {
      ID3_RemoveLyrics(tag);
    }
    if (replace || tag->Find(ID3FID_UNSYNCEDLYRICS) == NULL)
    {
      frame = new ID3_Frame(ID3FID_UNSYNCEDLYRICS);
      if (NULL != frame)
      {
        frame->Field(ID3FN_LANGUAGE) = lang;
        frame->Field(ID3FN_DESCRIPTION) = desc;
        frame->Field(ID3FN_TEXT) = text;
        tag->AttachFrame(frame);
      }
    }
  }
  
  return frame;
}

size_t ID3_RemoveLyrics(ID3_Tag *tag)
{
  size_t num_removed = 0;
  ID3_Frame *frame = NULL;

  if (NULL == tag)
  {
    return num_removed;
  }

  while ((frame = tag->Find(ID3FID_UNSYNCEDLYRICS)))
  {
    frame = tag->RemoveFrame(frame);
    delete frame;
    num_removed++;
  }

  return num_removed;
}

char *ID3_GetLyricist(const ID3_Tag *tag)
{
  char *sLyricist = NULL;
  if (NULL == tag)
  {
    return sLyricist;
  }

  ID3_Frame *frame = tag->Find(ID3FID_LYRICIST);
  if (frame != NULL)
  {
    sLyricist = ID3_GetString(frame, ID3FN_TEXT);
  }
  return sLyricist;
}

ID3_Frame* ID3_AddLyricist(ID3_Tag *tag, const char *text, bool replace)
{
  ID3_Frame* frame = NULL;
  if (NULL != tag && NULL != text && strlen(text) > 0)
  {
    if (replace)
    {
      ID3_RemoveLyricist(tag);
    }
    if (replace || (tag->Find(ID3FID_LYRICIST) == NULL))
    {    
      frame = new ID3_Frame(ID3FID_LYRICIST);
      if (frame)
      {
        frame->Field(ID3FN_TEXT) = text;
        tag->AttachFrame(frame);
      }
    }
  }

  return frame;
}

size_t ID3_RemoveLyricist(ID3_Tag *tag)
{
  size_t num_removed = 0;
  ID3_Frame *frame = NULL;

  if (NULL == tag)
  {
    return num_removed;
  }

  while ((frame = tag->Find(ID3FID_LYRICIST)))
  {
    frame = tag->RemoveFrame(frame);
    delete frame;
    num_removed++;
  }

  return num_removed;
}

ID3_Frame* ID3_AddSyncLyrics(ID3_Tag *tag, const uchar *data, size_t datasize,
                             ID3_TimeStampFormat format, bool replace)
{
  return ID3_AddSyncLyrics(tag, data, datasize, format, "", replace);
}

ID3_Frame* ID3_AddSyncLyrics(ID3_Tag *tag, const uchar *data, size_t datasize,
                             ID3_TimeStampFormat format, const char *desc, 
                             bool replace)
{
  return ID3_AddSyncLyrics(tag, data, datasize, format, desc, "XXX", replace);
}

ID3_Frame* ID3_AddSyncLyrics(ID3_Tag *tag, const uchar *data, size_t datasize,
                             ID3_TimeStampFormat format, const char *desc, 
                             const char *lang, bool replace)
{
  return ID3_AddSyncLyrics(tag, data, datasize, format, desc, lang, 
                           ID3CT_LYRICS, replace);
}

ID3_Frame* ID3_AddSyncLyrics(ID3_Tag *tag, const uchar *data, size_t datasize,
                             ID3_TimeStampFormat format, const char *desc, 
                             const char *lang, ID3_ContentType type, 
                             bool replace)
{
  ID3_Frame* frame = NULL;
  // language and descriptor should be mandatory
  if ((NULL == lang) || (NULL == desc))
  {
    return NULL;
  }

  // check if a SYLT frame of this language or descriptor already exists
  ID3_Frame* frm_exist = tag->Find(ID3FID_SYNCEDLYRICS, ID3FN_LANGUAGE, lang);
  if (!frm_exist)
  {
    frm_exist = tag->Find(ID3FID_SYNCEDLYRICS, ID3FN_DESCRIPTION, desc);
  }

  if (NULL != tag && NULL != data)
  {
    if (replace && frm_exist)
    {
      frm_exist = tag->RemoveFrame (frm_exist);
      delete frm_exist;
      frm_exist = NULL;
    }

    // if the frame still exist, cannot continue
    if (frm_exist)
    {
      return NULL;
    }

    ID3_Frame* frame = new ID3_Frame(ID3FID_SYNCEDLYRICS);
    if (NULL == frame)
    {
      ID3_THROW(ID3E_NoMemory);
    }

    frame->Field(ID3FN_LANGUAGE) = lang;
    frame->Field(ID3FN_DESCRIPTION) = desc;
    frame->Field(ID3FN_TIMESTAMPFORMAT) = format;
    frame->Field(ID3FN_CONTENTTYPE) = type;
    frame->Field(ID3FN_DATA).Set(data, datasize);
    tag->AttachFrame(frame);
  }

  return frame;
}

ID3_Frame *ID3_GetSyncLyricsInfo(const ID3_Tag *tag, const char *desc, 
                                 const char *lang,
                                 ID3_TimeStampFormat& format, 
                                 ID3_ContentType& type, size_t& size)
{
  // check if a SYLT frame of this language or descriptor exists
  ID3_Frame* frm_exist = NULL;
  if (NULL != lang)
  {
    // search through language
    frm_exist = tag->Find(ID3FID_SYNCEDLYRICS, ID3FN_LANGUAGE, lang);
  }
  else if (NULL != desc)
  {
    // search through descriptor
    frm_exist = tag->Find(ID3FID_SYNCEDLYRICS, ID3FN_DESCRIPTION, desc);
  }
  else
  {
    // both language and description not specified, search the first SYLT frame
    frm_exist = tag->Find(ID3FID_SYNCEDLYRICS);
  }
  
  if (!frm_exist)
  {
    return NULL;
  }
  
  // get the lyrics time stamp format
  format = static_cast<ID3_TimeStampFormat>(frm_exist->Field (ID3FN_TIMESTAMPFORMAT).Get ());
  
  // get the lyrics content type
  type = static_cast<ID3_ContentType>(frm_exist->Field (ID3FN_CONTENTTYPE).Get ());
  
  // get the lyrics size
  size = frm_exist->Field (ID3FN_DATA).Size ();
  
  // return the frame pointer for further uses
  return frm_exist;
}

ID3_Frame *ID3_GetSyncLyrics(const ID3_Tag *tag, const char *lang, 
                             const char *desc, const uchar *pData, size_t& size)
{
  // check if a SYLT frame of this language or descriptor exists
  ID3_Frame* frm_exist = NULL;
  if (NULL != lang)
  {
    // search through language
    frm_exist = tag->Find(ID3FID_SYNCEDLYRICS, ID3FN_LANGUAGE, lang);
  }
  else if (NULL != desc)
  {
    // search through descriptor
    frm_exist = tag->Find(ID3FID_SYNCEDLYRICS, ID3FN_DESCRIPTION, desc);
  }
  else
  {
    // both language and description not specified, search the first SYLT frame
    frm_exist = tag->Find(ID3FID_SYNCEDLYRICS);
  }

  if (NULL == frm_exist)
  {
    return NULL;
  }
  
  // get the lyrics size
  size_t datasize = frm_exist->Field(ID3FN_DATA).Size();
  size = MIN(size, datasize);

  // get the lyrics data
  pData = frm_exist->Field (ID3FN_DATA).GetBinary();

  // return the frame pointer for further uses
  return frm_exist;
}

