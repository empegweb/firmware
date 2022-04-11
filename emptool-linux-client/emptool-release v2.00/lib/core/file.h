/* file.h
 *
 * File and filesystem-related utility routines
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.13 01-Apr-2003 18:52 rob:)
 */

#ifndef UTIL_FILE_H
#define UTIL_FILE_H

#include <string>

#ifndef EMPEG_ERROR_H
#include "empeg_error.h"
#endif

namespace util {

/** Get the filename, including the extension. */
std::string GetFileName(const std::string &full_path);
std::string GetPathName(const std::string &full_path);

/** Get the filename, not including the extension. */
std::string TruncateFilename(const std::string &filename);

/** Put two path components together, inserting a slash or backslash if required. */
std::string AppendPath(const std::string &a, const std::string &b);

std::string GetFileExtension(const std::string &filename);
std::string AppendSlash(std::string s);
bool IsSpecialDirectory(const char *path);
std::string GetCurrentDirectory();
STATUS EnsurePathExists(const std::string &path);
int GetFileSize(const char *filename, unsigned *file_size);

};

#endif
