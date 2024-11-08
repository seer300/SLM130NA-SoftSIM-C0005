//modified 2018-03-28 yb
#ifndef _PHY_BASIC_TYPE_H_
#define _PHY_BASIC_TYPE_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>


typedef signed char           int8_t;
typedef signed short          int16_t;
//typedef signed int            int32_t;
typedef signed long long      int64_t;
typedef unsigned char         uint8_t;
typedef unsigned short        uint16_t;
//typedef unsigned int          uint32_t;
typedef unsigned long long    uint64_t;
typedef struct 
{
  int16_t re;
  int16_t im;
}complex16_low_re_t;

typedef struct 
{
  int16_t im;
  int16_t re;
}complex16_low_im_t;

typedef struct Complex8_t
{
  int8_t im;
  int8_t re;
}complex8_t;
typedef struct Complex32_t
{
  int32_t re;
  int32_t im;
}complex32_t;

#ifndef NULL
#define NULL  0
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif


#define PI 3.14159265

/* the local align */
#define ALIGN(x)    __attribute__((aligned(x)))
#define XT_MAX(a,b)     ((a > b) ? a : b)   //max((a),(b))
#define XT_MIN(a,b)   ((a < b)? a : b)
#define PHY_ABS(x) ((x) > 0? (x) : (-(x)))
#define PHY_Filter(newData,OldData,alpha) (((int64_t)(((int64_t)alpha*newData + (int64_t)(32767-alpha)*OldData)+16384))>>15)

#endif //_BASIC_TYPE_H_

