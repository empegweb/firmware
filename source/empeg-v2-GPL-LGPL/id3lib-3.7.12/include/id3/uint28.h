// id3lib: a C++ library for creating and manipulating id3v1/v2 tags
// $Id: uint28.h,v 1.5 2000/06/22 19:31:30 adcockj Exp $

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

#ifndef __ID3LIB_UINT28_H__
#define __ID3LIB_UINT28_H__

#include "globals.h"
#include <iostream.h>

/** Class representation of an unsigned 28-bit integer for use in an ID3v2 tag.
 **
 ** The ID3v2 tag specification (http://www.id3.org) defines an unsigned 28-bit
 ** integer in which the most-significant bit in each of the integer's 4 bytes
 ** is set to 0.  This is done to avoid having integers in an id3v2 tag that
 ** might be mistaken for an MP3 sync-signal, which is why this representation
 ** is sometimes referred to as "syncsafe integers."
 **
 ** Since these 4 bits are unavailable as representation of the integer, this
 ** leaves 28 bits to define its value.  Therefore a uint28 object can
 ** represent values between 0 and 2^28-1 (or ~256M)
 **
 ** To be useful within id3lib's implementation of the ID3v2 spec, the uint28
 ** class enables instantiations to be created from and written to 4-byte (or
 ** more) character buffers.  In keeping with the ID3v2 spec, uint28 numbers
 ** use a big-endian byte ordering, such that the number 0x12345678 is
 ** represented as 0x12 0x34 0x56 0x78 in byte form.  A big-endian
 ** representation of 255, normally 0x00 0x00 0x00 0xFF for 32-bit integers,
 ** would be 0x00 0x00 0x01 0x7F for a 28-bit integer.
 **/
class uint28
{
  // Store the 28-bit integer's value as a regular 32-bit integer.  The Parse()
  // and and Render() methods handle the manipulation of its actual byte
  // representation.
  uint32 __value;
public:
  // can't initilaize statics here as windows compiler doesn't like it
  // these variables are initilaized in the cpp file
  /** The number of bits used per byte of an unsigned 28-bit integer **/
  static const unsigned short BITSUSED;
  /** The minimum value this class can represent **/
  static const uint32 MINVAL;
  /** The maximum value this class can represent (2^28-1) **/
  static const uint32 MAXVAL;

  /** Default constructor
   ** \param val The initial value for the (defaults to 0)
   **/
  explicit uint28(uint32 val = 0) { *this = val; }

  /** Creates a uint28 object from a character string
   ** \param data The character string to parse to create the uint28 object
   **/
  explicit uint28(const uchar* data) { *this = data; }

  /** Copy constructor
   ** \param rhs The uint28 object to copy
   **/
  uint28(const uint28& rhs) : __value(rhs.to_uint32()) { ; }

  virtual ~uint28() { ; }

  /** Assigns a uint28 object a new value
   **
   ** If the value assigned is greater than can be represented, 
   ** \param val The new value to assign to this uint28 object (should be less
   **            than 2^28)
   ** \return The newly-assigned uint28 object (*this)
   **/
  uint28&    operator=(uint32 val)
  { 
    // The value must not exceed uint28::MAX
    __value = (val > MAXVAL ? MAXVAL : val);
    return *this;
  }
  
  /** Assignment operator
   ** \param rhs The uint28 object to copy
   ** \return The newly-assigned uint28 object (*this)
   **/
  uint28&    operator=(const uint28& rhs) 
  { 
    if (this != &rhs)
    {
      __value = rhs.to_uint32(); 
    }
    return *this; 
  }
  
  /** Character string assignment operator
   ** \param data The character string from which to extract a 28-bit integer
   **             (must be at least 4 characters long)
   ** \return The newly-assigned uint28 object (*this)
   **/
  uint28&    operator=(const uchar* data)
  {
    this->Parse(data);
    return *this;
  }
  
  /** Converts the unsigned 28-bit integer object to a built-in unsigned 32-bit
   ** integer.
   **/
  uint32    to_uint32() const 
  {
    return __value; 
  }
  
  /** Extracts a 28-bit integer from a character string
   ** \param data The character string to parse (should be at least 4 
   **             characters in length) 
   ** \return The number of characters parsed (should always be 4)
   **/
  size_t    Parse(const uchar* data);
  
  /** Renders a 28-bit integer to a character string
   ** \param data The character string to render to (should be at least 4 
   **             characters in length)
   ** \return The number of characters render (should always be 4)
   **/
  size_t    Render(uchar*) const;

};

/** Render a 28-bit integer to a character stream **/
ostream& operator<<(ostream&, const uint28&);
/** Extract a 28-bit integer from a character stream **/
istream& operator>>(istream&, uint28&);

#endif /* __ID3LIB_UINT28_H__ */
