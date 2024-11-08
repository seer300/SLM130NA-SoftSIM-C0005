#pragma once

#include <stdint.h>


typedef void (*Sys_Func_Cb)(void);

void regist_system_callback(void);


/**
  * @brief 启动阶段在at_ctl()里上报系统URC，如POWERON/NPSMR
  * @note 该接口内部可做定制，如移远上报Neul
  */
extern Sys_Func_Cb p_SysUp_URC_Hook;
void Sys_Up_URC_Regist(Sys_Func_Cb pfun);


/**
  * @brief 深睡之前，用户可以在此函数中定制上报的URC信息
  * @note  该接口运行在idle线程中，如果深睡过程中又来唤醒中断，可能被执行多次
  */
extern Sys_Func_Cb p_SysDown_URC_Hook;
void Sys_Down_URC_Regist(Sys_Func_Cb pfun);


/**
 * @brief  即将进入DEEPSLEEP睡眠时的回调注册。
 * @note   因为深睡时SRAM会下电，可用于深睡前的RAM上关键信息的保存，         也可以进行GPIO翻转等配置。
 * @warning 由于中断打断深睡，可能多次进入！！！
 */
extern Sys_Func_Cb p_Into_DeepSleep_Cb;
void DeepSleep_Before_Regist(Sys_Func_Cb pfun);

void Sys_Down_URC_95(void);
void Sys_Up_URC_95();

void Sys_Down_URC_260(void);
void Sys_Up_URC_260();

void Sys_Down_URC_25(void);
void Sys_Up_URC_25();

void Sys_Down_URC_default(void);
void Sys_Up_URC_default();


