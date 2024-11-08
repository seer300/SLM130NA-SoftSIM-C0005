	/** 
	* @file 	   cdp_init.h
	* @brief	   提供cdp基础功能的初始化，用户可根据需要自行设置cdp配置。
	* @attention
	*/
#ifndef _CDP_INIT_H
#define _CDP_INIT_H
	
	void cdp_user_config_init();
    void cdp_storage_nv_init();
	bool cdp_storage_uicc_isvalid();
#endif
