// id3lib: a C++ library for creating and manipulating id3v1/v2 tags
// $Id: uint28.cpp,v 1.5 2000/06/22 19:31:04 adcockj Exp $

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

#include "uint28.h"
#include <iomanip.h>

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

// values are initialized here
// this stops the window complier from complaining
const unsigned short uint28::BITSUSED = 7;
const uint32 uint28::MINVAL = 0;
const uint32 uint28::MAXVAL = MASK(BITSUSED*sizeof(uint32));

size_t uint28::Parse(const uchar* data)
{
  uint32 val = 0;
  // For each byte of the first 4 bytes in the string...
  for (size_t i = 0; i < sizeof(uint32); ++i)
  {
    // ...append the last 7 bits to the end of the temp integer...
    val = (val << BITSUSED) | static_cast<uint32>(data[i]) & MASK(BITSUSED);
  }
  // ...and assign this value to the object
  *this = val;
  // We should always parse 4 characters
  return sizeof(uint32);
}

size_t uint28::Render(uchar* data) const
{
  uint32 val = this->to_uint32();
  // This loop renders the value to the character buffer in reverse order, as 
  // it is easy to extract the last 7 bits of an integer.  This is why the
  // loop shifts the value of the integer by 7 bits for each iteration.
  for (size_t i = 0; i < sizeof(uint32); ++i)
  {
    // Extract the last BITSUSED bits from val and put it in its appropriate
    // place in the data buffer
    data[sizeof(uint32) - i - 1] = static_cast<uchar>(val & MASK(BITSUSED));

    // The last BITSUSED bits were extracted from the val.  So shift it to the
    // right by that many bits for the next iteration
    val >>= BITSUSED;
  }
  
  // Should always render 4 bytes
  return sizeof(uint32);
}

ostream& operator<<(ostream& os, const uint28& val)
{
  // The easiest way to write a 28-bit integer to a char stream is to render
  // it to a char buffer and then write the char buffer to the stream
  uchar data[sizeof(uint32)];
  val.Render(data);
  os.write(data, sizeof(uint32));
  return os;
}

istream& operator>>(istream& in, uint28& val)
{
  // The easiest way to extract a 28-bit integer from a char stream is to 
  // read 4 bytes into a char buffer and then extract the 28-bit integer from
  // the buffer
  uchar data[sizeof(uint32)];
  in.read(data, sizeof(uint32));
  val = data;
  return in;
}
