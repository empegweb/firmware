/* id3_writer_test.cpp
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "tag_extractor.h"
#include "tag_writer.h"
#include "var_string.h"
#include "id3v1_format.h"
#include <set>
#include <string>
#include <unistd.h>

#define TESTFILE1 "testfile1.mp3"

// MD5 of the empty string
#define EMPTY_RID "d41d8cd98f00b204e9800998ecf8427e"

typedef struct
{
    const char *tag;
    const char *value; // NULL means "any value allowed"
} expected_tag;

class TagCheckObserver: public TagExtractorObserver
{
    const expected_tag *m_tags;
    unsigned int m_nTags;
    std::set<std::string> m_seen;

public:
    TagCheckObserver(const expected_tag *tags, unsigned int nTags);
    ~TagCheckObserver();

    virtual void OnExtractTag(const char *tag, const char *value);
    virtual void OnExtractTag(const char *tag, int value);
};

TagCheckObserver::TagCheckObserver(const expected_tag *tags, unsigned int nTags)
    : m_tags(tags), m_nTags(nTags)
{
}

TagCheckObserver::~TagCheckObserver()
{
    for (unsigned int i=0; i<m_nTags; ++i)
    {
	if (m_seen.find(m_tags[i].tag) == m_seen.end())
	{
	    fprintf(stderr, "Tag '%s' not extracted, should be '%s'\n",
		    m_tags[i].tag, m_tags[i].value);
	    ASSERT(false);
	    exit(1);
	}
    }
}

void TagCheckObserver::OnExtractTag(const char *tag, const char *value)
{
    for (unsigned int i=0; i<m_nTags; ++i)
    {
	if (!strcmp(m_tags[i].tag, tag))
	{
	    m_seen.insert(tag);
	    if (!m_tags[i].value)
		return;
	    if (strcmp(value,m_tags[i].value))
	    {
		fprintf(stderr, "Tag '%s' is '%s' should be '%s'\n",
			tag, value, m_tags[i].value);
		ASSERT(false);
		exit(1);
	    }
	    return;
	}
    }
    fprintf(stderr, "Warning, tag '%s' not checked (it's '%s')\n", tag, value);
}

void TagCheckObserver::OnExtractTag(const char *tag, int value)
{
    char buf[20];
    sprintf(buf, "%d", value);
    OnExtractTag(tag, buf);
}

void check_tags(const char *file, const expected_tag *tags, unsigned int nTags)
{
    TagCheckObserver tco(tags, nTags);
    
    TagExtractor *te;

    VERIFY(SUCCEEDED(TagExtractor::Create(file, "mp3", &te)));
    STATUS st = te->ExtractTags(file, &tco);
    ASSERT(SUCCEEDED(st) || st == E_INVALID_MP3);
    delete te;
}

void write_tag(const char *file, const char *tag, const char *value)
{
    tags::TagWriter *tw;
    VERIFY(SUCCEEDED(tags::TagWriter::Create(file, "mp3", &tw)));
    VERIFY(SUCCEEDED(tw->SetTag(tag, value)));
    VERIFY(SUCCEEDED(tw->Write()));
    delete tw;
}

void test1()
{
    remove(TESTFILE1);
    FILE *f = fopen(TESTFILE1, "wb");
    fclose(f);

    static const expected_tag no_tags[] =
    {
	{ "codec", "mp3" },
	{ "offset", "0" },
        { "trailer", "0" },
	{ "rid", EMPTY_RID },
    };

    check_tags(TESTFILE1, no_tags, sizeof(no_tags)/sizeof(no_tags[0]));
}

void check_write_tag(const char *tag, const char *value)
{
    remove(TESTFILE1);
    FILE *f = fopen(TESTFILE1, "wb+");
    fclose(f);

    write_tag(TESTFILE1, tag, value);

    static expected_tag one_tag[] =
    {
        { "--placeholder--", "--placeholder--" },
	{ "codec", "mp3" },
	{ "offset", NULL },
	    { "rid", EMPTY_RID },
        { "trailer", "0" },
    };

    one_tag[0].tag = tag;
    one_tag[0].value = value;

    check_tags(TESTFILE1, one_tag, sizeof(one_tag)/sizeof(one_tag[0]));
}

void test2()
{
    check_write_tag("artist", "New Order");
    check_write_tag("source", "Substance");
    check_write_tag("title",  "Shellshock");
    check_write_tag("year", "1987");
    check_write_tag("genre", "Electronic");
    check_write_tag("tracknr", "6");
}

void check_write_v1_tag(const char *tag, const char *value)
{
    remove(TESTFILE1);
    FILE *f = fopen(TESTFILE1, "wb");
    static ID3V1_Tag v1 = { "TA", "Wombat", "Fnord", "Gurgle", "198",
				"Aubergine", 3 };
    memcpy(v1.signature, "TAG", 3);
    memcpy(v1.year, "1984", 4);

    fwrite(&v1, 128, 1, f);
    fclose(f);

    static const expected_tag v1_tags[] =
    {
	{ "title", "Wombat" },
	    { "artist", "Fnord" },
		{ "source", "Gurgle" },
		    { "year", "1984" },
			{ "comment", "Aubergine" },
			    { "genre", "Dance" },
				{ "trailer", "128" },
				    { "offset", "0" },
					{ "codec", "mp3" },
					    { "rid", EMPTY_RID }
    };

    check_tags(TESTFILE1, v1_tags, sizeof(v1_tags)/sizeof(v1_tags[0]));

    write_tag(TESTFILE1, tag, value);

    expected_tag v1_tags2[] =
    {
	{ "artist", "Fnord" },
	    { "title", "Wombat" },
		{ "source", "Gurgle" },
		    { "year", "1984" },
			{ "comment", "Aubergine" },
			    { "genre", "Dance" },
				{ "trailer", "128" },
				    { "offset", NULL },
					{ "codec", "mp3" },
					    { "rid", EMPTY_RID }
    };

    for (unsigned int i=0; i<sizeof(v1_tags2)/sizeof(v1_tags2[0]); ++i)
    {
	if (!strcmp(v1_tags2[i].tag, tag))
	    v1_tags2[i].value = value;
    }
    
    check_tags(TESTFILE1, v1_tags2, sizeof(v1_tags2)/sizeof(v1_tags2[0]));

    /* Check we wrote the V1 tag too */
    f = fopen(TESTFILE1, "rb+");
    VERIFY(fseek(f, -128, SEEK_END) == 0);
    ID3V1_Tag newtag;
    VERIFY(fread(&newtag, 128, 1, f) == 1);
    VERIFY(fseek(f, 0, SEEK_SET) == 0);
    VERIFY(fwrite(&newtag, 128, 1, f) == 1);
    fclose(f);
    VERIFY(truncate(TESTFILE1, 128) == 0);

    check_tags(TESTFILE1, v1_tags2, sizeof(v1_tags2)/sizeof(v1_tags2[0]));
}

void test3()
{
    check_write_v1_tag("artist",  "New Order");
    check_write_v1_tag("source", "Substance");
    check_write_v1_tag("title",  "Shellshock");
    check_write_v1_tag("year", "1987");
    check_write_v1_tag("genre", "Electronic");
    check_write_v1_tag("tracknr", "6");
}

int main()
{
    test1();
    test2();
    test3();

    remove(TESTFILE1);

    printf("Tests OK\n");

    return 0;
}
