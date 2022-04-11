/* hex.cpp
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "hex.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

void PrintHexLine(const char *pBuffer, unsigned cbBuffer)
{
    char hexPart[3 * 16 + 1];
    char textPart[16 + 1];
    
    hexPart[0]  = '\0';
    textPart[0] = '\0';
    
    for(unsigned i = 0; i < cbBuffer; ++i)
    {
	unsigned char ch = pBuffer[i];
	
	char hex[4];
	sprintf(hex, "%02x ", (unsigned char)ch);
	strcat(hexPart, hex);
	
	char text[2];
	if(isprint(ch))
	    sprintf(text, "%c", ch);
	else
	    sprintf(text, ".");
	
	strcat(textPart, text);
    }
    
    // pad it out...
    while(strlen(hexPart) < 48)
	strcat(hexPart, " ");
    
    char fullLine[(3 * 16) + 2 + (16) + 1];
    strcpy(fullLine, hexPart);
    strcat(fullLine, "  ");
    strcat(fullLine, textPart);
    
    printf("    %s\n", fullLine);
}

void PrintHex(const char *pBuffer, unsigned cbBuffer)
{
    const char *p = pBuffer;
    while(p < (pBuffer + cbBuffer) - 16)
    {
	PrintHexLine(p, 16);
	
	p += 16;
    }

    PrintHexLine(p, (pBuffer + cbBuffer) - p);
}
