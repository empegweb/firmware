// $Id: frame.h,v 1.28 2000/06/14 16:14:18 eldamitri Exp $

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

#ifndef __ID3LIB_FRAME_H__
#define __ID3LIB_FRAME_H__

#include "spec.h"
#include "header_frame.h"

class ID3_Field;
class ID3_Tag;

class ID3_Frame : public ID3_Speccable
{
public:
  ID3_Frame(ID3_FrameID id = ID3FID_NOFRAME);
  ID3_Frame(const ID3_FrameHeader&);
  ID3_Frame(const ID3_Frame&);

  /// Destructor.
  virtual ~ID3_Frame();
  
  void        Clear();

  bool        SetID(ID3_FrameID id);
  ID3_FrameID GetID() const { return __hdr.GetFrameID(); }
  
  ID3_Field  &Field(ID3_FieldID name) const;
  
  const char* GetDescription() const;
  static const char* GetDescription(ID3_FrameID);

  const char* GetTextID() const { return __hdr.GetTextID(); }

  ID3_Frame  &operator=(const ID3_Frame &);
  bool        HasChanged() const;
  size_t      Parse(const uchar *buffer, size_t size);
  size_t      Size();
  size_t      Render(uchar *buffer) const;
  bool        Contains(ID3_FieldID fld) const
  { return BS_ISSET(__field_bitset, fld) > 0; }
  bool        SetSpec(ID3_V2Spec);
  ID3_V2Spec  GetSpec() const;

  /** Sets the compression flag within the frame.  When the compression flag is
   ** is set, compression will be attempted.  However, the frame might not
   ** actually be compressed after it is rendered if the "compressed" data is
   ** no smaller than the "uncompressed" data.
   **/
  bool        SetCompression(bool b)  { return __hdr.SetCompression(b); }
  /** Returns whether or not the compression flag is set.  After parsing a tag,
   ** this will indicate whether or not the frame was compressed.  After
   ** rendering a tag, however, it does not actually indicate if the frame is
   ** compressed rendering.  It only indicates whether or not compression was
   ** attempted.  A frame will not be compressed, even whent the compression
   ** flag is set, if the "compressed" data is no smaller than the
   ** "uncompressed" data.
   **/
  bool        GetCompression() const  { return __hdr.GetCompression(); }
  bool        BadParse() const { return __bad_parse; }
  size_t      GetDataSize() const { return __hdr.GetDataSize(); }

protected:
  bool        _SetID(ID3_FrameID);
  bool        _ClearFields();
  void        _InitFields();
  void        _InitFieldBits();
  void        _UpdateFieldDeps();

  bool _SetEncryptionID(uchar id)
  {
    bool changed = id != __encryption_id;
    __encryption_id = id;
    __changed = __changed || changed;
    __hdr.SetEncryption(true);
    return changed;
  }
  uchar _GetEncryptionID() const { return __encryption_id; }
  bool _SetGroupingID(uchar id)
  {
    bool changed = id != __grouping_id;
    __grouping_id = id;
    __changed = __changed || changed;
    __hdr.SetGrouping(true);
    return changed;
  }
  uchar _GetGroupingID() const { return __grouping_id; }

private:
  mutable bool        __changed;    // frame changed since last parse/render?
  bitset      __field_bitset;       // which fields are present?
  size_t      __num_fields;         // how many fields are in this frame?
  ID3_Field **__fields;             // an array of field object pointers
  ID3_FrameHeader __hdr;            // 
  uchar       __encryption_id;      // encryption id
  uchar       __grouping_id;        // grouping id
  bool        __bad_parse;          //
}
;

#endif /* __ID3LIB_FRAME_H__ */
