
#ifndef __CTIOT_HEAP_H__
#define __CTIOT_HEAP_H__

void CtiotFree(void *pv);
void *pCtiotMalloc(size_t xWantedSize);
size_t CtiotGetFreeHeapSize(void);
size_t CtiotGetMinimumEverFreeHeapSize(void);
size_t CtiotGetAllHeapSize(void);

#endif
