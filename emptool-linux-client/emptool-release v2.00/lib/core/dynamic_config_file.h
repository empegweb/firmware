/* dynamic_config_file.h
 *
 * Configuration file support
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.11 01-Apr-2003 18:52 rob:)
 */

#ifndef DYNAMIC_CONFIG_FILE_H
#define DYNAMIC_CONFIG_FILE_H

#include <string>
#include <vector>
#include "empeg_error.h"
#include "mutex.h"

class DynamicConfigFile
{
    class ConfigLine
    {
    public:
	enum Type { COMMENT, SECTION, KEY } type;
	std::string content;

	std::string GetKey() const;
	std::string GetSection() const;
	std::string GetValue() const;

	ConfigLine(const std::string &);
	ConfigLine(Type type, const std::string &);
    };
	
    typedef std::vector<ConfigLine> LineCollection;

    // For now we use a simple mutex but in the future we should use a
    // read/write lock.
    typedef Mutex ConfigFileLock;
    typedef MutexLock ReadLock;
    typedef MutexLock WriteLock;

    mutable ConfigFileLock m_lines_lock;
    
    LineCollection lines;

    typedef LineCollection::iterator iterator;
    typedef LineCollection::const_iterator const_iterator;
    
    iterator FindLine(const std::string &section, const std::string &key);
    const_iterator FindLine(const std::string &section, const std::string &key) const;

    iterator FindSection(iterator start, const std::string &section);
    const_iterator FindSection(const_iterator start, const std::string &section) const;
    
    static bool StringsEqual(const std::string &s1, const std::string &s2);

public:
    DynamicConfigFile();

    /** A genuine bug found with -Weffc++ (DhcpConfig inherits from us) */
    virtual ~DynamicConfigFile() {}

    void Clear();
    void FromString(const std::string &str);
    void AppendFromString(const std::string &str);
    void ToString(std::string *str) const;
#if DEBUG>0
    void Dump();
#endif

    int GetIntegerValue(const std::string &section, const std::string &key, int default_value) const;
    const std::string GetStringValue(const std::string &section, const std::string &key, const std::string &default_value) const;
    bool GetStringValue(const std::string &section, const std::string &key, std::string *value) const;
    bool GetBooleanValue(const std::string &section, const std::string &key, bool default_value) const
    {
	return GetIntegerValue(section, key, default_value) > 0;
    }

    // deprecated
    const std::string GetValue(const std::string &section, const std::string &key, const std::string &default_value) const
    {
	return GetStringValue(section, key, default_value);
    }

    void SetIntegerValue(const std::string &section, const std::string &key, int value);
    void SetStringValue(const std::string &section, const std::string &key, const std::string &value);
    void SetBooleanValue(const std::string &section, const std::string &key, bool value)
    {
        SetIntegerValue(section, key, (value) ? 1 : 0);
    }

    bool DeleteKey(const std::string &section, const std::string &key);
    std::string FindKey(const std::string &section, const std::string &value) const;
};

#endif
