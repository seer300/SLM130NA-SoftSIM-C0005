#include "user_config.h"
#include "mcu_adapt.h"
#include "xy_system.h"
#include "system.h"
#include "string.h"

/************************************************************************************************************
I2C_EEPROM_TEST
本测试使用XY1200的I2C外设作为主机，AT24C64作为从机，在主函数中依次对AT24C64进行读写测试，单次测试I2C_LEN_MAX字节，并
将测试结果进行打印输出。此测试主要是对mcu_i2c.c中的接口耗时、功耗开销、稳定性、启停CP核的健壮性进行压测。

测试时，需要外接串口进打印测试结果，具体为将GPIO14接串口工具的RX引脚，可与SPI读写NORFLASH同时测试。

测试指令：AT+DEMOCFG=I2C,i2c_num,i2c_freq //选择待测的I2C，以及速率，i2c_num与i2c_freq取值具体参考mcu_i2c.c
         AT+DEMOCFG=BITMAP,8    //SPI单独测试
         AT+DEMOCFG=BITMAP,11   //SPI结合随机起停CP测试
         AT+DEMOCFG=BITMAP,264  //SPI、I2C联合测试
         AT+DEMOCFG=BITMAP,267  //SPI、I2C结合随机起停CP联合测试
*************************************************************************************************************/

uint32_t g_i2c_num = 0; //硬件I2C接口选择，0：选择I2C1进行EEPROM读写测试，1：选择I2C2进行EEPROM读写测试
uint32_t g_i2c_freq = 100; //I2C速率选择，可选100，400

static uint32_t local_test_cnt = 1; //记录I2C_EEPROM测试次数

//该宏定义i2c从机设备地址的最低位为读写控制位，默认为0，
//调用i2c读写接口时，接口内会实现读写控制位的改变，用户无需修改该位
#define E2_DEVICE_ADDR    (0xA0)   //eeprom设备地址
#define E2_LEN_MAX        (1024*8) //eeprom最大存储空间，单位字节
#define E2_PAGE_SIZE      (32)     //eeprom页长度，单位字节，如：AT24C02该宏为8，AT24C64该宏为32
#define E2_DATA_ADDR_SIZE (2)      //eeprom片内数据地址宽度，1或2，如：AT24C02该宏为1，AT24C64该宏为2

//eeprom读写相关宏定义、全局定义、类型定义
#define I2C_LEN_MAX  (512) //单次i2c最大收发长度，单位字节。最小E2_PAGE_SIZE，最大E2_LEN_MAX，且必须为E2_PAGE_SIZE整数倍
#define I2C_WR_TIMES (E2_LEN_MAX / I2C_LEN_MAX) //i2c读写完全部eeprom存储空间需要的次数

static uint8_t g_i2c_tx_buff[I2C_LEN_MAX] = {0}; //单次i2c发送缓存区
static uint8_t g_i2c_rx_buff[I2C_LEN_MAX] = {0}; //单次i2c接收缓存区

static uint16_t g_wr_times = 0; //记录操作e2的次数，不得大于I2C_WR_TIMES

/**
 * @brief 数据准备函数，按照增量依次对每个数组元素赋值，从0赋值到255。
 * 
 * @param length 数据长度
 * @param ptxdata 数据地址
 * @param inc 数据间隔增量，可选1-10
 * @return none 
 */
void PrepareSendData(uint32_t length, uint8_t *ptxdata, uint8_t inc)
{
    uint32_t integer = length / 256;
    uint32_t remainder = length % 256;
    uint32_t data = 0, t_data = 0;

    if(ptxdata == NULL)
    {
        return;
    }

    if(inc > 10 || inc < 1)
    {
        return;
    }

    if(length == 0)
    {
        return;
    }

    //计算首次大于256的数
    for(uint32_t i = 0; i < 256; i++)
    {
        data = i * inc;

        if(data >= 256)
        {
            t_data = data;
            break;
        }
    }

    //根据inc赋值ptxdata
    for(uint32_t i = 0; i < integer; i++)
    {
        for(uint32_t j = 0; j < 256; j++)
        {
            data = j * inc;

            if(data >= t_data)
            {
                data = data % t_data;
            }
            
            ptxdata[i * 256 + j] = (uint8_t)data;
        }
    }
    for(uint32_t i = 0; i < remainder; i++)
    {
        data = i * inc;

        if(data >= t_data)
        {
            data = data % t_data;
        }

        ptxdata[integer * 256 + i] = (uint8_t)data;
    }
}

/**
 * @brief I2C EEPROM按页写数据，顺序读数据，数据长度length
 * 
 * @param num I2C硬件号，可选0-1，对应I2C1、I2C2
 * @param page_addr E2页地址
 * @param length E2读写长度
 * @param ptxdata 写入E2数据缓存区
 * @param prxdata 读取E2数据缓存区
 * @return none 
 */
void e2_page_w_seq_r(uint8_t num,uint32_t freq, uint32_t page_addr, uint32_t length, uint8_t *ptxdata, uint8_t *prxdata)
{
    //初始化i2c接口，400KHz
    McuI2cMasterSet2(num, freq, 36, 35);

    //按页写数据
    uint32_t send_cnt_integer = length / E2_PAGE_SIZE;
    uint32_t send_cnt_remainder = length % E2_PAGE_SIZE;

    for (uint32_t i = 0; i < send_cnt_integer; i++)
    {
        McuI2cMasterWrite(num, E2_DEVICE_ADDR, E2_DATA_ADDR_SIZE, page_addr + (i * E2_PAGE_SIZE), &(ptxdata[i * E2_PAGE_SIZE]), E2_PAGE_SIZE, 50);
        HAL_Delay(5); //i2c eeprom写周期为5ms，必要延时
    }
    McuI2cMasterWrite(num, E2_DEVICE_ADDR, E2_DATA_ADDR_SIZE, page_addr + (send_cnt_integer * E2_PAGE_SIZE), &(ptxdata[send_cnt_integer * E2_PAGE_SIZE]), send_cnt_remainder, 50);
    HAL_Delay(5); //i2c eeprom的写周期为5ms，必要延时

    //顺序读数据
    McuI2cMasterRead(num, E2_DEVICE_ADDR, E2_DATA_ADDR_SIZE, page_addr, prxdata, length, 100);
}

/**
 * @brief I2C EEPROM随机对全片地址读写数据，写入内容随机
 * 
 * 地址循环从0增加到I2C_LEN_MAX*(I2C_WR_TIMES - 1)，每次增加I2C_LEN_MAX
 * 内容为0-255中间的任意值，在此基础上随机减0-10，以构造随机数
 *
 * @return none 
 */
void e2_test(void)
{
    //打印读写地址、读写长度 
    xy_printf("\r\n->Wr_time: No.%4d, PageAddr: 0x%4x, Length: %4d", g_wr_times + 1, g_wr_times * I2C_LEN_MAX, I2C_LEN_MAX);

    //产生1-10之间的随机数
    srand(Get_Tick());
    uint8_t inc = (uint8_t)(rand() % 10 + 1);

    //准备发送数据
    PrepareSendData(I2C_LEN_MAX, g_i2c_tx_buff, inc);

    //I2C_EEPROM读写操作，读写 I2C_LEN_MAX 个数据
    e2_page_w_seq_r(g_i2c_num, g_i2c_freq, g_wr_times * I2C_LEN_MAX, I2C_LEN_MAX, g_i2c_tx_buff, g_i2c_rx_buff);

    //打印发送数据、读到数据
    xy_printf("\r\n->Writing Data:\r\n");
    xy_printf("\r\n->Reading Data:\r\n");

    //比较每次读写数据内容，并打印比较结果
    xy_printf("\r\n->Wr_time: No.%4d, PageAddr: 0x%4x, Length: %4d", g_wr_times + 1, g_wr_times * I2C_LEN_MAX, I2C_LEN_MAX);
    xy_assert(memcmp(g_i2c_rx_buff, g_i2c_tx_buff, I2C_LEN_MAX) == 0);

    //e2读写次数管理
    (g_wr_times == (I2C_WR_TIMES - 1)) ? (g_wr_times = 0) : (g_wr_times++);
}


/**
 * @brief E2读写测试
 * 
 * @return none 
 */

void E2_Work(void)
{
    //对 g_i2c_num 做检查，详见该全局注释
    if(g_i2c_num > 1 || (g_i2c_freq !=100 && g_i2c_freq !=400))
    {
        xy_printf("\r\nerror: g_i2c_num is illegal, %d\r\n", g_i2c_num);
        return;
    }

    // //对 I2C_LEN_MAX 做检查，详见该宏注释
    // if((I2C_LEN_MAX > E2_LEN_MAX) || (I2C_LEN_MAX < E2_PAGE_SIZE) || (I2C_LEN_MAX % E2_PAGE_SIZE))
    // {
    //     xy_printf("\r\nerror: I2C_LEN_MAX is illegal, %d\r\n", I2C_LEN_MAX);
    //     return;
    // }

    //打印I2C_EEPROM读写测试的基本信息，打印读写地址、读写长度 
    // xy_printf("\r\nE2_WORK START");
    // xy_printf("\r\n===============No.%d I2C%d_E2 W&R Test===============", local_test_cnt, g_i2c_num);
    // xy_printf("\r\n->E2_DEVICE_ADDR: 0x%4x", E2_DEVICE_ADDR);
    // xy_printf("\r\n->E2_LEN_MAX: %4dByte", E2_LEN_MAX);
    // xy_printf("\r\n->E2_PAGE_SIZE: %4dByte", E2_PAGE_SIZE);
    // xy_printf("\r\n->E2_DATA_ADDR_SIZE: %4dByte", E2_DATA_ADDR_SIZE);

    //I2C EEPROM随机对全片地址读写数据，写入内容随机
    e2_test();

    //比较每次读写数据内容，并打印比较结果
    xy_printf("\r\n===============No.%d I2C%d_E2 W&R Test===============", local_test_cnt, g_i2c_num);
    xy_printf("\r\nE2_WORK END\r\n");

    //测试次数管理
    (local_test_cnt == 0xFFFFFFFF) ? (local_test_cnt = 1) : (local_test_cnt++);
    
}

