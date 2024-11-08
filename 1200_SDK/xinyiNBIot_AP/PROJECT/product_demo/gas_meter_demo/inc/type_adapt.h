
/***********************************Copyright(c)*******************************
** 文   件   名: type_adapt.h
** 说       明: 用于类型适配
** 修   改   人: 无                                                            
** 版       本: 无                                                            
** 日       期: 无                                                            
** 描       述: 无                                                            
**                                                                             
******************************************************************************/

#ifndef TYPE_ADAPT_H__
#define TYPE_ADAPT_H__


#include "system.h"

#if BAN_WRITE_FLASH
#define    __TYPE_IRQ_FUNC   __FLASH_FUNC   //中断代码，放RAM
#else
#define    __TYPE_IRQ_FUNC   __RAM_FUNC     //中断代码，放RAM
#endif

#endif


