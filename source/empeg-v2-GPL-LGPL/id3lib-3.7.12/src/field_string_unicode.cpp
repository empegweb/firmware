// $Id: field_string_unicode.cpp,v 1.17 2000/06/23 00:37:33 eldamitri Exp $

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

/** \fn ID3_Field& ID3_Field::operator=(const unicode_t*)
 ** \brief Shortcut for the Set operator.
 ** Performs similarly as operator=(const char*), taking a unicode_t
 ** string as a parameter rather than an ascii string.
 ** \sa Set(const unicode_t*)
 ** \param string The string to assign to the field
 **/

/** \brief Copies the supplied unicode string to the field.
 ** 
 ** Performs similarly as the ASCII Set() method, taking a unicode_t string
 ** as a parameter rather than an ascii string.
 ** 
 ** \param string The unicode string to set this field to.
 ** \sa Add(const unicode_t*)
 **/
void ID3_Field::Set(const unicode_t *string)
{
  size_t nBytes = (0 == __length) ? ucslen(string) : __length;
  
  // we can simply increment the nBytes count here because we just pilfer
  // the NULL which is present in the string which was passed to us
  if (__flags & ID3FF_CSTR)
  {
    nBytes++;
  }
    
  // doubling the nBytes because Unicode is twice the size of ASCII
  nBytes *= sizeof(unicode_t);
  
  Set((uchar *) string, nBytes);
  
  this->SetEncoding(ID3TE_UNICODE);
  __type = ID3FTY_TEXTSTRING;
  __changed = true;
  
  return ;
}


/** For fields which support this feature, adds a string to the list of
 ** strings currently in the field.
 ** 
 ** Performs similarly as the ASCII Add(const char*) method, taking a unicode_t
 ** string as a parameter rather than an ascii string.
 **/
void ID3_Field::Add(const unicode_t *string)
{
  if (NULL == __data)
  {
    Set(string);
  }
  else
  {
    unicode_t *uBuffer = (unicode_t *) __data;

    // +1 is for the NULL at the end and the other +1 is for the list divider
    size_t newLen = ucslen(string) + ucslen(uBuffer) + 1 + 1;
    
    unicode_t *temp = new unicode_t[newLen];
    if (NULL == temp)
    {
      ID3_THROW(ID3E_NoMemory);
    }

    ucscpy(temp, uBuffer);

    // I use the value 1 as a divider because then I can change it to either a
    // '/' or a NULL at render time.  This allows easy use of these functions
    // for text lists or in the IPLS frame
    temp[ucslen(uBuffer)] = L'\001';
    ucscpy(&temp[ucslen(uBuffer) + 1], string);
    temp[newLen - 1] = NULL_UNICODE;
      
    Set(temp);
      
    delete [] temp;
  }
  
  return ;
}


/** Copies the contents of the field into the supplied buffer, up to the
 ** number of characters specified; for fields with multiple entries, the
 ** optional third parameter indicates which of the fields to retrieve.
 ** 
 ** Performs similarly as the ASCII Get(char *, size_t, index_t) method, taking
 ** a unicode_t string as a parameter rather than an ascii string.  The
 ** maxChars parameter still represents the maximum number of characters, not
 ** bytes.
 **   
 ** \code
 **   unicode_t myBuffer[1024];
 **   size_t charsUsed = myFrame.Field(ID3FN_TEXT).Get(buffer, 1024);
 ** \endcode 
 **   
 ** \param buffer   Where the field's data is copied to
 ** \param maxChars The maximum number of characters to copy to the buffer.
 ** \param itemNum  For fields with multiple items (such as the involved
 **                 people frame, the item number to retrieve.
 ** \sa Get(char *, size_t, index_t)
 **/
size_t ID3_Field::Get(unicode_t *buffer, size_t maxChars, index_t itemNum) const
{
  size_t charsUsed = 0;
  
  // check to see if there is a string in the frame to copy before we even try
  if (NULL != __data)
  {
    lsint nullOffset = 0;
    
    if (__flags & ID3FF_CSTR)
    {
      nullOffset = -1;
    }
      
    // first we must find which element is being sought to make sure it exists
    // before we try to get it
    if (itemNum <= GetNumTextItems() && itemNum > 0)
    {
      unicode_t *source = (unicode_t *) __data;
      size_t posn = 0;
      size_t sourceLen = 0;
      index_t curItemNum = 1;
      
      // now we find that element and set the souvre pointer
      while (curItemNum < itemNum)
      {
        while (*source != L'\001' && *source != L'\0' && posn <
               ((__size / sizeof(unicode_t)) + nullOffset))
        {
          source++, posn++;
        }
          
        source++;
        curItemNum++;
      }
      
      // now that we are positioned at the first character of the string we
      // want, find the end of it
      while (source[sourceLen] != L'\001' && source[sourceLen] != L'\0' &&
             posn <((__size / sizeof(unicode_t) + nullOffset)))
      {
        sourceLen++, posn++;
      }
        
      if (NULL == buffer)
      {
        ID3_THROW(ID3E_NoBuffer);
      }

      size_t actualChars = MIN(maxChars, sourceLen);
        
      ucsncpy(buffer, source, actualChars);
      if (actualChars < maxChars)
      {
        buffer[actualChars] = L'\0';
      }
      charsUsed = actualChars;
    }
  }
  
  return charsUsed;
}


/** Returns the number of items in a text list.
 ** 
 ** \code
 **   size_t numItems = myFrame.Field(ID3FN_TEXT).GetNumItems();
 ** \endcode
 ** 
 ** \return The number of items in a text list.
 **/
size_t ID3_Field::GetNumTextItems() const
{
  size_t numItems = 0;
  
  if (NULL != __data)
  {
    index_t posn = 0;
    
    numItems++;
    
    while (posn < __size)
    {
      if (__data[posn++] == L'\001')
      {
        numItems++;
      }
    }
  }
  
  return numItems;
}


size_t 
ID3_Field::ParseUnicodeString(const uchar *buffer, size_t nSize)
{
  size_t nBytes = 0;
  unicode_t *temp = NULL;
  if (__length > 0)
  {
    nBytes = __length;
  }
  else
  {
    if (__flags & ID3FF_CSTR)
    {
      while (nBytes < nSize &&
             !(buffer[nBytes] == 0 && buffer[nBytes + 1] == 0))
      {
        nBytes += sizeof(unicode_t);
      }
    }
    else
    {
      nBytes = nSize;
    }
  }
  
  if (nBytes > 0)
  {
    // Sanity check our indices and sizes before we start copying memory
    if (nBytes > nSize)
    {
      ID3_THROW_DESC(ID3E_BadData, "field information invalid");
    }

    temp = new unicode_t[(nBytes / sizeof(unicode_t)) + 1];
    if (NULL == temp)
    {
      ID3_THROW(ID3E_NoMemory);
    }

    size_t loc = 0;

    memcpy(temp, buffer, nBytes);
    temp[nBytes / sizeof(unicode_t)] = NULL_UNICODE;
      
    // if there is a BOM, skip past it and check to see if we need to swap
    // the byte order around
    if (temp[0] == 0xFEFF || temp[0] == 0xFFFE)
    {
      loc++;
        
      // The following is taken from the following URL:
      // http://community.roxen.com/developers/idocs/rfc/rfc2781.html
      /* The Unicode Standard and ISO 10646 define the character "ZERO WIDTH
         NON-BREAKING SPACE" (0xFEFF), which is also known informally as "BYTE
         ORDER MARK" (abbreviated "BOM"). The latter name hints at a second
         possible usage of the character, in addition to its normal use as a
         genuine "ZERO WIDTH NON-BREAKING SPACE" within text. This usage,
         suggested by Unicode section 2.4 and ISO 10646 Annex F (informative),
         is to prepend a 0xFEFF character to a stream of Unicode characters as
         a "signature"; a receiver of such a serialized stream may then use the
         initial character both as a hint that the stream consists of Unicode
         characters and as a way to recognize the serialization order. In
         serialized UTF-16 prepended with such a signature, the order is
         big-endian if the first two octets are 0xFE followed by 0xFF; if they
         are 0xFF followed by 0xFE, the order is little-endian. Note that
         0xFFFE is not a Unicode character, precisely to preserve the
         usefulness of 0xFEFF as a byte-order mark. */

      if (0xFFFE == temp[0])
      {
        // we need to swap the byte order...
        for (index_t i = loc; i < ucslen(temp); i++)
        {
          uchar
            u1 = ((uchar *)(&temp[i]))[0],
            u2 = ((uchar *)(&temp[i]))[1];
          temp[i] = (u1 << 8) | u2;
        }
      }
    }
      
    Set(&temp[loc]);
      
    delete [] temp;
  }
  
  if (__flags & ID3FF_CSTR)
  {
    nBytes += sizeof(unicode_t);
  }
    
  __changed = false;
  
  return nBytes;
}


size_t ID3_Field::RenderUnicodeString(uchar *buffer) const
{
  size_t nBytes = 0;
  
  nBytes = BinSize();
  
  if (NULL != __data && __size && nBytes)
  {
    // we render at sizeof(unicode_t) bytes into the buffer because we make
    // room for the Unicode BOM
    memcpy(&buffer[sizeof(unicode_t)], (uchar *) __data, 
           nBytes - sizeof(unicode_t));
    
    unicode_t *ourString = (unicode_t *) &buffer[sizeof(unicode_t)];
    // now we convert the internal dividers to what they are supposed to be
    for (index_t i = sizeof(unicode_t); i < this->Size(); i++)
    {
      if (ourString[i] == 0x01)
      {
        unicode_t sub = L'/';
        
        if (__flags & ID3FF_LIST)
        {
          sub = L'\0';
        }
        
        ourString[i] = sub;
      }
    }
  }
  
  if (nBytes)
  {
    // render the BOM
    unicode_t *BOM = (unicode_t *) buffer;
    BOM[0] = 0xFEFF;
  }
  
  if (nBytes == sizeof(unicode_t) && (__flags & ID3FF_CSTR))
  {
    for (size_t i = 0; i < sizeof(unicode_t); i++)
    {
      buffer[i] = 0;
    }
  }
    
  __changed = false;
  
  return nBytes;
}
