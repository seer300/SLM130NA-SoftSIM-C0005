#pragma once



int  User_Err_Process(int errno);



/**
 * @brief 强行清空CP相关的所有事件、状态机、缓存等，仅用于用户容错，通常与Force_Stop_CP一起使用
 */
void Clear_Info_of_CP();

/**
 * @brief CP异常的容错处理
 */
void CP_Err_Process(void);


