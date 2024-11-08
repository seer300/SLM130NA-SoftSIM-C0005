
#ifndef __CTLW_HEAP_TRACE_H__
#define __CTLW_HEAP_TRACE_H__
#include <stdint.h>

void ctiot_print_trace(void);
void *ctiot_malloc_trace(size_t size, const char *file, const char *function, uint32_t lineno);
void ctiot_free_trace(void *p, const char *file, const char *function, uint32_t lineno);
char *ctiot_strdup_trace(const char *str, const char *file, const char *function, uint32_t lineno);

#endif
