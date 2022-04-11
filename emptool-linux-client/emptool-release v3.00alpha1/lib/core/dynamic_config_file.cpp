/* dynamic_config_file.cpp
 *
 * Configuration file support
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.22 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "dynamic_config_file.h"
#include "stringpred.h"
#include "var_string.h"
#include "line_chopper.h"
#include <stdio.h>

#define TRACE_DCF 0

DynamicConfigFile::DynamicConfigFile()
    : m_lines_lock("DynamicConfigFile")
{
}

DynamicConfigFile::ConfigLine::ConfigLine(const tstring &s)
    : content(s)
{
#if DEBUG>0
    if (s.length() != 0)
    {
	TRACEC(TRACE_DCF, "First is '%c', last is '%c'\n", *s.begin(), *(s.end() - 1));
    }
#endif
    if (s.length() == 0 || s[0] == '#' || s[0] == ';')
    {
	type = COMMENT;
    }
    else if (*s.begin() == '[' && *(s.end() - 1) == ']')
    {
	type = SECTION;
    }
    else if (s.find_first_of('=') == std::string::npos)
    {
	type = COMMENT;
	content = tstring(_T("; ??? ")) + s;
    }
    else
    {
	type = KEY;
    }
}

DynamicConfigFile::ConfigLine::ConfigLine(Type t, const tstring &s)
    : type(t), content(s)
{
    ASSERT((type == KEY) || (type == SECTION) || (type ==COMMENT));
}

tstring DynamicConfigFile::ConfigLine::GetSection() const
{
    //TRACEC(TRACE_DCF, "Attempted GetSection on content '%s'\n", content.c_str());
    ASSERT(type == SECTION);
    ASSERT(content.length() > 0);
    ASSERT(*content.begin() == '[');
    ASSERT(*(content.end() - 1) == ']');
    return tstring(content.begin() + 1, content.end() - 1);
}

tstring DynamicConfigFile::ConfigLine::GetKey() const
{
    ASSERT(type == KEY);
    ASSERT(content.length() > 0);
    tstring::size_type divide = content.find_first_of('=');
    ASSERT(divide != tstring::npos);
    return tstring(content, 0, divide);
}

tstring DynamicConfigFile::ConfigLine::GetValue() const
{
    ASSERT(type == KEY);
    ASSERT(content.length() > 0);
    tstring::size_type divide = content.find_first_of('=');
    ASSERT(divide != tstring::npos);
    return tstring(content, divide + 1, std::string::npos);    
}


inline std::string trim(const std::string &s)
{
    const std::string::size_type content_start = s.find_first_not_of(" \t\r\n");
    const std::string::size_type content_stop = s.find_last_not_of(" \t\r\n");

    if (content_start == std::string::npos)
	return std::string();
    else
	return std::string(s, content_start, content_stop + 1);
}

void DynamicConfigFile::Clear()
{
    WriteLock lock(m_lines_lock);
    m_lines.clear();
}

void DynamicConfigFile::FromString(const std::string &s)
{
    Clear();

    AppendFromString(s);
}

void DynamicConfigFile::AppendFromString(const std::string & s)
{
    WriteLock lock(m_lines_lock);
    
    typedef std::string::size_type size_type;
    size_type line_start = 0;
    size_type data_length = s.length();

    while (line_start < data_length)
    {
	const size_type line_stop = s.find_first_of('\n', line_start);
	TRACEC(TRACE_DCF, "Found a line, starting at %d, stopping at %d\n",
	       (int) line_start, (int) line_stop);
	
	std::string line(trim(std::string(s, line_start, line_stop - line_start)));
	m_lines.push_back(ConfigLine(util::TFromSystem(line)));
	if (line_stop == std::string::npos)
	    line_start = data_length;
	else
	    line_start = line_stop + 1;
    }
}

void DynamicConfigFile::ToString(std::string *s) const
{
    ReadLock lock(m_lines_lock);
    
    tstring r;
    for(LineCollection::const_iterator i = m_lines.begin(); i != m_lines.end(); ++i)
    {
	r.append(i->content);
	r.append(_T("\n"));
    }

    *s = util::SystemFromT(r);
}

#if DEBUG>0
void DynamicConfigFile::Dump()
{
    ReadLock lock(m_lines_lock);
    
    for(LineCollection::const_iterator i = m_lines.begin(); i != m_lines.end(); ++i)
    {
	switch(i->type)
	{
	case ConfigLine::COMMENT:
	    TRACEC(TRACE_DCF, "COMMENT:%s\n", i->content.c_str());
	    break;
	case ConfigLine::SECTION:
	    TRACEC(TRACE_DCF, "SECTION:%s -> '%s'\n", i->content.c_str(), i->GetSection().c_str());
	    break;
	case ConfigLine::KEY:
	    TRACEC(TRACE_DCF, "KEY    :%s -> key=%s, value='%s'\n", i->content.c_str(), i->GetKey().c_str(), i->GetValue().c_str());
	    break;
	default:
	    TRACEC(TRACE_DCF, "???????:%s\n", i->content.c_str());
	    break;
	}
    }
}
#endif

// Must be called with either a read or write lock active.
DynamicConfigFile::iterator DynamicConfigFile::FindLine(const tstring &section, const tstring &key)
{
    iterator section_iterator = m_lines.begin();

    while((section_iterator = FindSection(section_iterator, section)) != m_lines.end())
    {
	// We've found an occurence of the section we're looking for.
	ASSERT(section_iterator->type == ConfigLine::SECTION);
	iterator key_iterator = section_iterator;

	for(++key_iterator; key_iterator != m_lines.end() && key_iterator->type != ConfigLine::SECTION; ++key_iterator)
	{
	    stringpred::IgnoreCaseEq eq;
	    if (key_iterator->type == ConfigLine::KEY && eq(key_iterator->GetKey(), key))
		return key_iterator;
	}
	++section_iterator;
    }
    return m_lines.end();
}

// Must be called with either a read or write lock active.
DynamicConfigFile::const_iterator DynamicConfigFile::FindLine(const tstring &section, const tstring &key) const
{
    const_iterator section_iterator = m_lines.begin();

    while((section_iterator = FindSection(section_iterator, section)) != m_lines.end())
    {
	// We've found an occurence of the section we're looking for.
	ASSERT(section_iterator->type == ConfigLine::SECTION);
	const_iterator key_iterator = section_iterator;

	for(++key_iterator; key_iterator != m_lines.end() && key_iterator->type != ConfigLine::SECTION; ++key_iterator)
	{
	    if (key_iterator->type == ConfigLine::KEY)
	    {
		stringpred::IgnoreCaseEq eq;
		if (eq(key_iterator->GetKey(), key))
		    return key_iterator;
	    }
	}
	++section_iterator;
    }
    return m_lines.end();
}
    
// Must be called with either a read or write lock active.
DynamicConfigFile::iterator DynamicConfigFile::FindSection(iterator start, const tstring &section)
{
    for(iterator section_iterator = start; section_iterator != m_lines.end(); ++section_iterator)
    {
	if (section_iterator->type == ConfigLine::SECTION)
	{
	    stringpred::IgnoreCaseEq eq;
	    if (eq(section_iterator->GetSection(), section))
		return section_iterator;
	}
    }
    return m_lines.end();
}
    
// Must be called with either a read or write lock active.
DynamicConfigFile::const_iterator DynamicConfigFile::FindSection(const_iterator start, const tstring &section) const
{
    for(const_iterator section_iterator = start; section_iterator != m_lines.end(); ++section_iterator)
    {
	if (section_iterator->type == ConfigLine::SECTION)
	{
	    stringpred::IgnoreCaseEq eq;
	    if (eq(section_iterator->GetSection(), section))
		return section_iterator;
	}
    }
    return m_lines.end();
}
    
#if 0
int DynamicConfigFile::GetIntegerValue(const tstring &section, const tstring &key, int default_value) const
{
    ReadLock lock(m_lines_lock);
    
    ASSERT(key.find_first_of('=') == std::string::npos);
    const_iterator key_iterator = FindLine(section, key);
    if (key_iterator != m_lines.end())
    {
	tstring s = key_iterator->GetValue();
	const TCHAR *ptr = s.c_str();
	const TCHAR *endptr;
	const int value = empeg_strtol(ptr, const_cast<TCHAR **>(&endptr));
	if (endptr != ptr)
	    return value;
	else
	    return default_value;
    }
    else
	return default_value;
}

const tstring DynamicConfigFile::GetStringValue(const tstring &section, const tstring &key, const tstring &default_value) const
{
    ReadLock lock(m_lines_lock);
    
    ASSERT(key.find_first_of('=') == std::string::npos);
    const_iterator key_iterator = FindLine(section, key);
    if (key_iterator != m_lines.end())
	return key_iterator->GetValue();
    else
	return default_value;
}
#endif

bool DynamicConfigFile::GetStringValue(const tstring &section, const tstring &key, tstring *value) const
{
    ReadLock lock(m_lines_lock);
    
    ASSERT(key.find_first_of('=') == std::string::npos);
    const_iterator key_iterator = FindLine(section, key);
    if (key_iterator != m_lines.end())
    {
	*value = key_iterator->GetValue();
	return true;
    }
    else
	return false;
}

bool DynamicConfigFile::GetIntegerValue(const tstring &section, const tstring &key, int *pvalue) const
{
    ReadLock lock(m_lines_lock);
    
    ASSERT(key.find_first_of('=') == std::string::npos);
    const_iterator key_iterator = FindLine(section, key);
    if (key_iterator != m_lines.end())
    {
	tstring s = key_iterator->GetValue();
	const TCHAR *ptr = s.c_str();
	const TCHAR *endptr;
	const int value = empeg_strtol(ptr, const_cast<TCHAR **>(&endptr));
	if (endptr != ptr)
	{
	    *pvalue = value;
	    return true;
	}
    }
    
    return false;
}

void DynamicConfigFile::SetStringValue(const tstring &section, const tstring &key, const tstring &value)
{
    WriteLock lock(m_lines_lock);
    
    TRACEC(TRACE_DCF, "Enter SetStringValue('%s', '%s', '%s')\n",
	   section.c_str(), key.c_str(), value.c_str());
    
    ASSERT(key.find_first_of('=') == std::string::npos);
    ASSERT(section.find_first_of('[') == std::string::npos);
    
    iterator key_iterator = FindLine(section, key);
    if (key_iterator != m_lines.end())
    {
	TRACEC(TRACE_DCF, "Found an existing key, updating it.\n");
	// We found an existing key, update it.
	key_iterator->content = key + _T('=') + value;
    }
    else
    {
	TRACEC(TRACE_DCF, "Looking for first occurence of section.\n");
	iterator section_iterator = FindSection(m_lines.begin(), section);
	if (section_iterator == m_lines.end())
	{
	    TRACEC(TRACE_DCF, "It's a new section.\n");
	    // We didn't even find the section, create it at the end
	    // of the file
	    ConfigLine line(ConfigLine::SECTION, _T('[') + section + _T(']'));
	    m_lines.push_back(line);
	    section_iterator = m_lines.end();
	}
	else
	{
	    ++section_iterator;
	}
	
	TRACEC(TRACE_DCF, "Inserting key=value pair.\n");
	// Now insert a new key=value pair.
	ConfigLine line(ConfigLine::KEY, key + _T('=') + value);
	m_lines.insert(section_iterator, line);
    }
}

void DynamicConfigFile::SetIntegerValue(const tstring &section, const tstring &key, int value)
{
    // Locking done in SetStringValue
    SetStringValue(section, key, VarString::TPrintf(_T("%d"), value));
}

bool DynamicConfigFile::DeleteKey(const tstring &section, const tstring &key)
{
    WriteLock lock(m_lines_lock);
    
    iterator key_iterator = FindLine(section, key);
    if (key_iterator != m_lines.end())
    {
        TRACEC(TRACE_DCF, "Found an existing key, updating it.\n");
        // We found an existing key, update it.
        m_lines.erase(key_iterator);
        return true;
    }
    
    return false;
}

tstring DynamicConfigFile::FindKey(const tstring &section, const tstring &value) const
{
    ReadLock lock(m_lines_lock);
    
    const_iterator section_iterator = m_lines.begin();

    while((section_iterator = FindSection(section_iterator, section)) != m_lines.end())
    {
        // We've found an occurence of the section we're looking for.
        ASSERT(section_iterator->type == ConfigLine::SECTION);
        const_iterator key_iterator = section_iterator;

        for(++key_iterator; key_iterator != m_lines.end() && key_iterator->type != ConfigLine::SECTION; ++key_iterator)
        {
            if (key_iterator->type == ConfigLine::KEY)
            {
		stringpred::IgnoreCaseEq eq;
                if (eq(key_iterator->GetValue(), value))
                    return key_iterator->GetKey();
            }
        }
        ++section_iterator;
    }

    return tstring();
}


        /* RegistryConfigInfo */


#ifdef WIN32

bool RegistryConfigInfo::GetIntegerValue(const tstring& section,
					 const tstring& key, int *value) const
{
    try {
	CRegistryKey rk(HKEY_CURRENT_USER, (m_root + _T("\\") + section).c_str(), false);
	DWORD  dw;
	if (rk.GetInteger(&dw, key.c_str()))
	{
	    *value = (int)dw;
	    return true;
	}
    }
    catch (...)
    {
    }
    return false;
}

bool RegistryConfigInfo::GetBooleanValue(const tstring& section,
					 const tstring& key, bool *value) const
{
    int temp;
    if (GetIntegerValue(section, key, &temp))
    {
	*value = (temp != 0);
	return true;
    }

    return false;
}

bool RegistryConfigInfo::GetStringValue(const tstring& section,
					const tstring& key, tstring *value) const
{
    try {
	CRegistryKey rk(HKEY_CURRENT_USER, (m_root + _T("\\") + section).c_str(), false);
	return rk.GetString(key.c_str(), value);
    }
    catch (...)
    {
    }
    return false;
}

void RegistryConfigInfo::SetIntegerValue(const tstring& section, const tstring& key,
					 int value)
{
    try {
	CRegistryKey rk(HKEY_CURRENT_USER, (m_root + _T("\\") + section).c_str(), true);
	rk.SetInteger(key, value);
    }
    catch (...)
    {
	TRACE_WARN("Setting registry value [%s] %s=%d failed\n", section.c_str(), key.c_str(), value);
    }
}

void RegistryConfigInfo::SetBooleanValue(const tstring& section, const tstring& key,
					 bool value)
{
    SetIntegerValue(section, key, value);
}

void RegistryConfigInfo::SetStringValue(const tstring& section, const tstring& key,
					const tstring& value)
{
    try {
	CRegistryKey rk(HKEY_CURRENT_USER, (m_root + _T("\\") + section).c_str(), true);
	rk.SetString(key, value);
    }
    catch (...)
    {
	TRACE_WARN("Setting registry value [%s] %s='%s' failed\n", section.c_str(), key.c_str(), value.c_str());
    }
}

#endif // WIN32

bool ConfigFileUTF8::GetInteger(const TCHAR *section, const TCHAR *key, int *value)
{
    utf8_string s;
    if (GetString8(section, key, &s))
    {
	*value = empeg_strtol(s.c_str(), NULL);
	return true;
    }

    return false;
}

void ConfigFileUTF8::SetInteger(const TCHAR *section, const TCHAR *key, int value)
{
    utf8_string s(VarString::Printf("%d", value));	// Since we're doing numbers, this is OK.
    SetString8(section, key, s);
}

class ConfigFileUTF8::FindSectionAndKey
{
    tstring m_section;
    tstring m_key;

public:
    FindSectionAndKey(const TCHAR *section, const TCHAR *key)
	: m_section(section), m_key(key) { }

    bool operator() (const ConfigValue &v)
    {
	stringpred::IgnoreCaseEq eq;
	return eq(v.section, m_section) && eq(v.key, m_key);
    }
};

bool ConfigFileUTF8::GetString8(const TCHAR *section, const TCHAR *key, utf8_string *value)
{
    values_t::const_iterator i = std::find_if(m_values.begin(), m_values.end(), FindSectionAndKey(section, key));
    if (i != m_values.end())
    {
	*value = i->value;
	return true;
    }

    return false;
}

void ConfigFileUTF8::SetString8(const TCHAR *section, const TCHAR *key, const utf8_string &value)
{
    values_t::iterator i = std::find_if(m_values.begin(), m_values.end(), FindSectionAndKey(section, key));
    if (i == m_values.end())
    {
	ConfigValue v;
	v.section = section;
	v.key = key;
	v.value = value;

	m_values.push_back(v);
    }
    else
	i->value = value;
}

void ConfigFileUTF8::FromString(const utf8_string &s)
{
    LineChopper<utf8_string> chop(s);

    utf8_string current_section;
    for (LineChopper<utf8_string>::const_iterator i = chop.begin(); i != chop.end(); ++i)
    {
	std::string line = *i;
	if (line.length() > 0)
	{
	    if ((line[0] == '[') && 
		(line[line.length()-1] == ']'))
	    {
		// It's a section header
		current_section = utf8_string(line.begin() + 1, line.begin() + line.length() - 1);
	    }
	    else
	    {
		utf8_string::size_type pos = line.find('=');
		if (pos != utf8_string::npos)
		{
		    utf8_string current_key(line.begin(), line.begin() + pos);
		    utf8_string value(line.begin() + pos + 1, line.end());

		    if ((util::UTF8Classify(current_section) == util::ASCII) &&
			(util::UTF8Classify(current_key) == util::ASCII))
		    {
			tstring section = util::TFromUTF8(current_section);
			tstring key = util::TFromUTF8(current_key);
			
			SetString8(section.c_str(), key.c_str(), value);
		    }
		    else
			ASSERT(false);	// Section and key names must be ASCII
		}
	    }
	}
    }
}

void ConfigFileUTF8::ToString(utf8_string *s)
{
    utf8_string results;

    utf8_string section;
    for (values_t::const_iterator i = m_values.begin(); i != m_values.end(); ++i)
    {
	if (i->section != util::TFromUTF8(section))
	{
	    section = util::UTF8FromT(i->section);

	    // This is OK -- section headings are in ASCII
	    results.append(VarString::Printf("[%s]\n", section.c_str()));
	}

	results.append(VarString::Printf("%s=%s\n", util::UTF8FromT(i->key).c_str(), i->value.c_str()));
    }

    *s = results;
}

