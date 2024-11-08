#include <stdlib.h>
#include <string.h>

#include "cmsis_os2.h"


void free(void *ptr)
{
    if (ptr == NULL)
        return;

    osMemoryFree(ptr);/*lint !e534*/
}


void *malloc(size_t size) /*lint !e31 !e10*/
{
    void *ptr = NULL; /*lint !e64 !e10*/

    if (size == 0)
        return NULL; /*lint !e64*/

    ptr = osMemoryAlloc(size);

    return ptr;
}


void *zalloc(size_t size) /*lint !e10*/
{
    void *ptr = malloc (size);

    if (ptr != NULL)
    {
        memset((void *)ptr, (int)0, size);
    }

    return ptr;
}


void *calloc(size_t nitems, size_t size) /*lint !e578*/
{
    return zalloc(nitems * size);
}


void *realloc(void *ptr, size_t size)
{
    return osMemoryRealloc((void *)ptr, (size_t)size);
}


void *_malloc_r(struct _reent *ptr, size_t size)
{
    (void) ptr;
    return malloc(size);
}


void *_realloc_r(struct _reent *ptr, void *old, size_t newlen)
{
    (void) ptr;
    return realloc (old, newlen);
}


void *_calloc_r(struct _reent *ptr, size_t size, size_t len)
{
    (void) ptr;
    return calloc(size, len);
}


void _free_r(struct _reent *ptr, void *addr)
{
    (void) ptr;
    free(addr);
}
