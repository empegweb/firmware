/* dynamic_config_file.cpp
 *
 * Configuration file support
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.10.2.1 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "dynamic_config_file.h"
#include "stringpred.h"
#include <stdio.h>

#define TRACE_DCF 0


DynamicConfigFile::DynamicConfigFile()
    : m_lines_lock("DynamicConfigFile")
{
}

DynamicConfigFile::ConfigLine::ConfigLine(const std::string &s)
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
	content = std::string("; ??? ") + s;
    }
    else
    {
	type = KEY;
    }
}

DynamicConfigFile::ConfigLine::ConfigLine(Type t, const std::string &s)
    : type(t), content(s)
{
    ASSERT((type == KEY) || (type == SECTION) || (type ==COMMENT));
}

inline bool DynamicConfigFile::StringsEqual(const std::string &s1, const std::string &s2)
{
#if defined(WIN32)
    return stricmp(s1.c_str(), s2.c_str()) == 0;
#else
    return strcasecmp(s1.c_str(), s2.c_str()) == 0;
#endif
}

std::string DynamicConfigFile::ConfigLine::GetSection() const
{
    //TRACEC(TRACE_DCF, "Attempted GetSection on content '%s'\n", content.c_str());
    ASSERT(type == SECTION);
    ASSERT(content.length() > 0);
    ASSERT(*content.begin() == '[');
    ASSERT(*(content.end() - 1) == ']');
    return std::string(content.begin() + 1, content.end() - 1);
}

std::string DynamicConfigFile::ConfigLine::GetKey() const
{
    ASSERT(type == KEY);
    ASSERT(content.length() > 0);
    std::string::size_type divide = content.find_first_of('=');
    ASSERT(divide != std::string::npos);
    return std::string(content, 0, divide);
}

std::string DynamicConfigFile::ConfigLine::GetValue() const
{
    ASSERT(type == KEY);
    ASSERT(content.length() > 0);
    std::string::size_type divide = content.find_first_of('=');
    ASSERT(divide != std::string::npos);
    return std::string(content, divide + 1, std::string::npos);    
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
    lines.clear();
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
	TRACEC(TRACE_DCF, "Found a line, starting at %d, stopping at %d\n", line_start, line_stop);
	
	ConfigLine line(trim(std::string(s, line_start, line_stop - line_start)));
	lines.push_back(line);
	if (line_stop == std::string::npos)
	    line_start = data_length;
	else
	    line_start = line_stop + 1;
    }
}

void DynamicConfigFile::ToString(std::string *s) const
{
    ReadLock lock(m_lines_lock);
    
    for(LineCollection::const_iterator i = lines.begin(); i != lines.end(); ++i)
    {
	s->append(i->content);
	s->append("\n");
    }
}

#if DEBUG>0
void DynamicConfigFile::Dump()
{
    ReadLock lock(m_lines_lock);
    
    for(LineCollection::const_iterator i = lines.begin(); i != lines.end(); ++i)
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
DynamicConfigFile::iterator DynamicConfigFile::FindLine(const std::string &section, const std::string &key)
{
    iterator section_iterator = lines.begin();

    while((section_iterator = FindSection(section_iterator, section)) != lines.end())
    {
	// We've found an occurence of the section we're looking for.
	ASSERT(section_iterator->type == ConfigLine::SECTION);
	iterator key_iterator = section_iterator;

	for(++key_iterator; key_iterator != lines.end() && key_iterator->type != ConfigLine::SECTION; ++key_iterator)
	{
	    if (key_iterator->type == ConfigLine::KEY && StringsEqual(key_iterator->GetKey(), key))
		return key_iterator;
	}
	++section_iterator;
    }
    return lines.end();
}

// Must be called with either a read or write lock active.
DynamicConfigFile::const_iterator DynamicConfigFile::FindLine(const std::string &section, const std::string &key) const
{
    const_iterator section_iterator = lines.begin();

    while((section_iterator = FindSection(section_iterator, section)) != lines.end())
    {
	// We've found an occurence of the section we're looking for.
	ASSERT(section_iterator->type == ConfigLine::SECTION);
	const_iterator key_iterator = section_iterator;

	for(++key_iterator; key_iterator != lines.end() && key_iterator->type != ConfigLine::SECTION; ++key_iterator)
	{
	    if (key_iterator->type == ConfigLine::KEY)
	    {
		if (StringsEqual(key_iterator->GetKey(), key))
		    return key_iterator;
	    }
	}
	++section_iterator;
    }
    return lines.end();
}
    
// Must be called with either a read or write lock active.
DynamicConfigFile::iterator DynamicConfigFile::FindSection(iterator start, const std::string &section)
{
    for(iterator section_iterator = start; section_iterator != lines.end(); ++section_iterator)
    {
	if (section_iterator->type == ConfigLine::SECTION)
	{
	    if (StringsEqual(section_iterator->GetSection(), section))
		return section_iterator;
	}
    }
    return lines.end();
}
    
// Must be called with either a read or write lock active.
DynamicConfigFile::const_iterator DynamicConfigFile::FindSection(const_iterator start, const std::string &section) const
{
    for(const_iterator section_iterator = start; section_iterator != lines.end(); ++section_iterator)
    {
	if (section_iterator->type == ConfigLine::SECTION)
	{
	    if (StringsEqual(section_iterator->GetSection(), section))
		return section_iterator;
	}
    }
    return lines.end();
}
    
int DynamicConfigFile::GetIntegerValue(const std::string &section, const std::string &key, int default_value) const
{
    ReadLock lock(m_lines_lock);
    
    ASSERT(key.find_first_of('=') == std::string::npos);
    const_iterator key_iterator = FindLine(section, key);
    if (key_iterator != lines.end())
    {
	std::string s = key_iterator->GetValue();
	const char *ptr = s.c_str();
	const char *endptr;
	const int value = empeg_strtol(ptr, const_cast<char **>(&endptr));
	if (endptr != ptr)
	    return value;
	else
	    return default_value;
    }
    else
	return default_value;
}

const std::string DynamicConfigFile::GetStringValue(const std::string &section, const std::string &key, const std::string &default_value) const
{
    ReadLock lock(m_lines_lock);
    
    ASSERT(key.find_first_of('=') == std::string::npos);
    const_iterator key_iterator = FindLine(section, key);
    if (key_iterator != lines.end())
	return key_iterator->GetValue();
    else
	return default_value;
}

bool DynamicConfigFile::GetStringValue(const std::string &section, const std::string &key, std::string *value) const
{
    ReadLock lock(m_lines_lock);
    
    ASSERT(key.find_first_of('=') == std::string::npos);
    const_iterator key_iterator = FindLine(section, key);
    if (key_iterator != lines.end())
    {
	*value = key_iterator->GetValue();
	return true;
    }
    else
	return false;
}

void DynamicConfigFile::SetStringValue(const std::string &section, const std::string &key, const std::string &value)
{
    WriteLock lock(m_lines_lock);
    
    TRACEC(TRACE_DCF, "Enter SetStringValue('%s', '%s', '%s')\n",
	   section.c_str(), key.c_str(), value.c_str());
    
    ASSERT(key.find_first_of('=') == std::string::npos);
    ASSERT(section.find_first_of('[') == std::string::npos);
    
    iterator key_iterator = FindLine(section, key);
    if (key_iterator != lines.end())
    {
	TRACEC(TRACE_DCF, "Found an existing key, updating it.\n");
	// We found an existing key, update it.
	key_iterator->content = key + '=' + value;
    }
    else
    {
	TRACEC(TRACE_DCF, "Looking for first occurence of section.\n");
	iterator section_iterator = FindSection(lines.begin(), section);
	if (section_iterator == lines.end())
	{
	    TRACEC(TRACE_DCF, "It's a new section.\n");
	    // We didn't even find the section, create it at the end
	    // of the file
	    ConfigLine line(ConfigLine::SECTION, '[' + section + ']');
	    lines.push_back(line);
	    section_iterator = lines.end();
	}
	else
	{
	    ++section_iterator;
	}
	
	TRACEC(TRACE_DCF, "Inserting key=value pair.\n");
	// Now insert a new key=value pair.
	ConfigLine line(ConfigLine::KEY, key + '=' + value);
	lines.insert(section_iterator, line);
    }
}

void DynamicConfigFile::SetIntegerValue(const std::string &section, const std::string &key, int value)
{
    // Locking done in SetStringValue
    char buffer[32];
    sprintf(buffer, "%d", value);
    SetStringValue(section, key, buffer);
}

bool DynamicConfigFile::DeleteKey(const std::string &section, const std::string &key)
{
    WriteLock lock(m_lines_lock);
    
    iterator key_iterator = FindLine(section, key);
    if (key_iterator != lines.end())
    {
        TRACEC(TRACE_DCF, "Found an existing key, updating it.\n");
        // We found an existing key, update it.
        lines.erase(key_iterator);
        return true;
    }
    
    return false;
}

std::string DynamicConfigFile::FindKey(const std::string &section, const std::string &value) const
{
    ReadLock lock(m_lines_lock);
    
    const_iterator section_iterator = lines.begin();

    while((section_iterator = FindSection(section_iterator, section)) != lines.end())
    {
        // We've found an occurence of the section we're looking for.
        ASSERT(section_iterator->type == ConfigLine::SECTION);
        const_iterator key_iterator = section_iterator;

        for(++key_iterator; key_iterator != lines.end() && key_iterator->type != ConfigLine::SECTION; ++key_iterator)
        {
            if (key_iterator->type == ConfigLine::KEY)
            {
                if (StringsEqual(key_iterator->GetValue(), value))
                    return key_iterator->GetKey();
            }
        }
        ++section_iterator;
    }

    return "";
}
