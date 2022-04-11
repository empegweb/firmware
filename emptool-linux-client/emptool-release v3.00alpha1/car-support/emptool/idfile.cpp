/* idfile.cpp
 *
 * Test program for filemagic library
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 13-Mar-2003 18:15 rob:)
 */

#include "filemagic.h"

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
	printf("Use the source\n");
	return 1;
    }
    
    FileMagic magic;
    FileMagic::Status status = magic.Identify(argv[1]);
    
    switch(status)
    {
   	case FileMagic::StatusNoInfo:
	    printf("No info available\n");
	    break;
	    
	case FileMagic::StatusHaveInfo:
	case FileMagic::StatusPartialInfo:
	    if (status == FileMagic::StatusPartialInfo)
	    {
		printf("Partial info available\n");
	    }
	    else
	    {
		printf("Have info available\n");
	    }	
	    
	    printf("length    : %d\n", magic.GetByteLength());
	    printf("title     : %s\n", magic.GetTitle().c_str());
	    printf("artist    : %s\n", magic.GetArtist().c_str());
	    printf("source    : %s\n", magic.GetSource().c_str());
	    printf("year      : %s\n", magic.GetYear().c_str());
	    printf("genre     : %s\n", magic.GetGenre().c_str());
	    printf("bitrate   : %s\n", magic.GetBitrate().c_str());
	    printf("comment   : %s\n", magic.GetComment().c_str());
	    printf("length(ms): %d\n", magic.GetLengthMs());
	    printf("sampleRate: %d\n", magic.GetSampleRate());
	    printf("trackNr   : %d\n", magic.GetTrackNum());
	    break;
	    
	case FileMagic::StatusNotFound:
	    printf("File not found\n");
	    break;
	    
	case FileMagic::StatusOpenFail:
	    printf("File open failed\n");
	    break;
	    
	case FileMagic::StatusUnknownType:
	    printf("File is of unknown type\n");
	    break;
	    
	default:
	    printf( "Unexpected filemagic:: code %d\n", status );
	    break;
    }
    
    return 0;
}
