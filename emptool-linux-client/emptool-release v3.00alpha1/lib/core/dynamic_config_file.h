/* dynamic_config_file.h
 *
 * Configuration file support
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.21 13-Mar-2003 18:15 rob:)
 */

#ifndef DYNAMIC_CONFIG_FILE_H
#define DYNAMIC_CONFIG_FILE_H

#include <string>
#include <vector>
#include "empeg_status.h"
#include "empeg_tchar.h"
#include "mutex.h"
#include "utf8.h"
#ifdef WIN32
#include "win32/registry.h"
#endif

/** An abstract base class for configuration information, with two
 * implementations: one reading/writing a classic .ini-format file,
 * the other going for a Win32 registry tree.
 */
class ConfigInfo
{
public:
    virtual ~ConfigInfo() {}

    /** Gets a string; returns true if it was found, false otherwise */
    virtual bool GetStringValue(const tstring &section, const tstring &key, tstring *value) const = 0;
    virtual bool GetIntegerValue(const tstring &section, const tstring &key, int *value) const = 0;
    
    virtual void SetIntegerValue(const tstring &section, const tstring &key, int value) = 0;
    virtual void SetStringValue(const tstring &section, const tstring &key, const tstring &value) = 0;

    /** Implementation is allowed to persist changes either (a) immediately
     * or (b) only when you call this.
     */
    virtual void Commit() {}
};

class DynamicConfigFile: public ConfigInfo
{
    class ConfigLine
    {
    public:
	enum Type { COMMENT, SECTION, KEY } type;
	tstring content;

	tstring GetKey() const;
	tstring GetSection() const;
	tstring GetValue() const;

	ConfigLine(const tstring &);
	ConfigLine(Type type, const tstring &);
    };
	
    typedef std::vector<ConfigLine> LineCollection;

    // For now we use a simple mutex but in the future we should use a
    // read/write lock.
    typedef Mutex ConfigFileLock;
    typedef MutexLock ReadLock;
    typedef MutexLock WriteLock;

    mutable ConfigFileLock m_lines_lock;
    
    LineCollection m_lines;

    typedef LineCollection::iterator iterator;
    typedef LineCollection::const_iterator const_iterator;
    
    iterator FindLine(const tstring &section, const tstring &key);
    const_iterator FindLine(const tstring &section, const tstring &key) const;

    iterator FindSection(iterator start, const tstring &section);
    const_iterator FindSection(const_iterator start, const tstring &section) const;
    
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

    bool GetStringValue(const tstring &section, const tstring &key, tstring *value) const;
    bool GetBooleanValue(const tstring &section, const tstring &key, bool* pvalue) const
    {
	int intval;
	bool result = GetIntegerValue(section, key, &intval);
	if (result)
	    *pvalue = (intval != 0);
	return result;
    }
    virtual bool GetIntegerValue(const tstring& section, const tstring& key, int *pValue) const;

    /** Doesn't exist; is used to ensure compile failures of old code. We can't
     * just remove it, as otherwise people supplying 0 as the default would
     * call the other function passing NULL as pValue. Oops.
     */
    class UseTheOtherVersionOfGetIntegerValue GetIntegerValue(const tstring&, const tstring&, int) const;

    void SetIntegerValue(const tstring &section, const tstring &key, int value);
    void SetStringValue(const tstring &section, const tstring &key, const tstring &value);
    void SetBooleanValue(const tstring &section, const tstring &key, bool value)
    {
        SetIntegerValue(section, key, (value) ? 1 : 0);
    }

    bool DeleteKey(const tstring &section, const tstring &key);
    tstring FindKey(const tstring &section, const tstring &value) const;
};

#ifdef WIN32
class RegistryConfigInfo: public ConfigInfo
{
    tstring m_root;

public:
    RegistryConfigInfo(const tstring& root) : m_root(root) {}

    virtual bool GetStringValue(const tstring &section, const tstring &key, tstring *value) const;
    virtual bool GetIntegerValue(const tstring &section, const tstring &key, int *value) const;
    virtual bool GetBooleanValue(const tstring &section, const tstring &key, bool *value) const;
    
    virtual void SetIntegerValue(const tstring &section, const tstring &key, int value);
    virtual void SetStringValue(const tstring &section, const tstring &key, const tstring &value);
    virtual void SetBooleanValue(const tstring &section, const tstring &key, bool value);
};
#endif //def WIN32

class ConfigFileUTF8
{
    /** @todo We need to keep the lines around, so that we can write them back to
     * the file intact. */
    struct ConfigValue {
	tstring section;
	tstring key;
	utf8_string value;
    };

    // Maybe this ought to be a map for efficiency?
    // Preserving the order could be tricky, though.
    typedef std::vector<ConfigValue> values_t;
    values_t m_values;

    class FindSectionAndKey;

public:
    /** Get an integer value from the named section and key.  If present,
     * store it in the out param and return true.  If absent, return false,
     * leaving the out param unchanged.
     */
    bool GetInteger(const TCHAR *section, const TCHAR *key, int *value);
    void SetInteger(const TCHAR *section, const TCHAR *key, int value);

    /** Get a string value from the named section and key.  If present,
     * store it in the out param and return true.  If absent, return false,
     * leaving the out param unchanged.
     */
    bool GetString8(const TCHAR *section, const TCHAR *key, utf8_string *value);
    void SetString8(const TCHAR *section, const TCHAR *key, const utf8_string &value);

    void FromString(const utf8_string &s);
    void ToString(utf8_string *s);
};

#endif // DYNAMIC_CONFIG_FILE_H
