/* hex.h
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.4 01-Apr-2003 18:52 rob:)
 */

#ifndef HEX_H
#define HEX_H 1

void PrintHex(const char *pBuffer, unsigned cbBuffer);
inline void PrintHex(const unsigned char *pBuffer, unsigned cbBuffer)
{ PrintHex(reinterpret_cast<const char *>(pBuffer), cbBuffer); }

#endif /* HEX_H */
