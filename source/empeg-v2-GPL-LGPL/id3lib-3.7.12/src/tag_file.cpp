// $Id: tag_file.cpp,v 1.20 2000/07/07 23:14:13 eldamitri Exp $

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
#include <stdio.h>
#include <fstream.h>

#ifdef  MAXPATHLEN
#  define ID3_PATH_LENGTH   (MAXPATHLEN + 1)
#elif   defined (PATH_MAX)
#  define ID3_PATH_LENGTH   (PATH_MAX + 1)
#else   /* !MAXPATHLEN */
#  define ID3_PATH_LENGTH   (2048 + 1)
#endif  /* !MAXPATHLEN && !PATH_MAX */

#if defined WIN32
#  include <windows.h>
static int truncate(const char *path, size_t length)
{
  int result = -1;
  HANDLE fh;
  
  fh = ::CreateFile(path,
                    GENERIC_WRITE | GENERIC_READ,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
  
  if(INVALID_HANDLE_VALUE != fh)
  {
    SetFilePointer(fh, length, NULL, FILE_BEGIN);
    SetEndOfFile(fh);
    CloseHandle(fh);
    result = 0;
  }
  
  return result;
}

// prevents a weird error I was getting compiling this under windows
#  if defined CreateFile
#    undef CreateFile
#  endif

#else
#  include <unistd.h>
#endif

#if defined HAVE_CONFIG_H
#  include <config.h>
#endif

#include "tag.h"

bool exists(const char *name)
{
  bool doesExist = false;
  FILE *in = NULL;
  
  if (NULL == name)
  {
    return false;
  }

  in = fopen(name, "rb");
  doesExist = (NULL != in);
  if (doesExist)
  {
    fclose(in);
  }
    
  return doesExist;
}



ID3_Err ID3_Tag::CreateFile()
{
  CloseFile();

  // Create a new file
  __file_handle = fopen(__file_name, "wb+");

  // Check to see if file could not be created
  if (NULL == __file_handle)
  {
    return ID3E_ReadOnly;
  }

  // Determine the size of the file
  fseek(__file_handle, 0, SEEK_END);
  __file_size = ftell(__file_handle);
  fseek(__file_handle, 0, SEEK_SET);
  
  return ID3E_NoError;
}

ID3_Err ID3_Tag::OpenFileForWriting()
{
  CloseFile();
  __file_size = 0;
  if (exists(__file_name))
  {
    // Try to open the file for reading and writing.
    __file_handle = fopen(__file_name, "rb+");
  }
  else
  {
    return ID3E_NoFile;
  }

  // Check to see if file could not be opened for writing
  if (NULL == __file_handle)
  {
    return ID3E_ReadOnly;
  }

  // Determine the size of the file
  fseek(__file_handle, 0, SEEK_END);
  __file_size = ftell(__file_handle);
  fseek(__file_handle, 0, SEEK_SET);
  
  return ID3E_NoError;
}

ID3_Err ID3_Tag::OpenFileForReading()
{
  CloseFile();
  __file_size = 0;

  __file_handle = fopen(__file_name, "rb");
  
  if (NULL == __file_handle)
  {
    return ID3E_NoFile;
  }

  // Determine the size of the file
  fseek(__file_handle, 0, SEEK_END);
  __file_size = ftell(__file_handle);
  fseek(__file_handle, 0, SEEK_SET);

  return ID3E_NoError;
}

bool ID3_Tag::CloseFile()
{
  bool bReturn = ((NULL != __file_handle) && (0 == fclose(__file_handle)));
  if (bReturn)
  {
    __file_handle = NULL;
  }
  return bReturn;
}

size_t ID3_Tag::Link(const char *fileInfo, bool parseID3v1, bool parseLyrics3)
{
  flags_t tt = ID3TT_NONE;
  if (parseID3v1)
  {
    tt |= ID3TT_ID3V1;
  }
  if (parseLyrics3)
  {
    tt |= ID3TT_LYRICS;
  }
  return this->Link(fileInfo, tt);
}

/** Attaches a file to the tag, parses the file, and adds any tag information
 ** found in the file to the tag.
 ** 
 ** Use this method if you created your ID3_Tag object without supplying a
 ** parameter to the constructor (maybe you created an array of ID3_Tag
 ** pointers).  This is the preferred method of interacting with files, since
 ** id3lib can automatically do things like parse foreign tag formats and
 ** handle padding when linked to a file.  When a tag is linked to a file,
 ** you do not need to use the <a href="#Size">Size</a>, <a
 ** href="#Render">Render</a>, or <a href="#Parse">Parse</a> methods or the
 ** <code>ID3_IsTagHeader</code> function---id3lib will take care of those
 ** details for you.  The single parameter is a pointer to a file name.
 ** 
 ** Link returns a 'luint' which is the byte position within the file that
 ** the audio starts (i.e., where the id3v2 tag ends).
 ** 
 ** \code
 **   ID3_Tag *myTag;
 **   if (myTag = new ID3_Tag)
 **   {
 **     myTag->Link("mysong.mp3");
 **     
 **     // do whatever we want with the tag
 **     // ...
 **   
 **     // setup all our rendering parameters
 **     myTag->SetUnsync(false);
 **     myTag->SetExtendedHeader(true);
 **     myTag->SetCompression(true);
 **     myTag->SetPadding(true);
 **     
 **     // write any changes to the file
 **     myTag->Update()
 **     
 **     // free the tag
 **     delete myTag;
 **   }
 ** \endcode
 ** 
 ** @see ID3_IsTagHeader
 ** @param fileInfo The filename of the file to link to.
 **/
size_t ID3_Tag::Link(const char *fileInfo, flags_t tag_types)
{
  __tags_to_parse.set(tag_types);
  
  if (NULL == fileInfo)
  {
    return 0;
  }

  // if we were attached to some other file then abort
  if (__file_handle != NULL)
  {
    // Log this
    CloseFile();
    //ID3_THROW(ID3E_TagAlreadyAttached);
  }
  
  strcpy(__file_name, fileInfo);
  __changed = true;

  if (ID3E_NoError == OpenFileForReading())
  {
    ParseFile();
    CloseFile();
  }
  
  return this->GetPrependedBytes();
}

size_t RenderV1ToHandle(ID3_Tag& tag, FILE* handle)
{
  if (handle == NULL)
  {
    // log this
    //ID3_THROW(ID3E_NoData);
    return 0;
    // cerr << "*** Ack! handle is null!" << endl;
  }
  
  uchar sTag[ID3_V1_LEN];
  size_t tag_size = tag.Render(sTag, ID3TT_ID3V1);

  if (tag_size > tag.GetAppendedBytes())
  {
    if (fseek(handle, 0, SEEK_END) != 0)
    {
      // TODO:  This is a bad error message.  Make it more descriptive
      ID3_THROW(ID3E_NoData);
    }
  }
  else
  {
    // We want to check if there is already an id3v1 tag, so we can write over
    // it.  First, seek to the beginning of any possible id3v1 tag
    if (fseek(handle, 0-tag_size, SEEK_END) != 0)
    {
      // TODO:  This is a bad error message.  Make it more descriptive
      ID3_THROW(ID3E_NoData);
    }
    
    char sID[ID3_V1_LEN_ID];
    // Read in the TAG characters
    if (fread(sID, 1, ID3_V1_LEN_ID, handle) != ID3_V1_LEN_ID)
    {
      // TODO:  This is a bad error message.  Make it more descriptive
      ID3_THROW(ID3E_NoData);
    }

    // If those three characters are TAG, then there's a preexisting id3v1 tag,
    // so we should set the file cursor so we can overwrite it with a new tag.
    if (memcmp(sID, "TAG", ID3_V1_LEN_ID) == 0)
    {
      if (fseek(handle, 0-tag_size, SEEK_END) != 0)
      {
        // TODO:  This is a bad error message.  Make it more descriptive
        ID3_THROW(ID3E_NoData);
      }
    }
    // Otherwise, set the cursor to the end of the file so we can append on 
    // the new tag.
    else
    {
      if (fseek(handle, 0, SEEK_END) != 0)
      {
        // TODO:  This is a bad error message.  Make it more descriptive
        ID3_THROW(ID3E_NoData);
      }
    }
  }
  
  fwrite(sTag, sizeof(uchar), tag_size, handle);

  return tag_size;
}

size_t RenderV2ToHandle(const ID3_Tag& tag, FILE*& handle)
{
  uchar *buffer = NULL;
  
  if (NULL == handle)
  {
    ID3_THROW(ID3E_NoData);
  }

  // Size() returns an over-estimate of the size needed for the tag
  size_t tag_size = 0;
  size_t size_est = tag.Size();
  if (size_est)
  {
    buffer = new uchar[size_est];
    if (NULL == buffer)
    {
      ID3_THROW(ID3E_NoMemory);
    }
  
    tag_size = tag.Render(buffer, ID3TT_ID3V2);
    if (!tag_size)
    {
      delete [] buffer;
      buffer = NULL;
    }
  }
  

  // if the new tag fits perfectly within the old and the old one
  // actually existed (ie this isn't the first tag this file has had)
  if ((!tag.GetPrependedBytes() && !ID3_GetDataSize(tag)) ||
      (tag_size == tag.GetPrependedBytes()))
  {
    fseek(handle, 0, SEEK_SET);
    if (buffer)
    {
      fwrite(buffer, 1, tag_size, handle);
    }
  }
  else
  {
#if !defined HAVE_MKSTEMP
    // This section is for Windows folk

    FILE *tempOut = tmpfile();
    if (NULL == tempOut)
    {
      ID3_THROW(ID3E_ReadOnly);
    }
    
    if (buffer)
    {
      fwrite(buffer, 1, tag_size, tempOut);
    }
    
    fseek(handle, tag.GetPrependedBytes(), SEEK_SET);
    
    uchar buffer2[BUFSIZ];
    while (! feof(handle))
    {
      size_t nBytes = fread(buffer2, 1, BUFSIZ, handle);
      fwrite(buffer2, 1, nBytes, tempOut);
    }
    
    rewind(tempOut);
    freopen(tag.GetFileName(), "wb+", handle);
    
    while (!feof(tempOut))
    {
      size_t nBytes = fread(buffer2, 1, BUFSIZ, tempOut);
      fwrite(buffer2, 1, nBytes, handle);
    }
    
    fclose(tempOut);
    
#else

    // else we gotta make a temp file, copy the tag into it, copy the
    // rest of the old file after the tag, delete the old file, rename
    // this new file to the old file's name and update the handle

    const char sTmpSuffix[] = ".XXXXXX";
    if (strlen(tag.GetFileName()) + strlen(sTmpSuffix) > ID3_PATH_LENGTH)
    {
      ID3_THROW_DESC(ID3E_NoFile, "filename too long");
    }
    char sTempFile[ID3_PATH_LENGTH];
    strcpy(sTempFile, tag.GetFileName());
    strcat(sTempFile, sTmpSuffix);
    
    int fd = mkstemp(sTempFile);
    if (fd < 0)
    {
      remove(sTempFile);
      ID3_THROW_DESC(ID3E_NoFile, "couldn't open temp file");
    }

    ofstream tmpOut(sTempFile);
    if (!tmpOut.is_open())
    {
      remove(sTempFile);
      ID3_THROW(ID3E_ReadOnly);
    }
    if (buffer)
    {
      tmpOut.write(buffer, tag_size);
    }
    fseek(handle, tag.GetPrependedBytes(), SEEK_SET);
      
    uchar buffer2[BUFSIZ];
    while (! feof(handle))
    {
      size_t nBytes = fread(buffer2, 1, BUFSIZ, handle);
      tmpOut.write(buffer2, nBytes);
    }
      
    tmpOut.close();

    fclose(handle);
    handle = NULL;

    remove(tag.GetFileName());
    rename(sTempFile, tag.GetFileName());
    
#endif
  }

  if (buffer)
  {
    delete [] buffer;
  }

  return tag_size;
}


/** Renders the tag and writes it to the attached file; the type of tag
 ** rendered can be specified as a parameter.  The default is to update only
 ** the ID3v2 tag.  See the ID3_TagType enumeration for the constants that
 ** can be used.
 ** 
 ** Make sure the rendering parameters are set before calling the method.
 ** See the Link dcoumentation for an example of this method in use.
 ** 
 ** \sa ID3_TagType
 ** \sa Link
 ** \param tt The type of tag to update.
 **/
flags_t ID3_Tag::Update(flags_t ulTagFlag)
{
  flags_t tags = ID3TT_NONE;

  OpenFileForWriting();
  if (NULL == __file_handle)
  {
    CreateFile();
  }
  if ((ulTagFlag & ID3TT_ID3V2) && this->HasChanged())
  {
    __prepended_bytes = RenderV2ToHandle(*this, __file_handle);
    if (__prepended_bytes)
    {
      tags |= ID3TT_ID3V2;
    }
  }
  
  OpenFileForWriting();
  if (NULL == __file_handle)
  {
    CreateFile();
  }
  if ((ulTagFlag & ID3TT_ID3V1) && 
      (!this->HasTagType(ID3TT_ID3V1) || this->HasChanged()))
  {
    size_t tag_bytes = RenderV1ToHandle(*this, __file_handle);
    if (tag_bytes)
    {
      // only add the tag_bytes if there wasn't an id3v1 tag before
      if (! __file_tags.test(ID3TT_ID3V1))
      {
        __appended_bytes += tag_bytes;
      }
      tags |= ID3TT_ID3V1;
    }
  }
  __changed = false;
  __file_tags.add(tags);
  CloseFile();
  return tags;
}

/** Strips the tag(s) from the attached file. The type of tag stripped
 ** can be specified as a parameter.  The default is to strip all tag types.
 ** 
 ** \param tt The type of tag to strip
 ** \sa ID3_TagType@see
 **/
flags_t ID3_Tag::Strip(flags_t ulTagFlag)
{
  flags_t ulTags = ID3TT_NONE;
  
  // First remove the v2 tag, if requested
  if (ulTagFlag & ID3TT_PREPENDED & __file_tags.get())
  {
    OpenFileForWriting();

    // We will remove the id3v2 tag in place: since it comes at the beginning
    // of the file, we'll effectively move all the data that comes after the
    // tag back n bytes, where n is the size of the id3v2 tag.  Once we've
    // copied the data, we'll truncate the file.
    //
    // To copy the data, we'll need to keep two "pointers" in the file: one
    // will mark where to read from next, the other will indicate where to 
    // write to. 
    long nNextWrite = ftell(__file_handle);
    // Set the read pointer past the tag
    fseek(__file_handle, this->GetPrependedBytes(), SEEK_CUR);
    long nNextRead = ftell(__file_handle);
    
    uchar aucBuffer[BUFSIZ];
    
    // The nBytesRemaining variable indicates how many bytes are to be copied
    size_t nBytesToCopy = ID3_GetDataSize(*this);

    // Here we increase the nBytesToCopy by the size of any tags that appear
    // at the end of the file if we don't want to strip them
    if (!(ulTagFlag & ID3TT_APPENDED))
    {
      nBytesToCopy += this->GetAppendedBytes();
    }
    
    // The nBytesRemaining variable indicates how many bytes are left to be 
    // moved in the actual file.
    // The nBytesCopied variable keeps track of how many actual bytes were
    // copied (or moved) so far.
    size_t 
      nBytesRemaining = nBytesToCopy,
      nBytesCopied = 0;
    while (! feof(__file_handle))
    {
      // Move to the next read position
      fseek(__file_handle, nNextRead, SEEK_SET);
      size_t
        nBytesToRead = MIN(nBytesRemaining - nBytesCopied, BUFSIZ),
        nBytesRead   = fread(aucBuffer, 1, nBytesToRead, __file_handle);
      // Now that we've read, mark the current spot as the next spot for
      // reading
      nNextRead = ftell(__file_handle);
      
      if (nBytesRead > 0)
      {
        // Move to the next write position
        fseek(__file_handle, nNextWrite, SEEK_SET);
        size_t nBytesWritten = fwrite(aucBuffer, 1, nBytesRead, __file_handle);
        if (nBytesRead > nBytesWritten)
        {
          // TODO: log this
          //cerr << "--- attempted to write " << nBytesRead << " bytes, "
          //     << "only wrote " << nBytesWritten << endl;
        }
        // Marke the current spot as the next write position
        nNextWrite = ftell(__file_handle);
        nBytesCopied += nBytesWritten;
      }
      
      if (nBytesCopied == nBytesToCopy)
      {
        break;
      }
      if (nBytesToRead < BUFSIZ)
      {
        break;
      }
    }
    CloseFile();
  }
  
  size_t nNewFileSize = ID3_GetDataSize(*this);

  if ((__file_tags.get() & ID3TT_APPENDED) && (ulTagFlag & ID3TT_APPENDED))
  {
    ulTags |= __file_tags.get() & ID3TT_APPENDED;
  }
  else
  {
    // if we're not stripping the appended tags, be sure to increase the file
    // size by those bytes
    nNewFileSize += this->GetAppendedBytes();
  }
  
  if ((ulTagFlag & ID3TT_PREPENDED) && (__file_tags.get() & ID3TT_PREPENDED))
  {
    // If we're stripping the ID3v2 tag, there's no need to adjust the new
    // file size, since it doesn't account for the ID3v2 tag size
    ulTags |= __file_tags.get() & ID3TT_PREPENDED;
  }
  else
  {
    // add the original prepended tag size since we don't want to delete it,
    // and the new file size represents the file size _not_ counting the ID3v2
    // tag
    nNewFileSize += this->GetPrependedBytes();
  }

  if (ulTags && (truncate(__file_name, nNewFileSize) == -1))
  {
    ID3_THROW(ID3E_NoFile);
  }

  __prepended_bytes = (ulTags & ID3TT_PREPENDED) ? 0 : __prepended_bytes;
  __appended_bytes  = (ulTags & ID3TT_APPENDED)  ? 0 : __appended_bytes;
  
  __changed = __file_tags.remove(ulTags) || __changed;
  
  return ulTags;
}
