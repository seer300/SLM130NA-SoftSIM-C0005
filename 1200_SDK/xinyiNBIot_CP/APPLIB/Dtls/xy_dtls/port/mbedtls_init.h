#ifndef MBEDTLS_INIT_H
#define MBEDTLS_INIT_H

#include <stdint.h>

uint32_t dtls_gettime();

void *dtls_calloc(size_t n, size_t size);

void dtls_free(void *ptr);

void dtls_init(void);

#endif/*MBEDTLS_INIT_H*/

