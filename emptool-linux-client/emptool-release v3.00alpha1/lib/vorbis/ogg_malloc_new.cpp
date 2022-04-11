/* ogg_malloc_new.cpp
 *
 * Send ogg_malloc and friends via standard library
 *
 * (C) 2002 Empeg Limited
 */

#include "config.h"
#include "trace.h"

extern "C" {

#include "os_types.h"

void *_ogg_malloc(size_t sz)
{
    return malloc(sz);
}

void *_ogg_calloc(size_t x, size_t y)
{
    return calloc(x,y);
}

void *_ogg_realloc(void *p, size_t sz)
{
    return realloc(p, sz);
}

void _ogg_free(void *p)
{
    free(p);
}

}; // extern C
