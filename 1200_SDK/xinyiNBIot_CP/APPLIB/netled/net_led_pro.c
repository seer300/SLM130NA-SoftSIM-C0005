/**
* @file        net_led_pro.c
* @ingroup     peripheral
* @brief       LED指示灯，指示当前驻网情况
* @attention   1，由于硬件上有些限制，如果要使用，请咨询我们的硬件FAE;
*              2，目前默认未使用该源文件;
* @par	点灯规则：
* @par							亮                        灭
* @par			上电默认注网：     64ms    800ms
* @par			注网成功		   64ms    2000ms
* @par			idle		   0ms	   ~ms
* @par			PSM			   0ms	   ~ms
* @par			PDP deact      64ms    800ms
* @par			PDP act		   64ms    2000ms
***********************************************************************************/
#include "gpio.h"
//#include "os_extend.h"
#include "xy_ps_api.h"
#include "xy_atc_interface.h"

typedef enum
{
	LED_ON = 0,
	LED_SLOWFLICKER,   //慢速闪烁
    LED_FASTFLICKER,   //快速闪烁
    LED_STOP, 
} net_led_status;


osTimerId_t net_led_timer = NULL;
osThreadId_t g_user_led_Handle = NULL;
osMessageQueueId_t at_led_q = NULL;


struct led_msg {
	int		msg_id;
};
static uint8_t g_ledcalback_count = 0;
static uint16_t g_ledon_time, g_ledoff_time,g_led_on_only = 0;


void logT(int id,char *msg)
{
    char* debug = xy_malloc(128);
	
	memset(debug,0,128);
    snprintf(debug, 128, "\r\nlog:%s/%d\r\n",msg,id);
    write_to_at_uart(debug, strlen(debug));
    xy_free(debug);
}

void net_led_timer_calback(void)//net_led_timer calback
{	
	if (g_led_on_only==1) {
		//长亮
		GPIO_WritePin(g_softap_fac_nv->led_pin, SET);
		osTimerStart(net_led_timer, g_ledon_time);
		return;
	}

	g_ledcalback_count++;	
	if(g_ledcalback_count%2)
	{
		GPIO_WritePin(g_softap_fac_nv->led_pin, RESET);
		osTimerStart(net_led_timer, g_ledoff_time);
	}
	else
	{		
		GPIO_WritePin(g_softap_fac_nv->led_pin, SET);
		osTimerStart(net_led_timer, g_ledon_time);
	}
	if(g_ledcalback_count == 2)
		g_ledcalback_count =0;
	
}



void net_led_light()
{
	osTimerAttr_t timer_attr = {0};

	GPIO_WritePin(g_softap_fac_nv->led_pin, RESET);
	g_ledcalback_count = 0;
	
	if(net_led_timer == NULL)
	{
		timer_attr.name = "led_tmr1";
		net_led_timer = osTimerNew((osTimerFunc_t)(net_led_timer_calback), osTimerOnce, NULL, &timer_attr);
		osTimerStart(net_led_timer, g_ledon_time);
		
	}
	else
	{
		osTimerStart(net_led_timer, g_ledon_time);
	}
}

void net_led_stop(void)
{		
	GPIO_WritePin(g_softap_fac_nv->led_pin, RESET);

	if(net_led_timer != NULL)
	{
		osTimerDelete(net_led_timer);
		net_led_timer = NULL;
	}	
}


int write_led_pro(net_led_status msg_type)
{
	struct led_msg *msg =NULL;
	
	msg = xy_malloc(sizeof(struct led_msg));
	msg->msg_id = msg_type;

	osMessageQueuePut(at_led_q, &msg, 0, osWaitForever);
	return 1;
	
}

void user_led_pin_init(void)
{
	GPIO_InitTypeDef gpio_init = {0};

	gpio_init.Pin = g_softap_fac_nv->led_pin;
	gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_init.Pull = GPIO_FLOAT;
	GPIO_Init(&gpio_init);

	GPIO_WritePin(g_softap_fac_nv->led_pin, RESET);
}

void net_led_task()
{
	struct led_msg *rcv_msg = NULL;	

	write_led_pro(LED_FASTFLICKER);//初始状态为未驻网

	while(1)
	{
		osMessageQueueGet(at_led_q, &rcv_msg, NULL, osWaitForever);

		xy_printf(0,XYAPP, WARN_LOG, "led net is %d\r\n",rcv_msg->msg_id);

		
		switch (rcv_msg->msg_id)
		{
		case LED_SLOWFLICKER:
			g_ledon_time=64; g_ledoff_time=2000;
			net_led_light();
			break;
		
		case LED_FASTFLICKER:
			g_ledon_time=64; g_ledoff_time=800;
			net_led_light();
			break;
		case LED_ON:
			g_led_on_only = 1;
			net_led_light();
			break;
		case LED_STOP:
		default:
			net_led_stop();
			g_ledcalback_count = 0;
			break;
		}

		xy_free(rcv_msg);		
	}
}



//+CGEV: ME PDN DEACT 0
void led_urc_CGEV_Callback(unsigned long eventId, void *param, int paramLen)
{
	UNUSED_ARG(eventId);
	xy_assert(paramLen == sizeof(ATC_MSG_CGEV_IND_STRU));
	ATC_MSG_CGEV_IND_STRU *cgev_urc = (ATC_MSG_CGEV_IND_STRU*)param;

	//logT(cgev_urc->ucCgevEventId,"CGEV in");
	switch(cgev_urc->ucCgevEventId)
	{
		case D_ATC_CGEV_ME_PDN_ACT:   
		case D_ATC_CGEV_IS:
			write_led_pro(LED_FASTFLICKER);
			break;

		case D_ATC_CGEV_NW_PDN_DEACT:  
		case D_ATC_CGEV_ME_PDN_DEACT:  
		case D_ATC_CGEV_OOS:
			write_led_pro(LED_STOP);
			break;

		default:
			write_led_pro(LED_STOP);
			break;
	}
}

void led_urc_CSCON_Callback(unsigned long eventId, void *param, int paramLen)
{
	UNUSED_ARG(eventId);
	xy_assert(paramLen == sizeof(ATC_MSG_CSCON_IND_STRU));
	ATC_MSG_CSCON_IND_STRU *cscon_urc = (ATC_MSG_CSCON_IND_STRU*)param;

	/*
		信令状态: 1连接,0空闲
	 */
	//logT(cscon_urc->stPara.ucMode,"CSCON in");

	if(cscon_urc->stPara.ucMode == 1) {
		write_led_pro(LED_ON);
		g_led_on_only = 1;
	}else {
		write_led_pro(LED_SLOWFLICKER);
		g_led_on_only = 0;
	}
}



/**
 * @brief 用户任务初始化函数，在user_task_init中添加
 * @attention   
 */	
//if((g_softap_fac_nv != NULL) && (g_softap_fac_nv->led_pin <= GPIO_PAD_NUM_63))
void net_led_init(void)
{
	osThreadAttr_t thread_attr = {0};
	
	user_led_pin_init();	

	if(at_led_q == NULL)
		at_led_q = osMessageQueueNew(20, sizeof(void *), NULL);

	xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_CGEV, led_urc_CGEV_Callback);//网络状态
	xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_CSCON, led_urc_CSCON_Callback);//信令连接状态

	
	if(g_user_led_Handle == NULL)
	{
		thread_attr.name	   = "user_led_demo";
		thread_attr.priority   = osPriorityNormal;
		thread_attr.stack_size = 0x400;
		g_user_led_Handle = osThreadNew ((osThreadFunc_t)(net_led_task),NULL,&thread_attr);
		diag_port_send_log_directly("user_led_demo create",strlen("user_led_demo create"));
	}

}


