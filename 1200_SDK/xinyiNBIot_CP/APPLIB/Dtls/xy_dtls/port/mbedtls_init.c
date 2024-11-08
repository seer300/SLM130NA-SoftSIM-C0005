#include "xy_utils.h"

uint64_t dtls_gettime_ms(void)
{
    return get_utc_ms();
}

uint32_t dtls_gettime()
{
    return (uint32_t)(get_utc_ms() / 1000);
}

void *dtls_calloc(size_t n, size_t size)
{
    void *p = xy_malloc(n * size);

	if(p != NULL)
	{
		memset(p, 0, n * size);
	}

    return p;
}

void dtls_free(void *ptr)
{
    if(ptr == NULL)
        return;
    
    (void)xy_free(ptr);
}


void dtls_init(void)
{
//   (void)mbedtls_platform_set_calloc_free(dtls_calloc, dtls_free);
   (void)mbedtls_platform_set_snprintf(snprintf);
   (void)mbedtls_platform_set_time(dtls_gettime);
}

