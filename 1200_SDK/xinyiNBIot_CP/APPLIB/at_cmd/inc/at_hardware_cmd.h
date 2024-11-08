#pragma once

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/**
  * @brief 芯片当前温度和电池电压
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval int
  * @arg 请求类AT命令：AT+xCHIPINFO=<cmdadc_channel>
  *      @arg <cmd>, ALL:所有数据；TEMP当前温度（摄氏度）；VBAT:电池电压（mv）
  * @arg 查询类AT命令：AT+xCHIPINFO=?
  */
int at_CHIPINFO_req(char *at_buf,char **prsp_cmd);
/**
  * @brief  ADC测试相关的AT命令处理函数
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 执行类AT命令：AT+xADC
  * @arg 测试类AT命令：AT+xADC=?
  * @arg 设置类AT命令：AT+xADC=<channel>
    */
int at_ADC_req(char *at_buf, char **prsp_cmd);

/** 
  * @brief  VBAT电压测试AT命令处理函数，显示结果为VBAT电压（单位mV）
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 测试AT命令：AT+VBAT=?
  */
int at_VBAT_req(char *at_buf, char **prsp_cmd);

/**
  * @brief  +LED   
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 查询类AT命令：AT+*LED*?			 +*LED*: <mode>
  * @arg 测试类AT命令：AT+*LED*=? 		 +*LED*: (0,1)
  * @arg 设置类AT命令：AT+*LED*=<mode>    +*LED*: <mode>
  */

int at_LED_req(char *at_buf, char **prsp_cmd);


/**
  * @brief  引脚测试相关的AT命令处理函数
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 请求类AT命令：AT+XYCNNT=<bit map>
  * @arg 查询类AT命令：AT+XYCNNT?
  * @arg 测试类AT命令：AT+XYCNNT=?
  */
int at_XYCNNT_req(char *at_buf, char **prsp_cmd);


/*BC95硬件相关命令集*/
int at_CHIPINFO_BC95_req(char *at_buf, char **prsp_cmd);
int at_ADC_BC95_req(char *at_buf, char **prsp_cmd);
int at_LED_BC95_req(char *at_buf, char **prsp_cmd);


/*BC25硬件相关命令集*/
int at_ADC_BC25_req(char *at_buf, char **prsp_cmd);
int at_CBC_BC25_req(char *at_buf, char **prsp_cmd);
int at_LED_BC25_req(char *at_buf, char **prsp_cmd);


/*BC260Y硬件相关命令集*/
int at_ADC_260Y_req(char *at_buf, char **prsp_cmd);
int at_CBC_260Y_req(char *at_buf, char **prsp_cmd);

