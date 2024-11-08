/*******************************************************************************
File name    : nbiot_ps_export_interface.h
Description  : 
*******************************************************************************/
#ifndef __NBIOT_PS_EXPORT_INTERFACE_H
#define __NBIOT_PS_EXPORT_INTERFACE_H

#include "cmsis_os2.h"

/* Common Message Struct Definition */
typedef struct msg_header_stru
{
	unsigned short                  ulMemSize;                                          /* memory size                              */
	unsigned char                   ulSrcTskId;                                         /* source task ID                           */
	unsigned char                   ulDestTskId;                                        /* destination task ID                      */
	unsigned long                   ulMsgClass;                                         /* message class                            */
	unsigned long                   ulMsgName;                                          /* message name                             */
}MSG_HEADER_STRU;

typedef osMessageQueueId_t MSGQUE_ID;

typedef struct
{
    unsigned long   MsgType;
    unsigned long   MsgPointer;
    //unsigned long Reserve1;
    //unsigned long     Reserve2;
}PS_MSG_FORMAT_TYPE;

extern unsigned short Ps_Get_InVar_F_Sizeof();
extern unsigned short Ps_Get_Var_F_Sizeof();
extern void Ps_Set_ptVar_F();
extern unsigned long Ps_Get_ptVar_F();
extern unsigned long Ps_Get_ptInVar_F();
extern void PsSendPsComExtMsg(const PS_MSG_FORMAT_TYPE *pMsg);
extern unsigned char IsMtNetTest(void);
extern void Ps_SaveInvar_forLpm();
#endif
