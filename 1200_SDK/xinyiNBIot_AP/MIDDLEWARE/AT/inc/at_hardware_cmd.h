
#pragma once
#include <stdint.h>

extern uint8_t g_baudrate_flag;
#if VER_BC95
void natspeed_succ_hook();
//易变NV，AP侧无易变nv相关结构体，此地址对应于CP侧g_softap_var_nv中的at_parity,用户不可修改   
#define  SOFT_VAR_NV_ATPARITY                RAM_NV_VOLATILE_SOFTAP_START+43   
#endif

/**
  * @brief  AT串口配置AT命令
  * @param  param:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 请求类AT命令：AT+NATSPEED=<baud_rate>[,<timeout>,[<store>,[<sync_mode>[,<stopbits>]]]]
  *      @arg <baud_rate>,可配置波特率（必选参数）：2400,4800,9600,57600,115200,230400,460800,921600
  *      @arg <timeout>,超时时间,不支持,默认填为0。
  *      @arg <store>,是否存储在NV中，1存储，0不存储。默认为1：存储。
  *      @arg <sync_mode>,同步模式,只支持[0]，0表示正常采样，不支持提前或稍后采样。
  *      @arg <stopbits>,停止位，只支持[2],2表示两个停止位。
  * @arg 查询类AT命令：AT+NATSPEED?
  * @arg 测试类AT命令：AT+NATSPEED=?
  * 
  * @attention 波特率大于9600则关闭standby,小于等于9600则打开standby。
  */
int at_NATSPEED_req(char  *param, char **rsp_cmd);


/**
 * @brief  AT串口配置(波特率)
 * @param  param:
 * @param  prsp_cmd:
 * @retval AT_END
 * @arg 请求类AT命令：AT+IPR=<baud_rate>
 *      @arg <baud_rate>,可配置波特率：0,4800,9600,19200,38400,57600,115200,230400,460800,921600;0表示启用波特率自适应
 *		 若设置的波特率高于9600，则同时修改NV关闭standby
 * @arg 查询类AT命令：AT+IPR?
 * @arg 测试类AT命令：AT+IPR=?
 * 
 * @attention 波特率大于9600则关闭standby,小于等于9600则打开standby。
 */
int at_IPR_req(char  *param, char **rsp_cmd);

/**
  * @brief  AT串口配置(波特率（必选），是否存储在NV中（可选），是否打开standby（可选）)
  * @param  param:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 请求类AT命令：AT+UARTSET=<baud_rate>[,<store>[,<open standby>]]
  *      @arg <baud_rate>,可配置波特率（必选）：2400,4800,9600,57600,115200,230400,460800,921600
  *      @arg <store>,是否动态波特率，默认值为0，即动态切换波特率。
  * 				1：表示固定波特率保存到NV中；
  * 				0：表示动态波特率生效，需要对方同步切换波特率。
  *      @arg <open standby>,是否打开standby睡眠，由于大于9600波特率时，standby会导致脏数据，故不建议客户使用此参数，维持默认设置即可。
  * 				1：表示启动standby睡眠机制；
  * 				0：表示波特率大于9600则关闭standby,小于等于9600则打开standby。
  * @arg 查询类AT命令：AT+UARTSET?
  * @arg 测试类AT命令：AT+UARTSET=?  
  *  
  */
int at_UARTSET_req(char  *param, char **rsp_cmd);

/**
 * @brief 按键生效时长命令
 * 
 * 请求命令：AT+RESETCTL=<mode>
 * 当LPUART_RXD_RESET_MUX为0时，mode可取0~11中任意值
 *          @arg <mode>:1，按键高电平大于10ms唤醒，大于320ms复位
 *          @arg <mode>:2，按键高电平大于10ms唤醒，大于1.28s复位
 *          @arg <mode>:3，按键高电平大于10ms唤醒，大于2.56s复位
 * 			    @arg <mode>:4，按键高电平大于10ms唤醒，大于5.12s复位
 *          @arg <mode>:5，按键高电平大于20ms唤醒，大于320ms复位
 *          @arg <mode>:6，按键高电平大于20ms唤醒，大于1.28s复位
 *          @arg <mode>:7，按键高电平大于20ms唤醒，大于2.56s复位
 * 			    @arg <mode>:8，按键高电平大于20ms唤醒，大于5.12s复位
 *          @arg <mode>:9，按键高电平大于160ms唤醒，大于320ms复位
 *          @arg <mode>:10，按键高电平大于160ms唤醒，大于1.28s复位
 *          @arg <mode>:11，按键高电平大于160ms唤醒，大于2.56s复位
 * 			    @arg <mode>:0，按键高电平大于160ms唤醒，大于5.12s复位，NV默认值
 * 当LPUART_RXD_RESET_MUX为1时，mode可取0~3中任意值
 *          @arg <mode>:2，按键低电平大于320ms复位
 *          @arg <mode>:3，按键低电平大于1.28s复位
 *          @arg <mode>:0，按键低电平大于2.56s复位，NV默认值
 * 			    @arg <mode>:1，按键低电平大于5.12s复位
 * 执行命令：AT+RESETCTL
 * 查询命令：AT+RESETCTL?，查询当前设定档位
 * 测试命令：AT+RESETCTL?，查询所有可设定档位
 * 
 */
int at_RESETCTL_req(char *param, char **rsp_cmd);

/**
  * @brief  引脚测试相关的AT命令处理函数
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 请求类AT命令：AT+XYCNNT=<bit map>
  * @arg 查询类AT命令：AT+XYCNNT?
  * @arg 测试类AT命令：AT+XYCNNT=?
  * @attention 此指令仅限于示例用，用户根据具体使用的GPIO自行调整维护。
  */
int at_XYCNNT_req(char *param, char **rsp_cmd);
