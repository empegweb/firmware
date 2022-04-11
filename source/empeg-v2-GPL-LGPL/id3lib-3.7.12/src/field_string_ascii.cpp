// $Id: field_string_ascii.cpp,v 1.13 2000/06/06 12:22:34 eldamitri Exp $

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
#include "field.h"
#include "utils.h"

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

/** \fn ID3_Field& ID3_Field::operator=(const char* data)
 ** \brief Shortcut for the Set operator.
 ** \param data The string to assign to this field
 ** \sa Set(const char*)
 **/

/** \brief Copies the supplied string to the field.
 ** You may dispose of the source string after a call to this method.
 ** \code
 **   myFrame.Field(ID3FN_TEXT).Set("ID3Lib is very cool!");
 ** \endcode
 **/
void ID3_Field::Set(const char *sString)
{
  if (sString != NULL)
  {
    Clear();
    size_t nStrLen = (0 == __length) ? strlen(sString) : __length;
    unicode_t *temp = new unicode_t[nStrLen + 1];
    if (NULL == temp)
    {
      ID3_THROW(ID3E_NoMemory);
    }

    mbstoucs(temp, sString, nStrLen + 1);

    Set(temp);
    delete [] temp;
      
    this->SetEncoding(ID3TE_ASCII);
    __type = ID3FTY_TEXTSTRING;
  }
  
  return ;
}


// the ::Get() function for ASCII

/** Copies the contents of the field into the supplied buffer, up to the
 ** number of characters specified; for fields with multiple entries, the
 ** optional third parameter indicates which of the fields to retrieve.
 ** 
 ** The third parameter is useful when using text lists (see Add(const char*)
 ** for more details).  The default value for this third parameter is 1,
 ** which returns the entire string if the field contains only one item.
 ** 
 ** It returns the number of characters (not bytes necessarily, and not
 ** including any NULL terminator) of the supplied buffer that are now used.
 ** 
 ** \code
 **   char myBuffer[1024];
 **   size_t charsUsed = myFrame.Field(ID3FN_TEXT).Get(buffer, 1024);
 ** \endcode
 ** 
 ** It fills the buffer with as much data from the field as is present in the
 ** field, or as large as the buffer, whichever is smaller.
 ** 
 ** \code
 **   char myBuffer[1024];
 **   size_t charsUsed = myFrame.Field(ID3FN_TEXT).Get(buffer, 1024, 3);
 ** \endcode
 ** 
 ** This fills the buffer with up to the first 1024 characters from the third
 ** element of the text list.
 ** 
 ** \sa Add(const char*)
 **/
size_t ID3_Field::Get(char* buffer,     ///< Where to copy the data
                      size_t maxLength, ///< Max number of characters to copy
                      index_t itemNum   ///< The item number to retrieve
                      ) const
{
  unicode_t* unicode_buffer = new unicode_t[maxLength];
  if (NULL == unicode_buffer)
  {
    ID3_THROW(ID3E_NoMemory);
  }

  size_t len = Get(unicode_buffer, maxLength, itemNum);

  char* ascii_buffer = new char[len + 1];
  if (NULL == ascii_buffer)
  {
    ID3_THROW(ID3E_NoMemory);
  }

  ucstombs(ascii_buffer, unicode_buffer, len + 1);

  size_t ascii_len = strlen(ascii_buffer);
  size_t length = MIN(ascii_len, maxLength);
        
  strncpy(buffer, ascii_buffer, length);
  buffer[length] = '\0';
        
  delete [] ascii_buffer;
    
  delete [] unicode_buffer;
    
  return length;
}


/** For fields which support this feature, adds a string to the list of
 ** strings currently in the field.
 ** 
 ** This is useful for using id3v2 frames such as the involved people list,
 ** composer, and part of set.  You can use the GetNumTextItems() method to
 ** find out how many such items are in a list.
 ** 
 ** \code
 **   myFrame.Field(ID3FN_TEXT).Add("this is a test");
 ** \endcode
 ** 
 ** \param string The string to add to the field
 **/
void ID3_Field::Add(const char *str)
{
  if (str)
  {
    unicode_t *unicode_buffer;
    
    unicode_buffer = new unicode_t[strlen(str) + 1];
    if (NULL == unicode_buffer)
    {
      ID3_THROW(ID3E_NoMemory);
    }

    mbstoucs(unicode_buffer, str, strlen(str) + 1);
    Add(unicode_buffer);
    delete [] unicode_buffer;
    
    this->SetEncoding(ID3TE_ASCII);
    __type = ID3FTY_TEXTSTRING;
  }
}


size_t 
ID3_Field::ParseASCIIString(const uchar *buffer, size_t nSize)
{
  size_t bytesUsed = 0;
  char *temp = NULL;
  
  if (__length > 0)
  {
    // The string is of fixed length
    bytesUsed = __length;
  }
  else if (!(__flags & ID3FF_CSTR) || (__flags & ID3FF_LIST))
  {
    // If the string isn't null-terminated or if it is null divided, we're
    // assured this is the last field of of the frame, and we can claim the
    // remaining bytes for ourselves
    bytesUsed = nSize;
  }
  else
  {
    while (bytesUsed < nSize && buffer[bytesUsed] != '\0')
    {
      bytesUsed++;
    }
  }

  if (0 == bytesUsed)
  {
    Set("");
  }
  // This check needs to come before the check for ID3FF_CSTR
  else if (__flags & ID3FF_LIST)
  {
    const char *sBuffer = (const char *) buffer;
    for (size_t i = 0; i < bytesUsed; i += strlen(&sBuffer[i]) + 1)
    {
      Add(&sBuffer[i]);
    }
  }
  // This check needs to come after the check for ID3FF_LIST
  else if (__flags & ID3FF_CSTR)
  {
    Set((const char *)buffer);
  }
  else
  {
    // Sanity check our indices and sizes before we start copying memory
    if (bytesUsed > nSize)
    {
      ID3_THROW_DESC(ID3E_BadData, "field information invalid");
    }

    temp = new char[bytesUsed + 1];
    if (NULL == temp)
    {
      ID3_THROW(ID3E_NoMemory);
    }
    
    memcpy(temp, buffer, bytesUsed);
    temp[bytesUsed] = '\0';
    Set(temp);
      
    delete [] temp;
  }
  
  if (__flags & ID3FF_CSTR && !(__flags & ID3FF_LIST))
  {
    bytesUsed++;
  }
    
  __changed = false;
  
  return bytesUsed;
}


size_t ID3_Field::RenderASCIIString(uchar *buffer) const
{
  size_t nChars = BinSize();

  if ((NULL != __data) && (nChars > 0))
  {
    ucstombs((char *) buffer, (unicode_t *) __data, nChars);
      
    // now we convert the internal dividers to what they are supposed to be
    for (index_t i = 0; i < nChars; i++)
    {
      if ('\1' == buffer[i])
      {
        char sub = '/';
          
        if (__flags & ID3FF_LIST)
        {
          sub = '\0';
        }
        buffer[i] = sub;
      }
    }
  }
  
  if ((1 == nChars) && (__flags & ID3FF_CSTR))
  {
    buffer[0] = '\0';
  }
    
  __changed = false;
  
  return nChars;
}
