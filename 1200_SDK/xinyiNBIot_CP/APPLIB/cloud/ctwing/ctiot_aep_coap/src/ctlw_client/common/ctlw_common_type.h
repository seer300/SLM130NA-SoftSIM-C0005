/****************************************************************************

            (c) Copyright 2019 by 天翼物联科技有限公司. All rights reserved.

****************************************************************************/
#ifndef _CTLW_COMMON_TYPE_H
#define _CTLW_COMMON_TYPE_H

// These macros must exactly match those in the Windows SDK's intsafe.h.

#if !defined(INT8_MIN)
#define INT8_MIN (-127i8 - 1)
#endif
#if !defined(INT16_MIN)
#define INT16_MIN (-32767i16 - 1)
#endif
#if !defined(INT32_MIN)
#define INT32_MIN (-2147483647i32 - 1)
#endif
#if !defined(INT64_MIN)
#define INT64_MIN (-9223372036854775807i64 - 1)
#endif
#if !defined(INT8_MAX)
#define INT8_MAX 127i8
#endif
#if !defined(INT16_MAX)
#define INT16_MAX 32767i16
#endif
#if !defined(INT32_MAX)
#define INT32_MAX 2147483647i32
#endif
#if !defined(INT64_MAX)
#define INT64_MAX 9223372036854775807i64
#endif
#if !defined(UINT8_MAX)
#define UINT8_MAX 0xffui8
#endif
#if !defined(UINT16_MAX)
#define UINT16_MAX 0xffffui16
#endif
#if !defined(UINT32_MAX)
#define UINT32_MAX 0xffffffffui32
#endif
#if !defined(UINT64_MAX)
#define UINT64_MAX 0xffffffffffffffffui64
#endif
#if !defined(INT8_C)
#define INT8_C(x) (x)
#endif
#if !defined(INT16_C)
#define INT16_C(x) (x)
#endif
#if !defined(INT32_C)
#define INT32_C(x) (x)
#endif
#if !defined(INT64_C)
#define INT64_C(x) (x##LL)
#endif
#if !defined(UINT8_C)
#define UINT8_C(x) (x)
#endif
#if !defined(UINT16_C)
#define UINT16_C(x) (x)
#endif
#if !defined(UINT32_C)
#define UINT32_C(x) (x##U)
#endif
#if !defined(UINT64_C)
#define UINT64_C(x) (x##ULL)
#endif
#endif //_CTLW_COMMON_TYPE_H
