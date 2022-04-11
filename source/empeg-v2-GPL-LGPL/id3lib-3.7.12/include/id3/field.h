// $Id: field.h,v 1.30 2000/06/14 16:13:28 eldamitri Exp $

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

#ifndef __ID3LIB_FIELD_H__
#define __ID3LIB_FIELD_H__

#include <stdlib.h>
#include "error.h"

struct ID3_FieldDef
{
  ID3_FieldID   eID;
  ID3_FieldType eType;
  size_t        ulFixedLength;
  ID3_V2Spec    eSpecBegin;
  ID3_V2Spec    eSpecEnd;
  flags_t       ulFlags;
  ID3_FieldID   eLinkedField;
  static const ID3_FieldDef* DEFAULT;
};

class ID3_Frame;
class ID3_Tag;

// Structure used for defining how frames are defined internally.
struct ID3_FrameDef
{
  ID3_FrameID   eID;
  char          sShortTextID[3 + 1];
  char          sLongTextID[4 + 1];
  bool          bTagDiscard;
  bool          bFileDiscard;
  const ID3_FieldDef* aeFieldDefs;
  const char *  sDescription;
};

class ID3_Field
{
  friend ID3_Frame;
public:
  ~ID3_Field();
  
  void Clear();

  size_t Size() const { return this->BinSize(false); }
  size_t GetNumTextItems() const;

  // integer field functions
  ID3_Field&    operator= (uint32 val) { this->Set(val); return *this; }
  void          Set(uint32);
  uint32        Get() const { return (uint32) __data; }

  // ASCII string field functions
  ID3_Field&    operator= (const char* s) { this->Set(s); return *this; }
  void          Set(const char*);
  size_t        Get(char*, size_t, index_t = 1) const;
  void          Add(const char*);

  // Unicode string field functions
  ID3_Field&    operator= (const unicode_t* s) { this->Set(s); return *this; }
  void          Set(const unicode_t*);
  size_t        Get(unicode_t *buffer, size_t, index_t = 1) const;
  void          Add(const unicode_t*);

  // binary field functions
  void          Set(const uchar*, size_t);
  void          Get(uchar*, size_t) const;
  void          FromFile(const char*);
  void          ToFile(const char *sInfo) const;
  
  // miscelaneous functions
  ID3_Field&    operator=( const ID3_Field & );
  const uchar*  GetBinary() const { return __data; }
  bool          InScope(ID3_V2Spec spec) const
  { return __spec_begin <= spec && spec <= __spec_end; }

  ID3_FieldID   GetID() const { return __id; }
  ID3_FieldType GetType() const { return __type; }
  bool          SetEncoding(ID3_TextEnc enc);
  ID3_TextEnc   GetEncoding() const { return __enc; }
  bool          IsEncodable() const { return (__flags & ID3FF_ENCODABLE) > 0; }
  

private:
  size_t        BinSize(bool withExtras = true) const;
  bool          HasChanged();
  //void          SetSpec(ID3_V2Spec);
  size_t        Render(uchar *buffer) const;
  size_t        Parse(const uchar *buffer, size_t buffSize);

private:
  // To prevent public instantiation, the constructor is made private
  ID3_Field();

  ID3_FieldID   __id;              // the ID of this field
  ID3_FieldType __type;            // what type is this field or should be
  size_t        __length;          // length of field (fixed if positive)
  ID3_V2Spec    __spec_begin;      // spec end
  ID3_V2Spec    __spec_end;        // spec begin
  flags_t       __flags;           // special field flags
  mutable bool  __changed;         // field changed since last parse/render?
  uchar        *__data;
  size_t        __size;
  ID3_TextEnc   __enc;             // encoding
protected:
  size_t RenderInteger(uchar *buffer) const;
  size_t RenderASCIIString(uchar *buffer) const;
  size_t RenderUnicodeString(uchar *buffer) const;
  size_t RenderBinary(uchar *buffer) const;
  
  size_t ParseInteger(const uchar *buffer, size_t);
  size_t ParseASCIIString(const uchar *buffer, size_t);
  size_t ParseUnicodeString(const uchar *buffer, size_t);
  size_t ParseBinary(const uchar *buffer, size_t);
  
};

// Ack! Not for public use
ID3_FrameDef *ID3_FindFrameDef(ID3_FrameID id);
ID3_FrameID   ID3_FindFrameID(const char *id);

#endif /* __ID3LIB_FIELD_H__ */
