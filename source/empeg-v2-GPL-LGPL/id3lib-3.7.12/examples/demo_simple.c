/* $Id: demo_simple.c,v 1.1 2000/04/21 05:49:20 eldamitri Exp $

 * Copyright 2000 Scott Thomas Haug <scott@id3.org>
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <id3.h>
#include <stdio.h>

int
main( int argc, char *argv[] )
{
  char *filename = NULL;
  ID3Tag *my_tag;
  
  if (argc != 2)
  {
    printf("*** Usage: %s file\n", argv[0]);
    exit (1);
  }
  filename = argv[1];
  printf("*** Reading %s\n", filename);
  
  if (my_tag = ID3Tag_New())
  {
    luint frm_size = ID3Tag_Link(my_tag, filename);
    ID3Frame *my_frame;
    if (my_frame = ID3Tag_FindFrameWithID(my_tag, ID3FID_TITLE))
    {
      ID3Field *my_field;
      if (my_field = ID3Frame_GetField(my_frame, ID3FN_TEXT))
      {
        char title[1024];
        luint num_chars = ID3Field_GetASCII(my_field, title, 1024, 1);
        printf("Title: %s\n", title);
      }
      else
      {
        printf("Didn't get the field\n");
      }
    }
    else
    {
      printf("Didn't get the frame\n");
    }
  }
  else
  {
    printf("Didn't get the tag\n");
  }
  exit (0);
}
