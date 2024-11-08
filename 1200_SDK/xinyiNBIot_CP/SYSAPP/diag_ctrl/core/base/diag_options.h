#ifndef DIAG_OPTIONS_H
#define DIAG_OPTIONS_H

#include "diag_config.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#ifndef DIAG_ASSERT
    #define DIAG_ASSERT(x)
#endif

#ifndef DIAG_CRITICAL_DEF
    #define DIAG_CRITICAL_DEF(x)
#endif

#ifndef DIAG_MALLOC
    #define DIAG_MALLOC(size) malloc((size_t) (size))
#endif

#ifndef DIAG_FREE
    #define DIAG_FREE(mem) free((void *) (mem))
#endif

#ifndef DIAG_MEMCPY
    #define DIAG_MEMCPY(dst, src, size) memcpy((void *) (dst), (const void *) (src), (size_t) (size))
#endif

#ifndef DIAG_MEMSET
    #define DIAG_MEMSET(dst, num, size) memset((void *) (dst), (int) (num), (size_t) (size))
#endif

#ifndef DIAG_STRLEN
    #define DIAG_STRLEN(str) strlen((const char *) (str))
#endif

#ifndef DIGA_ATTR_PRINTF
    #define DIGA_ATTR_PRINTF(a, b)
#endif

#ifndef DIAG_TRANSMIT_WITH_DMA
    #define DIAG_TRANSMIT_WITH_DMA 0
#endif

#ifndef DIAG_TRANSMIT_IN_DMA_LIST_MODE
    #define DIAG_TRANSMIT_IN_DMA_LIST_MODE 0
#endif

#ifndef DIAG_DMA_LIST_ONCE_SEND_NUM
    #define DIAG_DMA_LIST_ONCE_SEND_NUM 0
#endif

#ifndef DIAG_MALLOC_RECORD_FILE_LINE
    #define DIAG_MALLOC_RECORD_FILE_LINE 0
#endif

#ifndef DIAG_MAX_OCCUPANCY_SIZE
    #define DIAG_MAX_OCCUPANCY_SIZE 0
#endif

#ifndef DIAG_SIGNAL_RESERVED_SIZE
    #define DIAG_SIGNAL_RESERVED_SIZE 0
#endif

#ifndef DIAG_MAX_OCCUPANCY_NODE
    #define DIAG_MAX_OCCUPANCY_NODE 0
#endif

#ifndef DIAG_ONE_LOG_MAX_SIZE_BYTE
    #define DIAG_ONE_LOG_MAX_SIZE_BYTE 0
#endif

#ifndef DIAG_DUMP_ONE_PACKET_SIZE
    #define DIAG_DUMP_ONE_PACKET_SIZE 128
#endif

#ifndef DIAG_HEART_BEAT_PACKET
    #define DIAG_HEART_BEAT_PACKET 0
#endif

#ifndef DIAG_HEART_VALID_TIME
    #define DIAG_HEART_VALID_TIME 1000
#endif


#ifndef DIAG_ENTER_CRITICAL
    #error "DIAG_ENTER_CRITICAL() must be defined"
#endif

#ifndef DIAG_EXIT_CRITICAL
    #error "DIAG_EXIT_CRITICAL() must be defined"
#endif

#ifndef DIAG_GET_TICK_COUNT
    #error "DIAG_GET_TICK_COUNT() must be defined"
#endif

#if ((DIAG_TRANSMIT_WITH_DMA == 1) && (DIAG_TRANSMIT_IN_DMA_LIST_MODE == 1) && (DIAG_DMA_LIST_ONCE_SEND_NUM <= 1))
    #error "DIAG_DMA_LIST_ONCE_SEND_NUM must be over than 1 if DIAG_TRANSMIT_WITH_DMA and DIAG_TRANSMIT_IN_DMA_LIST_MODE is enabled"
#endif

#if (DIAG_MAX_OCCUPANCY_SIZE == 0)
    #error "DIAG_MAX_OCCUPANCY_SIZE cannot set to 0"
#endif

#if (DIAG_SIGNAL_RESERVED_SIZE == 0)
    #error "DIAG_SIGNAL_RESERVED_SIZE cannot set to 0"
#endif

#if (DIAG_SIGNAL_RESERVED_SIZE >= DIAG_MAX_OCCUPANCY_SIZE)
    #error "DIAG_SIGNAL_RESERVED_SIZE has to be less than DIAG_MAX_OCCUPANCY_SIZE"
#endif

#if (DIAG_MAX_OCCUPANCY_NODE == 0)
    #error "DIAG_MAX_OCCUPANCY_NODE cannot set to 0"
#endif

#if (DIAG_ONE_LOG_MAX_SIZE_BYTE < 16)
    #error "DIAG_ONE_LOG_MAX_SIZE_BYTE cannot set less than 16"
#endif

#if (DIAG_DUMP_ONE_PACKET_SIZE < 64)
    #error "DIAG_DUMP_ONE_PACKET_SIZE cannot set less than 64"
#endif

#if ((DIAG_HEART_BEAT_PACKET == 1) && (DIAG_HEART_VALID_TIME == 0))
    #error "DIAG_HEART_VALID_TIME cannot set to 0 when DIAG_HEART_BEAT_PACKET is enabled"
#endif


#endif  /* DIAG_OPTIONS_H */
