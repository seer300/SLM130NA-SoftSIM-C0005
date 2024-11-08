/**
 * @file 
 * @brief 
 * @version 1.0
 * @date 2023-08-04
 * @copyright Copyright (c) 2023  芯翼信息科技有限公司
 * 
 */

#include "xy_system.h"
#include "softap_nv.h"




/*启动阶段在at_ctl()里上报系统URC，如POWERON/NPSMR*/
Sys_Func_Cb p_SysUp_URC_Hook = NULL;

void Sys_Up_URC_Regist(Sys_Func_Cb pfun)
{
	p_SysUp_URC_Hook = pfun;
}


/*该接口运行在idle线程中，如果深睡过程中又来唤醒中断，可能被执行多次*/
Sys_Func_Cb p_SysDown_URC_Hook = NULL;

void Sys_Down_URC_Regist(Sys_Func_Cb pfun)
{
	p_SysDown_URC_Hook = pfun;
}




/*DeepSleep睡眠前调用;由于中断打断深睡，可能多次进入*/
Sys_Func_Cb p_Into_DeepSleep_Cb = NULL;
void DeepSleep_Before_Regist(Sys_Func_Cb pfun)
{
	p_Into_DeepSleep_Cb = pfun;
}

void regist_system_callback(void)
{
#if 0//VER_BC95
	Sys_Up_URC_Regist(Sys_Up_URC_95);
	Sys_Down_URC_Regist(Sys_Down_URC_95);
#elif VER_260Y
	Sys_Up_URC_Regist(Sys_Up_URC_260);
	Sys_Down_URC_Regist(Sys_Down_URC_260);
#elif VER_BC25
	Sys_Up_URC_Regist(Sys_Up_URC_25);
	Sys_Down_URC_Regist(Sys_Down_URC_25);
#else
	Sys_Up_URC_Regist(Sys_Up_URC_default);
	Sys_Down_URC_Regist(Sys_Down_URC_default);
#endif
}
