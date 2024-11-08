#include "app_init.h"




void test(void)
{


}


/*section方式定义用户的初始化函数，最终在main入口被依次执行*/
application_init(test);


/*section方式执行用户定义的初始化函数，即application_init()定义的函数*/
void User_Startup_Init()
{
    extern uint8_t *__appRegTable_start__; //定义在sections.ld文件
    extern uint8_t *__appRegTable_end__; //定义在sections.ld文件
    appRegItem_t *appRegTable_start = (appRegItem_t *)&__appRegTable_start__;
    appRegItem_t *appRegTable_end = (appRegItem_t *)&__appRegTable_end__;

    appRegItem_t *cur = appRegTable_start;
    while (cur < appRegTable_end)
    {
        cur->app_init_entry();
        cur += 1;
    }
}