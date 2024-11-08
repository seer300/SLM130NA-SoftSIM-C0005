#pragma once
#include "stdint.h"

typedef void (*app_t)();
typedef struct
{
    app_t app_init_entry;
} appRegItem_t;


#define _appRegTable_attr_ __attribute__((unused, section(".appRegTable")))

/**
 * @brief 业务模块的初始化注册接口，最终会在main函数的app_start中
 */
#define application_init(app_init_entry) const appRegItem_t _regAppItem_##app_init_entry _appRegTable_attr_ = {app_init_entry}


void User_Startup_Init();
