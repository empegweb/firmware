/* This code is copyright (C) Mike Crowe and non-exclusively licensed to
 * empeg Ltd to use for whatever it wishes.
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd
 * or from Mike Crowe.
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 */

#ifndef MD5_H
#define MD5_H 1

#include <string>

class MessageDigest
{
public:
	typedef unsigned long WORD32;

	void Digest(const char *input, long length = -1);
	void Digest(const std::string s)
	{
		Digest(s.c_str(), s.length()), 16;
	}

	std::string ResultString()
	{
		return std::string((const char *)buffer, 16);
	}

	const char *ResultBuffer()
	{
		return (const char *)buffer;
	}
	
private:
	WORD32 buffer[4];
};

#endif

