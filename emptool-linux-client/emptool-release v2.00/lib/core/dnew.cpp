/* dnew.cpp
 *
 * Memory leak detecting C++ operator new. Ideally suited for use with
 * Electric Fence.
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *
 * Originally (C) 1997-9 Mike Crowe.
 *
 * This source file is non-exclusively licensed for use by Empeg Ltd for however
 * it sees fit but without warranty.
 *
 * empeg ltd in turn licenses it to you under the GNU General Public Licence
 * (see file COPYING), unless you possess an alternative written licence from
 * empeg ltd.
 *
 * (:Empeg Source Release 1.32 01-Apr-2003 18:52 rob:)
 */

// Need to define new and delete here myself because otherwise electric fence
// doesn't work quite right with C++.

#include "config.h"
#include "trace.h"
#include "dnew.h"

#if !defined(WIN32)
#include "mutex.h"
#include "types.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <linux/kernel.h>
#include <linux/sys.h>
#include <linux/unistd.h>

_syscall1(int, sysinfo, struct sysinfo *, info);

#define TRACE_MEM		0
#define TRACE_MEM_PER_THREAD	0
#define TRACE_MEM_PER_FILE	0
#define DO_MEMORY_WALK		0

#undef new

#ifdef DEBUG_MEMORY

static void do_walk();

#if 0 //DEBUG != 4
// Debug=4 means electric fence, and we don't want this with efence
#define INTERCEPT_MALLOC
#endif

// #define INTERCEPT_MALLOC

#define MAGIC_ALLOCATED 0x0a110c8d
#define MAGIC_FREED 0x00FF00FF

#define UNKNOWN_SIMPLE 42
#define UNKNOWN_ARRAY 43
#define UNKNOWN_SIMPLE_NOTHROW 44
#define UNKNOWN_ARRAY_NOTHROW 45
#define UNKNOWN_MALLOC 46
#define UNKNOWN_REALLOC 47
#define UNKNOWN_FREE 48
#define KNOWN_HARMLESS_LEAK 49

#ifdef INTERCEPT_MALLOC
#define real_malloc __libc_malloc
#define real_free __libc_free
#define real_realloc __libc_realloc
#define real_calloc __libc_calloc

extern "C" void *real_malloc(size_t);
extern "C" void real_free(void *);
extern "C" void *real_realloc(void *, size_t);
extern "C" void *real_calloc(size_t, size_t);
#else
#define real_malloc malloc
#define real_free free
#define real_realloc realloc
#define real_calloc calloc
#endif

/// Used for catching people who allocate with new[] and free with
/// delete etc.
enum AllocType
{
    ALLOC_NEW = 0,
    ALLOC_ARRAYNEW = 1,
    ALLOC_MALLOC = 2
};

inline const char *AllocTypeName(AllocType t)
{
    switch(t)
    {
    case ALLOC_NEW:
	return "NEW";
    case ALLOC_ARRAYNEW:
	return "NEW[]";
    case ALLOC_MALLOC:
	return "EMALLOC";
    };
    return "unknown";
}

class my_link
{
public:
    unsigned long magic;
    int size;
    my_link *pPrev;
    my_link *pNext;
    char szFilename[32];
    AllocType alloc_type;
    int nLine;
    int nSeq;
    void *caller;
#if TRACE_MEM_PER_THREAD
    pthread_t thread_id;
#endif

    my_link() { pPrev = pNext = NULL; }
};

class guard_section
{
    unsigned long guard1;
    unsigned long guard2;
    unsigned long guard3;
    unsigned long guard4;

    enum { GUARD1_VALUE = 0xabcdef17,
	   GUARD2_VALUE = 0x563412ae,
	   GUARD3_VALUE = 0x0,
	   GUARD4_VALUE = 0x72791234,
    };

public:
    void Init()
    {
	ASSERT_PTR(this);
	guard1 = GUARD1_VALUE;
	guard2 = GUARD2_VALUE;
	guard3 = GUARD3_VALUE;
	guard4 = GUARD4_VALUE;
    }
    void Destroy()
    {
	ASSERT_PTR(this);
	guard1 = ~GUARD1_VALUE;
	guard2 = ~GUARD2_VALUE;
	guard3 = ~GUARD3_VALUE;
	guard4 = ~GUARD4_VALUE;
    }
    void Validate(const char *file, int line, void *called_by, int seq)
    {
	ASSERT_PTR(this);
	ASSERT_EX(guard1 == GUARD1_VALUE, "seq=%d guard1=0x%08lx - called by %p %s:%d\n",
		  seq, guard1, called_by, file, line);
	ASSERT_EX(guard2 == GUARD2_VALUE, "seq=%d guard1=0x%08lx - called by %p %s:%d\n",
		  seq, guard2, called_by, file, line);
	ASSERT_EX(guard3 == GUARD3_VALUE, "seq=%d guard1=0x%08lx - called by %p %s:%d\n",
		  seq, guard3, called_by, file, line);
	ASSERT_EX(guard4 == GUARD4_VALUE, "seq=%d guard1=0x%08lx - called by %p %s:%d\n",
		  seq, guard4, called_by, file, line);
    }
    bool ValidateNonFatal(const char *file, int line, void *called_by, int seq)
    {
	ASSERT_PTR(this);
	if (guard1 != GUARD1_VALUE)
	{
	    ERROR("seq=%d guard1(%p)=0x%08lx - called by %p %s:%d\n",
		  seq, &guard1, guard1, called_by, file, line);
	    return false;
	}
	if (guard2 != GUARD2_VALUE)
	{
	    ERROR("seq=%d guard2=0x%08lx - called by %p %s:%d\n",
		  seq, guard2, called_by, file, line);
	    return false;
	}
	if (guard3 != GUARD3_VALUE)
	{
	    ERROR("seq=%d guard3=0x%08lx - called by %p %s:%d\n",
		  seq, guard3, called_by, file, line);
	    return false;
	}
	if (guard4 != GUARD4_VALUE)
	{
	    ERROR("seq=%d guard4=0x%08lx - called by %p %s:%d\n",
		  seq, guard4, called_by, file, line);
	    return false;
	}
	return true;
    }
};

struct Sentinel
{
    char data[32];

public:
    void Init()
    {
	for(int i = 0; i < 32; i++)
	    data[i] = 42 + i;
    }
    bool Check()
    {
	for(int i = 0; i < 32; i++)
	    if (data[i] != 42 + i)
		return false;
	return true;
    }
};		

SimpleMutex alloc_mutex = SIMPLEMUTEX_INITIALISER;

my_link Head;
static int total_commit_bytes = 0;
static int total_commit_count = 0;
static int total_commit_ever = 0;

static int total_max_bytes = 0;
static int total_max_count = 0;

static int nSeqCount = 0;

int trace_mem_alloc = 0;

#if TRACE_MEM_PER_THREAD || TRACE_MEM_PER_FILE

template <class ID>
class AllocCount
{
    ID m_id;
    int m_allocs;
    int m_amount;
    int m_max_amount;
public:
    inline void Init() throw() {
	m_id = ID();
	m_allocs = 0;
	m_amount = 0;
	m_max_amount = 0;
    }

    inline ID GetId() const throw() { return m_id; }
    inline void SetId(const ID &id) throw() { m_id = id; }
    
    void Add(int amt) throw();
    void Remove(int amt) throw();

    int GetAllocCount() const { return m_allocs; }
    int GetAllocAmount() const { return m_amount; }
    int GetMaxAmount() const { return m_amount; }
};

template<class ID>
void AllocCount<ID>::Add(int amt) throw()
{
    m_allocs++;
    m_amount += amt;
    if(m_amount <= m_max_amount)
	return;
    m_max_amount = m_amount;
//    TRACE("m_max_amount:%d\n", m_max_amount);
}

template<class ID>
void AllocCount<ID>::Remove(int amt) throw()
{
    m_allocs--;
    m_amount -= amt;
    if(m_allocs >= 0 && m_amount >= 0)
	return;
    
//    TRACE("allocs:%d amount:%d\n", m_allocs, m_amount);
//    TRACE("allocs:%d amount:%d\n", m_allocs, m_amount);
    ASSERT(false);
}
#endif // TRACE_MEM_PER_THREAD || TRACE_MEM_PER_FILE

#if TRACE_MEM_PER_THREAD
class ThreadAllocCount
{
    enum { MAX_THREADS		= 64 };
    static SimpleMutex m_mutex;
    static AllocCount<pthread_t> *alloc_counts;

public:
    ThreadAllocCount() throw();
    ~ThreadAllocCount() throw();

    static AllocCount<pthread_t> *FindCount(pthread_t id) throw();
    static void Add(int amt) throw();
    static void Remove(pthread_t id, int amt) throw();
};

SimpleMutex ThreadAllocCount::m_mutex = SIMPLEMUTEX_INITIALISER;
AllocCount<pthread_t> *ThreadAllocCount::alloc_counts = NULL;

ThreadAllocCount::ThreadAllocCount() throw()
{
    alloc_counts = reinterpret_cast<AllocCount<pthread_t> *>(real_malloc(MAX_THREADS * sizeof(AllocCount<pthread_t>)));
    for(unsigned i = 0; i < MAX_THREADS; i++)
	alloc_counts[i].Init();
}

ThreadAllocCount::~ThreadAllocCount() throw()
{
    real_free(alloc_counts);
}

AllocCount<pthread_t> *ThreadAllocCount::FindCount(pthread_t id) throw()
{
    for(unsigned i = 0; i < MAX_THREADS; i++) {
	if(alloc_counts[i].GetId() == id)
	    return &alloc_counts[i];
    }
    for(unsigned i = 0; i < MAX_THREADS; i++) {
	if(!alloc_counts[i].GetId()) {
	    alloc_counts[i].SetId(id);
	    return &alloc_counts[i];
	}
    }
    ASSERT(false);
    return NULL;
}
    

void ThreadAllocCount::Add(int amt) throw()
{
    ASSERT_EX(amt >= 0, "amount added was %d\n", amt);
    SimpleMutexLock lock(&m_mutex);
    AllocCount<pthread_t> *count = FindCount(pthread_self());
    count->Add(amt);
}

void ThreadAllocCount::Remove(pthread_t id, int amt) throw()
{
    ASSERT_EX(amt >= 0, "amount removed was %d\n", amt);
    SimpleMutexLock lock(&m_mutex);
    AllocCount<pthread_t> *count = FindCount(id);
    count->Remove(amt);
}


static ThreadAllocCount thread_alloc_count;

#endif // TRACE_MEM_PER_THREAD

#if TRACE_MEM_PER_FILE
#include <string>

class FileAllocCount
{
    enum { MAX_FILES = 64 };
    static SimpleMutex m_mutex;

    struct Filename
    {
	char f[32];
    };
    
    static AllocCount<Filename> alloc_counts[MAX_FILES];
    static unsigned m_used;

public:
    static AllocCount<Filename> *FindCount(const char *file) throw();
    static void Add(const char *file, int amt) throw();
    static void Remove(const char *file, int amt) throw();

    static void Dump();
};

SimpleMutex FileAllocCount::m_mutex = SIMPLEMUTEX_INITIALISER;
AllocCount<FileAllocCount::Filename> FileAllocCount::alloc_counts[MAX_FILES];
unsigned FileAllocCount::m_used = 0;

void FileAllocCount::Add(const char *file, int amt)
{
    SimpleMutexLock lock(&m_mutex);
    AllocCount<Filename> *count = FindCount(file);
    count->Add(amt);
}

void FileAllocCount::Remove(const char *file, int amt)
{
    SimpleMutexLock lock(&m_mutex);
    AllocCount<Filename> *count = FindCount(file);
    count->Remove(amt);
}

AllocCount<FileAllocCount::Filename> *FileAllocCount::FindCount(const char *id)
{
    unsigned len = strlen(id);
    if (len > sizeof(Filename::f))
	len = sizeof(Filename::f);
    
    for(unsigned i = 0; i < MAX_FILES; i++)
    {
	if(strncmp(alloc_counts[i].GetId().f, id, len) == 0)
	    return &alloc_counts[i];
    }
    ASSERT(m_used < MAX_FILES);

    Filename fn;
    memset(fn.f, 0, sizeof(fn.f));
    strncpy(fn.f, id, sizeof(fn.f) - 1); // Last byte must be zero so we can trace it.
	    
    alloc_counts[m_used].SetId(fn);
    return &alloc_counts[m_used++];
}

void FileAllocCount::Dump()
{
    SimpleMutexLock lock(&m_mutex);
    TRACE("File memory usage ----------------\n");
    int total_bytes = 0;
    int total_calls = 0;
    for(unsigned i = 0; i < m_used; ++i)
    {
	AllocCount<Filename> *a = &alloc_counts[i];
	TRACE("  %-30s %7d bytes in %4d calls (max %7d)\n",
	      a->GetId().f,
	      a->GetAllocAmount(),
	      a->GetAllocCount(),
	      a->GetMaxAmount());
	total_bytes += a->GetAllocAmount();
	total_calls += a->GetAllocCount();
    }
    TRACE("  %-30s %7d bytes in %4d calls\n",
	  "TOTAL", total_bytes, total_calls);
    TRACE("----------------------------------\n");
}

#endif // TRACE_MEM_PER_FILE

void *my_alloc(size_t s, AllocType type, const char *szFile, int nLine, void *caller) THROW_NONE;
void *my_alloc_throw(size_t s, AllocType type, const char *szFile, int nLine, void *caller) THROW(std::bad_alloc);
void my_free(void *, AllocType type);

void *protected_alloc(size_t s, AllocType type, const char *szFile, int nLine, void *caller) THROW_NONE;
void *protected_alloc_throw(size_t s, AllocType type, const char *szFile, int nLine, void *caller) THROW_NONE;
void protected_free(my_link *);

void *operator new(size_t s) THROW(std::bad_alloc)
{
    void *result = my_alloc_throw(s, ALLOC_NEW, "Unknown", -UNKNOWN_SIMPLE, CALLED_BY);
    if(trace_mem_alloc) {
	TRACEC(TRACE_MEM, "Allocating unknown memory %u at %p, called by %p.\n", s, result, CALLED_BY);
    }
    return result;
}

void *operator new[](size_t s) THROW(std::bad_alloc)
{
    void *result = my_alloc_throw(s, ALLOC_ARRAYNEW, "Unknown array", -UNKNOWN_ARRAY, CALLED_BY);
    if(trace_mem_alloc) {
	TRACEC(TRACE_MEM, "Allocating unknown array %u at %p, called by %p.\n", s, result, CALLED_BY);
    }
    return result;
}

void *operator new(size_t s, const char *szFile, int nLine, void *caller) THROW(std::bad_alloc)
{
    if(!s) WARN("Attempt to allocate zero bytes %s:%d\n", szFile, nLine);
    return my_alloc_throw(s, ALLOC_NEW, szFile ? szFile : "Unspecified", nLine, caller ? caller : CALLED_BY);
}

void *operator new[](size_t s, const char *szFile, int nLine, void *caller) THROW(std::bad_alloc)
{
    return my_alloc_throw(s, ALLOC_ARRAYNEW, szFile ? szFile : "Unspecified", nLine, caller ? caller : CALLED_BY);
}

void *operator new(size_t s, nothrow_t) THROW_NONE
{
    void *result = my_alloc(s, ALLOC_NEW, "Unknown", -UNKNOWN_SIMPLE_NOTHROW, CALLED_BY);
    if(trace_mem_alloc) {
	TRACEC(TRACE_MEM, "Allocating unknown memory %u at %p.\n", s, result);
    }
    return result;
}

void *operator new[](size_t s, nothrow_t) THROW_NONE
{
    void *result = my_alloc(s, ALLOC_ARRAYNEW, "Unknown array", -UNKNOWN_ARRAY_NOTHROW, CALLED_BY);
    if(trace_mem_alloc) {
	TRACEC(TRACE_MEM, "Allocating unknown array %u at %p.\n", s, result);
    }
    return result;
}

void *operator new(size_t s, nothrow_t, const char *szFile, int nLine, void *caller) THROW_NONE
{
    if(!s) WARN("Attempt to allocate zero bytes %s:%d\n", szFile, nLine);
    return my_alloc(s, ALLOC_NEW, szFile ? szFile : "Unspecified", nLine, caller ? caller : CALLED_BY);
}

void *operator new[](size_t s, nothrow_t, const char *szFile, int nLine, void *caller) THROW_NONE
{
    if(!s) WARN("Attempt to allocate zero bytes %s:%d\n", szFile, nLine);
    return my_alloc(s, ALLOC_ARRAYNEW, szFile ? szFile : "Unspecified", nLine, caller ? caller : CALLED_BY);
}

void operator delete(void *p) THROW_NONE
{
    if(trace_mem_alloc) {
	my_link *l = ((my_link *)p) - 1;
	TRACEC(TRACE_MEM, "Deleting %s:%u size %u\n", l->szFilename, l->nLine, l->size);
    }
    my_free(p, ALLOC_NEW);
}

void operator delete[](void *p) THROW_NONE
{
    if(trace_mem_alloc) {
	my_link *l = ((my_link *)p) - 1;
	TRACEC(TRACE_MEM, "Deleting array %s:%u size %u\n", l->szFilename, l->nLine, l->size);
    }
    my_free(p, ALLOC_ARRAYNEW);
}

void *my_alloc(size_t s, AllocType type, const char *szFile, int nLine, void *caller) THROW_NONE
{
    if (nLine == -1)
    {
	nLine = -2;
    }

#if TRACE_MEM_PER_THREAD
    ThreadAllocCount::Add(s);
#endif
#if TRACE_MEM_PER_FILE
    FileAllocCount::Add(szFile,  s);
#endif
    
    if(!s) WARN("Attempt to allocate zero bytes %s:%d, called by %p\n",
		szFile, nLine, caller);

    const int aligned_size = (s + 3) & ~3;
    const int alloc_size = aligned_size + sizeof(my_link) + sizeof(guard_section) * 2;
    my_link *p = (my_link *)real_malloc(alloc_size);
    if(trace_mem_alloc || s>65535) {
	TRACEC(TRACE_MEM, "Allocated object size %u for %s:%d at %p\n", s, szFile, nLine, p+1);
    }
    if (p)
    {
	ASSERT_VALID(p);
	SimpleMutexLock sml(&alloc_mutex);
	do_walk();
	total_commit_bytes += alloc_size;
	total_commit_count++;
	total_commit_ever++;

	if(total_commit_bytes > total_max_bytes) {
	    total_max_bytes = total_commit_bytes;
	    TRACEC(TRACE_MEM, "total_max_bytes:%d\n", total_max_bytes);
	}
	if(total_commit_count > total_max_count) {
	    total_max_count = total_commit_count;
	    TRACEC(TRACE_MEM, "total_max_count:%d\n", total_max_count);
	}
	
	memset(p->szFilename, 0xff, sizeof(p->szFilename));
	ASSERT(strlen(szFile) < sizeof(p->szFilename) + 1);
	strcpy(p->szFilename, szFile);
	p->magic = MAGIC_ALLOCATED;
	p->nLine = nLine;
	p->nSeq = ++nSeqCount;
	p->caller = caller;
	p->pNext = Head.pNext;
	p->pPrev = &Head;
	p->size = s;
	p->alloc_type = type;
#if TRACE_MEM_PER_THREAD
	p->thread_id = pthread_self();
#endif
	Head.pNext = p;
	if (p->pNext)
	    p->pNext->pPrev = p;

	// After the header we have the guard section
	guard_section *g1 = (guard_section *)(p + 1);
	// Then we have the data
	char *q = (char *)(g1 + 1);
	// Then we have another guard section
	guard_section *g2 = (guard_section *)(q + aligned_size);

	ASSERT_EX((char *)(g2 + 1) == ((char *)p) + alloc_size, "end=%p, start=%p, size=%d\n", g2 + 1, p, alloc_size);

	g1->Init();
	g2->Init();
	g1->Validate(p->szFilename, p->nLine, p->caller, 0);
	g2->Validate(p->szFilename, p->nLine, p->caller, 1);
	g1->Validate(p->szFilename, p->nLine, p->caller, 0);
	g2->Validate(p->szFilename, p->nLine, p->caller, 1);

	memset(q, 0xcccccccc, p->size);
	
	return q;
    }
    else
	return NULL;
}

void *my_alloc_throw(size_t s, AllocType type, const char *szFile, int nLine, void *caller) THROW(std::bad_alloc)
{
    void *p = my_alloc(s, type, szFile, nLine, caller);
#ifdef __EXCEPTIONS
    if (!p)
	throw std::bad_alloc();
#endif
    return p;
}

void my_free(void *pv, AllocType type)
{
    if (pv)
    {
	ASSERT_VALID(pv);
	SimpleMutexLock sml(&alloc_mutex);
	do_walk();

	guard_section *g1 = (guard_section *)pv;
	--g1;
	my_link *p = (my_link *)g1;
	--p;

	ASSERT_EX(p->magic == MAGIC_ALLOCATED,
		  "Attempt to free memory at %p that wasn't allocated by us (magic was 0x%08lx).\n", pv, p->magic);

	ASSERT_EX(p->alloc_type == type, "Memory allocated with %s at %s:%d, deleted as if it was %s\n",
		  AllocTypeName(p->alloc_type), p->szFilename, p->nLine, AllocTypeName(type));

	// Check the first guard pointer.
	g1->Validate(p->szFilename, p->nLine, p->caller, 0);

	// Find and check the second guard pointer.
	const int aligned_size = (p->size + 3) & ~3;
	const int alloc_size = aligned_size + sizeof(my_link) + sizeof(guard_section) * 2;
	
	guard_section *g2 = (guard_section *)(((char *)pv) + aligned_size);
	g2->Validate(p->szFilename, p->nLine, p->caller, 1);
	
	g1->Destroy();
	g2->Destroy();
	
	p->magic = MAGIC_FREED;
	p->pPrev->pNext = p->pNext;
	if (p->pNext)
	    p->pNext->pPrev = p->pPrev;

	p->pPrev = NULL;
	p->pNext = NULL;
	total_commit_bytes -= alloc_size;
	total_commit_count--;

#if TRACE_MEM_PER_THREAD
	ThreadAllocCount::Remove(p->thread_id, p->size);
#endif
#if TRACE_MEM_PER_FILE
	FileAllocCount::Remove(p->szFilename, p->size);
#endif

	// Stomp all over the memory.
	memset(pv, 0xdddddddd, p->size);
	p->size = 0;
	real_free(p);

	do_walk();
    }
}

#if 0
const unsigned PROTECTED_PAGE_SIZE = 4096;
const unsigned PROTECTED_PAGE_MASK = (PROTECTED_PAGE_SIZE - 1);

void *protected_alloc(size_t size, AllocType type, const char *szFile, int nLine, void *caller) THROW_NONE
{
    SimpleMutexLock sml(&alloc_mutex);    

    TRACE("Enter protected_alloc\n");
    const unsigned pages = ((size + sizeof(struct my_link) + PROTECTED_PAGE_SIZE - 1) / PROTECTED_PAGE_SIZE);

    // First allocate some space in the memory map
    caddr_t page = reinterpret_cast<caddr_t>(mmap(NULL, (pages + 1) * PROTECTED_PAGE_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

    TRACE("page=%p, allocated = %d\n", page, (pages + 1) * PROTECTED_PAGE_SIZE);
    
    ASSERT_PTR(page);
    
    // Now let us talk to all but the last page.
    if (mprotect(page, pages * PROTECTED_PAGE_SIZE, PROT_READ | PROT_WRITE) < 0)
	ASSERT(false);

    // Now fiddle our pointer so that the end of the memory we've allocated falls right on the cusp.
    caddr_t block = (page + pages * PROTECTED_PAGE_SIZE) - ((size + 3) & ~3);

    my_link *link = reinterpret_cast<struct my_link *>(block) - 1;

    TRACE("block=%p, link=%p\n", block, link);

    link->magic = MAGIC_ALLOCATED;
    link->protect = true;
    strcpy(link->szFilename, szFile);
    link->nLine = nLine;
    link->nSeq = ++nSeqCount;
    link->caller = caller;
    link->pNext = Head.pNext;
    link->pPrev = &Head;
    link->size = size;
    link->alloc_type = type;
#if TRACE_MEM_PER_THREAD
    link->thread_id = pthread_self();
#endif

    Head.pNext = link;
    if (link->pNext)
	link->pNext->pPrev = link;

    total_commit_bytes += size;
    ++total_commit_count;

    TRACE("Returning %p\n", block);
    return block;
}

void *protected_alloc_throw(size_t s, AllocType type, const char *szFile, int nLine, void *caller) THROW(std::bad_alloc)
{
    void *p = protected_alloc(s, type, szFile, nLine, caller);
#ifdef __EXCEPTIONS
    if (!p)
	throw std::bad_alloc();
#endif
    return p;
}

void protected_free(my_link *link)
{
    link->magic = MAGIC_FREED;
    link->pPrev->pNext = link->pNext;
    if (link->pNext)
	link->pNext->pPrev = link->pPrev;
    
    link->pPrev = NULL;
    link->pNext = NULL;
    total_commit_bytes -= link->size;
    total_commit_count--;

    // Throw away the memory
    caddr_t page = reinterpret_cast<caddr_t>(reinterpret_cast<size_t>(link) & ~PROTECTED_PAGE_MASK);
    const unsigned pages = ((link->size + sizeof(struct my_link) + PROTECTED_PAGE_SIZE - 1) / PROTECTED_PAGE_SIZE);

    TRACE("link=%p, page=%p, pages=%d\n", link, page, pages);
    
    munmap(page, (pages + 1) * PROTECTED_PAGE_SIZE);
}
#endif

void dumpleaks()
{
    SimpleMutexLock sml(&alloc_mutex);
    do_walk();
    my_link *p = Head.pNext;
    if (p)
    {
	unsigned harmless_count = 0U;
	WARN("*LEAK* Memory leak dump...\n");
	while (p)
	{
//	    char buffer[64];
//	    const char *q = (const char *)(p+1);
//	    char *o = buffer;

	    if (p->nLine < 0)
	    {
		const char *reason;
		switch(-p->nLine)
		{
		case UNKNOWN_SIMPLE:
		    reason = "unknown new";
		    break;
		case UNKNOWN_ARRAY:
		    reason = "unknown new[]";
		    break;
		case UNKNOWN_SIMPLE_NOTHROW:
		    reason = "unknown new(nothrow)";
		    break;
		case UNKNOWN_ARRAY_NOTHROW:
		    reason = "unknown new(nothrow)[]";
		    break;
		case KNOWN_HARMLESS_LEAK:
		    ++harmless_count;
		    reason = NULL;
		    break;
		    
		default:
		    WARN("Unknown reason code: %d\n", p->nLine);
		    reason = "????";
		    break;
		}

		if (reason)
		    WARN("*LEAK* Object %d at %p size %u (%s) (caller=%p)\n",
			 p->nSeq, p+1, p->size, reason, p->caller);
	    }
	    else
	    {
		WARN("*LEAK* Object %d at %p size %u by %s:%d (caller=%p)\n",
		     p->nSeq, p+1, p->size, p->szFilename, p->nLine, p->caller);
	    }
			
	    p = p->pNext;
	}
	WARN("-----------------------------------------\n");
	WARN("Leak dump complete (%u harmless ignored).\n", harmless_count);
    }
    else
    {
	TRACE("No memory leaks detected. Good!\n");
    }
}

void DumpMemoryUsage()
{
#if TRACE_MEM_PER_FILE
    FileAllocCount::Dump();
#endif // TRACE_MEM_PER_FILE
}
    

void dumpfds()
{
    char fd_path[] = "/proc/self/fd";

    WARN("*Dumping fds...*\n");

    int fd_leaks = 0;
    
    DIR *d = opendir(fd_path);
    struct dirent *de;
    while ((de = readdir(d)) != NULL)
    {
	if ((strcmp(de->d_name, ".") == 0) ||
	    (strcmp(de->d_name, "..") == 0))
	    continue;
	
	char fd_file[255];
	strcpy(fd_file, fd_path);
	strcat(fd_file, "/");
	strcat(fd_file, de->d_name);
	
	char filename[255];
	int r = readlink(fd_file, filename, sizeof(filename));
	if (r < 0)
	{
	    WARN("readlink failed, errno = %d\n", errno);
	    filename[0] = '\0';
	}
	else
	    filename[r] = '\0';

	WARN(" %s -> %s\n", fd_file, filename);
	++fd_leaks;
    }
    
    closedir(d);

    /** @todo Adjust this for those that we don't plan on closing... */
    WARN("* %d fd(s) leaked*\n", fd_leaks);
    sleep(1);
}

#ifdef INTERCEPT_MALLOC
extern "C" void *malloc(size_t s)
{
    return my_alloc(s, "Unknown malloc", UNKNOWN_MALLOC, CALLED_BY);
}

extern "C" void free(void *p)
{
    return my_free(p);
}

extern "C" void *realloc(void *p, size_t s)
{
    return empeg_realloc(p, s, "Unknown realloc", UNKNOWN_REALLOC, CALLED_BY);
}

extern "C" void *calloc(size_t x, size_t y)
{
    void *ptr = my_alloc(x*y, "Unknown calloc", UNKNOWN_MALLOC, CALLED_BY);
    if ( ptr )
	memset( ptr, '\0', x*y );
    return ptr;
}
#endif

void *empeg_malloc(size_t s, const char *file, int line, void *caller)
{
    return my_alloc(s, ALLOC_MALLOC, file, line, caller);
}

void empeg_free(void *p, const char *, int, void *)
{
    return my_free(p, ALLOC_MALLOC);
}

void *empeg_calloc(size_t s, const char *file, int line, void *caller)
{
    void *p = my_alloc(s, ALLOC_MALLOC, file, line, caller);
    if (p)
	memset(p, 0, s);
    return p;
}

void *empeg_realloc(void *pv, size_t s, const char *file, int line, void *caller)
{
    TRACE("realloc called for data at %p.\n", pv);
    if(trace_mem_alloc) {
	TRACE("empeg_realloc called.\n");
    }
    if (line == -1)
    {
	line = -2;
    }

    if (pv == NULL)
	return empeg_malloc(s, file, line, caller);

    if (s == 0)
    {
	empeg_free(pv, file, line, caller);
	return NULL;
    }
    
    ASSERT_VALID(pv);

    if(!s) WARN("Attempt to reallocate zero bytes %s:%d, called by %p\n",
		file, line, caller);

    // Get the actual address of the block.
    SimpleMutexLock sml(&alloc_mutex);
    do_walk();

    guard_section *g1 = (guard_section *)pv;
    --g1;
    my_link *p = (my_link *)g1;
    --p;
    const int old_aligned_size = (p->size + 3) & ~3;
    const int old_alloc_size = old_aligned_size + sizeof(my_link) + sizeof(guard_section) * 2;
    guard_section *g2 = (guard_section *)(((char *)pv) + old_aligned_size);

    ASSERT_EX(p->magic == MAGIC_ALLOCATED,
	      "Attempt to realloc memory at %p that wasn't allocated by us (magic was 0x%08lx).", pv, p->magic);
    ASSERT(p->alloc_type == ALLOC_MALLOC);

    g1->Validate(p->szFilename, p->nLine, p->caller, 0);
    g2->Validate(p->szFilename, p->nLine, p->caller, 1);
    
    ASSERT(p->pPrev == NULL || p->pPrev->pNext == p);
    ASSERT(p->pNext == NULL || p->pNext->pPrev == p);

    // We need to add on the size of our link.
    const int new_aligned_size = (s + 3) & ~3;
    const int new_alloc_size = new_aligned_size + sizeof(my_link) + sizeof(guard_section) * 2;
    
    p = (my_link *)real_realloc(p, new_alloc_size);

    if (p)
    {
	// It worked
	int old_size = p->size;
	p->size = s;
	total_commit_bytes += (new_alloc_size - old_alloc_size);

	if(total_commit_bytes > total_max_bytes) {
	    total_max_bytes = total_commit_bytes;
	    TRACE("total_max_bytes:%d\n", total_max_bytes);
	}

	guard_section *g1 = (guard_section *)(p + 1);
	unsigned char *q = (unsigned char *)(g1 + 1);
	guard_section *old_g2 = (guard_section *)(q + old_aligned_size);
	old_g2->Validate(__FILE__, __LINE__, NULL, 1);
	old_g2->Destroy();
	
	if (p->size > old_size)
	    memset(q + old_size, 0xcccccccc, p->size - old_size);

	guard_section *new_g2 = (guard_section *)(q + new_aligned_size);
	new_g2->Init();

	// Better update those that point at us.
	if (p->pPrev)
	    p->pPrev->pNext = p;
	
	if (p->pNext)
	    p->pNext->pPrev = p;

	do_walk();

	return q;
    }
    else
    {
	// It failed - everything is unchanged.
	do_walk();
	
	return NULL;
    }
}

void empeg_walk()
{
    SimpleMutexLock sml(&alloc_mutex);
    do_walk();
}

#if DO_MEMORY_WALK
static void do_walk()
{
    // Must be locked when we do this.
    my_link *current = Head.pNext;
    my_link *previous = &Head;

    while (current)
    {
	ASSERT_PTR(current);
	ASSERT(current->magic == MAGIC_ALLOCATED);
	ASSERT(previous->pNext == current);
	ASSERT(previous == current->pPrev);
	ASSERT(current->pNext == NULL || current->pNext->pPrev == current);
	ASSERT(previous->pPrev == NULL || current->pPrev->pNext == current);

	const int aligned_size = (current->size + 3) & ~3;

	// Beyond the header is the guard.
	guard_section *g1 = (guard_section *)(current + 1);
	guard_section *g2 = (guard_section *)(((char *)(g1 + 1)) + aligned_size);

	if (!g1->ValidateNonFatal(current->szFilename, current->nLine, current->caller, 0)
	    || !g2->ValidateNonFatal(current->szFilename, current->nLine, current->caller, 1))
	{
	    TRACE("Data at %p, length 0x%x\n", g1 + 1, current->size);
	    TRACE("First guard is at %p, second guard is at %p\n", g1, g2);
	    TRACE_HEX("Memory contents\n", current, g2 + 1);
	    TRACE("Called by %p\n", CALLED_BY);
	    ASSERT(false);
	}

	previous = current;
	current = current->pNext;
    }
}
#else
static void do_walk()
{
}
#endif

class bing
{
public:
    bing() { ASSERT_EX((sizeof(my_link) & 3) == 0, "sizeof(my_link)=%d\n", sizeof(my_link)); }
    ~bing() { dumpleaks(); dumpfds(); }
};

static bing bingbing;

int GetCommitBytes()
{
    return total_commit_bytes;
}

int GetCommitCount()
{
    return total_commit_count;
}

int GetCommitCountEver()
{
    return total_commit_ever;
}

void empeg_meminfo()
{
    char buffer[80];
    int cache = -1;
    int vmlck = -1;
    
    struct sysinfo info;
    
    //int sysinfo(struct sysinfo *info);
    sysinfo(&info);
    
    FILE *str = fopen("/proc/meminfo", "r");
    while (1)
    {
	fgets(buffer, 80, str);
	if(feof(str))
	    break;
	if(!strncmp(buffer, "Cached:", 7))
	{
	    cache = atol(buffer+7);
	    break;
	}
    }
    fclose(str);
    
    str = fopen("/proc/self/status", "r");
    while (1)
    {
	fgets(buffer, 80, str);
	if(feof(str)) break;

	if(!strncmp(buffer, "VmLck:", 6))
	{
	    vmlck = atol(buffer+6);
	}
    }
    fclose(str);

    fprintf(stderr, "Total: %ld, Free: %ld, shared: %ld, buffers: %ld\n",
	    info.totalram, info.freeram, info.sharedram, info.bufferram);
    fprintf(stderr, "Cache: %d, vm locked: %d\n", cache, vmlck);
}

#else // DEBUG_MEMORY

#ifdef DO_WE_WANT_TO_USE_MALLOC_RATHER_THAN_THE_BUILTIN_NEW

void *alloc_throw(size_t s)
{
    void *p = malloc(s);
    if (p)
	return p;

    ERROR("Memory allocation failed!\n");

    return NULL;
}

void *operator new(size_t s) THROW(std::bad_alloc)
{
    return alloc_throw(s);
}

void *operator new[](size_t s) THROW(std::bad_alloc)
{
    return alloc_throw(s);
}

void *operator new(size_t s, nothrow_t) THROW_NONE
{
    return malloc(s);
}

void *operator new[](size_t s, nothrow_t) THROW_NONE
{
    return malloc(s);
}

void operator delete(void *p) THROW_NONE
{
    free(p);
}

void operator delete[](void *p) THROW_NONE
{
    free(p);
}

#endif

#endif // WIN32
#endif // DEBUG_MEMORY
