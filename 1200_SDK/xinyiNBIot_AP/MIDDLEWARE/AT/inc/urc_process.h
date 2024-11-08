/*
 * @file	urc_process.h
 * @brief	通过接收URC主动上报的框架文件，包括匹配注册上报的接口和执行上报回调的接口；
*/
#pragma once

//用户注册的上报回调，需添加至urc_reg_list链表
extern void URC_SIMST_Proc(char *paramlist);


extern void URC_NNMI_Proc(char *paramlist);
extern void URC_CTLWRECV_Proc(char *paramlist);

extern void URC_MIPLREAD_Proc(char *paramlist);
extern void URC_MIPLWRITE_Proc(char *paramlist);
extern void URC_MIPLEXECUTE_Proc(char *paramlist);
extern void URC_MIPLPARAMETER_Proc(char *paramlist);

extern void URC_NSOCLI_Proc(char *paramlist);
extern void URC_NSONMI_Proc(char *paramlist);

extern void URC_QMTRECV_Proc(char *paramlist);
extern void URC_QMTSTAT_Proc(char *paramlist);


/**
  * @brief  匹配注册上报的接口
  * @param  at_buf 从核间AT通道接收的字符串
  * @return 匹配结果，0:失败，1: 成功。
*/
int Match_URC_Cmd(char *at_buf);

/**
  * @brief  该接口会执行收到的上报的回调,在main函数while循环内部调用。
*/
void CP_URC_Process();

