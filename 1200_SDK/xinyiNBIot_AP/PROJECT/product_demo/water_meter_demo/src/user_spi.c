#include "hal_gpio.h"
#include "hal_def.h"
#include "hw_gpio.h"
#include "user_gpio.h"
#include "hal_spi.h"
#include "user_spi.h"
#include "user_time.h"


HAL_SPI_HandleTypeDef spi_master = {0};

__RAM_FUNC void User_SPI_Init(void)
{
    HAL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = USER_SPI_MOSI;
    GPIO_InitStruct.Mode = GPIO_MODE_HW_PER;
    GPIO_InitStruct.PinRemap = GPIO_SPI_MOSI;
    HAL_GPIO_Init(&GPIO_InitStruct);

    GPIO_InitStruct.Pin = USER_SPI_MISO;
    GPIO_InitStruct.Mode = GPIO_MODE_HW_PER;
    GPIO_InitStruct.PinRemap = GPIO_SPI_MISO;
    HAL_GPIO_Init(&GPIO_InitStruct);

    GPIO_InitStruct.Pin = USER_SPI_CLK;
    GPIO_InitStruct.Mode = GPIO_MODE_HW_PER;
    GPIO_InitStruct.PinRemap = GPIO_SPI_SCLK;
    HAL_GPIO_Init(&GPIO_InitStruct);


    GPIO_InitStruct.Pin = USER_SPI_CS;
    GPIO_InitStruct.Mode = GPIO_MODE_HW_PER;
    GPIO_InitStruct.PinRemap = GPIO_SPI_SS_N;
    HAL_GPIO_Init(&GPIO_InitStruct);

    //spi 初始化
    spi_master.Instance = HAL_SPI;
    spi_master.Init.MasterSlave = HAL_SPI_MODE_MASTER;
    spi_master.Init.WorkMode = HAL_SPI_WORKMODE_0;
    if(GetAPClockFreq() > 36000000)//防止stopcp但AP侧未睡眠的情况
    {
        spi_master.Init.Clock_Prescaler = HAL_SPI_CLKDIV_64;
    }
    else
    {
#if (RC26MCLKDIV == 4)     
        spi_master.Init.Clock_Prescaler = HAL_SPI_CLKDIV_4;//维持测试主频频率与客户接近 
#elif(RC26MCLKDIV == 2)
        spi_master.Init.Clock_Prescaler = HAL_SPI_CLKDIV_8;
#elif(RC26MCLKDIV == 1)
        spi_master.Init.Clock_Prescaler = HAL_SPI_CLKDIV_16;
#endif        
    }
    HAL_StatusTypeDef ret = HAL_SPI_Init(&spi_master);

    if(ret != HAL_OK)
    {
        xy_assert(0);
    }
}

__RAM_FUNC void User_SPI_Deinit(void)
{
    HAL_SPI_DeInit(&spi_master);
}

#if AUL_TEST

//CD offset
int16_t s16CD1_offset = 0;
int16_t s16CD2_offset = 0;
int16_t s16CD1=0; 
int16_t s16CD2=0; 
uint32_t faultcnt=0;
uint32_t successcnt=0;    //定义两个变量记录SPI通讯成功与失败次数 

__RAM_FUNC void bsp_spi_delay(void)
{
    uint16_t i;
    for(i = 0; i < 2; i++)
    {
        __NOP();
    }
}

__RAM_FUNC void bsp_spi_write_reg(uint8_t address, uint8_t data)
{
    HAL_StatusTypeDef ret = HAL_OK;
    uint8_t sendData[2]={0};

    sendData[0] = (address | AU20xx_CMD_WRITE);
    sendData[1] = data;

    SPI_CS_OUTPUT_LOW;
    ret = HAL_SPI_Transmit(&spi_master, sendData, 2, 100);
    if(ret != HAL_OK)
    {
        xy_assert(0);          
    }
    SPI_CS_OUTPUT_HIGH;
}

__RAM_FUNC void bsp_spi_multi_read(uint8_t* p_addr, uint8_t* p_data, uint8_t cnt)
{
    HAL_StatusTypeDef ret = HAL_OK;
    SPI_CS_OUTPUT_LOW;
    ret = HAL_SPI_Master_TransmitReceive(&spi_master, p_addr, p_data, cnt, 100);
    if(ret != HAL_OK)
    {
        xy_assert(0); 
    }
    SPI_CS_OUTPUT_HIGH;
}

__RAM_FUNC void set_sensor_offset(void)
{
    bsp_spi_write_reg(AU2001_SNS1_OFF0, s16CD1_offset & 0xFF);
    bsp_spi_write_reg(AU2001_SNS1_OFF1, (s16CD1_offset >> 8) & 0xFF);
    bsp_spi_write_reg(AU2001_SNS2_OFF0, s16CD2_offset & 0xFF);
    bsp_spi_write_reg(AU2001_SNS2_OFF1, (s16CD2_offset >> 8) & 0xFF);
}

__RAM_FUNC void testspi(void)
{
    unsigned char u8ReadRegTab1[6] = {AU2001_SNS1_OFF0 | AU20xx_CMD_READ, AU2001_SNS1_OFF1 | AU20xx_CMD_READ, \
                                               AU2001_SNS2_OFF0 | AU20xx_CMD_READ, AU2001_SNS2_OFF1 | AU20xx_CMD_READ, \
                                               0, 0};           // 0x00 is dummy address to read the register specified before
	unsigned char u8RegDataTab1[6] = {0};// [0] is invalid data

      
    bsp_spi_write_reg(AU2001_SNS1_OFF0, 0xa5);   //修改2001寄存器AU2001_SNS1_OFF0的值为A5
    bsp_spi_write_reg(AU2001_SNS1_OFF1, 0x5a);   //修改2001寄存器AU2001_SNS1_OFF1的值为5A
    bsp_spi_write_reg(AU2001_SNS2_OFF0, 0xa5);   //修改2001寄存器AU2001_SNS2_OFF0的值为A5
    bsp_spi_write_reg(AU2001_SNS2_OFF1, 0x5a);   //修改2001寄存器AU2001_SNS2_OFF1的值为5A
    
	bsp_spi_multi_read(u8ReadRegTab1,u8RegDataTab1,5);   //读取AU2001_SNS1_OFF0、读取AU2001_SNS1_OFF1、读取AU2001_SNS2_OFF0、读取AU2001_SNS2_OFF1的值
		
	if(u8RegDataTab1[1]==0xa5 && u8RegDataTab1[2]==0x5a && u8RegDataTab1[3]==0xa5 && u8RegDataTab1[4]==0x5a) // 判断寄存器值是否都成功
	{
		successcnt++;       //寄存器值修改成功次数+1
	}
	else
	{
		faultcnt++;         //寄存器值修改成功次数+1
        LEDGON();
        LEDRON();
        xy_assert(0);
	}
	set_sensor_offset();    //将原本的AU2001_SNS1_OFF0、AU2001_SNS1_OFF1、AU2001_SNS2_OFF0、AU2001_SNS2_OFF1值还原
}

__RAM_FUNC void User_SPI_Func(void)
{
    User_SPI_Deinit();
    User_SPI_Init();
    testspi();
}

#else
//W25Q64指令
#define Write_En        (0x06) //写使能
#define Write_Dis       (0x04) //写使能
#define Read_Status_Reg (0x05) //读状态寄存器
#define Sector_Erase    (0x20) //扇区擦除
#define Page_Program    (0x02) //页编程
#define Read_Data       (0x03) //读数据
#define SPI_PAGE_SIZE   (256)  //页长度，单位字节

uint8_t spi_instr[4] = {0};
uint8_t spi_regval[4] = {0};
uint16_t g_flash_protect = 0;

__RAM_FUNC uint8_t Read_Reg_Staus(void)
{
    HAL_StatusTypeDef ret = HAL_OK;

    spi_instr[0] = Read_Status_Reg;

    SPI_CS_OUTPUT_LOW;

    ret = HAL_SPI_Master_TransmitReceive(&spi_master,spi_instr,spi_regval,2,500);

    SPI_CS_OUTPUT_HIGH;

    if(ret != HAL_OK)
    {
        xy_assert(0);
    }
    return spi_regval[1];
}

__RAM_FUNC void Check_Write(void)
{
    while(Read_Reg_Staus() & 0x03);

    HAL_Delay(1);
}

__RAM_FUNC void Check_Busy(void)
{   
    while(Read_Reg_Staus() & 0x01);

    HAL_Delay(10);
}

__RAM_FUNC void Wait_Write_Enable(void)
{
    while((Read_Reg_Staus() & 0x02) == 0);

    HAL_Delay(1);
}

__RAM_FUNC void Write_Enable(void)
{
    HAL_StatusTypeDef ret = HAL_OK;
    //写使能
    spi_instr[0] = Write_En;  

    SPI_CS_OUTPUT_LOW;
    ret = HAL_SPI_Transmit(&spi_master, spi_instr, 1, 500);
    if(ret != HAL_OK)
    {
   	    xy_assert(0);
    }
    SPI_CS_OUTPUT_HIGH;
    HAL_Delay(1);
}

__RAM_FUNC void Write_Disable(void)
{
    HAL_StatusTypeDef ret = HAL_OK;
    //写使能
    spi_instr[0] = Write_Dis;  

    SPI_CS_OUTPUT_LOW;
    ret = HAL_SPI_Transmit(&spi_master, spi_instr, 1, 500);
    if(ret != HAL_OK)
    {
   	    xy_assert(0);
    }
    SPI_CS_OUTPUT_HIGH;
    HAL_Delay(1);
}

__RAM_FUNC void Sector_erase(void)
{
    HAL_StatusTypeDef ret = HAL_OK;  
    //扇区擦除
    uint32_t flash_addr = g_flash_protect * SPI_PAGE_SIZE;
    spi_instr[0] = Sector_Erase;
    spi_instr[1] = ((uint8_t)(flash_addr & 0xFF));
    spi_instr[2] = ((uint8_t)((flash_addr & 0xFF00) >> 8));
    spi_instr[3] = ((uint8_t)((flash_addr & 0xFF0000) >> 16));

    SPI_CS_OUTPUT_LOW;
    ret = HAL_SPI_Transmit(&spi_master,spi_instr,4,500);
    if(ret != HAL_OK)
    {
        xy_assert(0);
    }
    SPI_CS_OUTPUT_HIGH;  
    HAL_Delay(1);  
}

__RAM_FUNC void Read_Data_By_Page(uint8_t *data, uint8_t size)
{
    HAL_StatusTypeDef ret = HAL_OK;  
    uint8_t p_send_packet[256] = {0};
    uint8_t p_recv_packet[260] = {0};

    uint32_t flash_addr = g_flash_protect * SPI_PAGE_SIZE;
    memset(p_send_packet,0xff,256);
    p_send_packet[0] = Read_Data;
    p_send_packet[1] = ((uint8_t)(flash_addr & 0xFF));
    p_send_packet[2] = ((uint8_t)((flash_addr & 0xFF00) >> 8));
    p_send_packet[3] = ((uint8_t)((flash_addr & 0xFF0000) >> 16));
    
    Check_Busy();
    
    SPI_CS_OUTPUT_LOW;

    HAL_SPI_Master_TransmitReceive(&spi_master,p_send_packet,p_recv_packet,(4 + size),500);

    memcpy(data,&p_recv_packet[4],size);

    if(ret != HAL_OK)
    {
        xy_assert(0);
    }
    SPI_CS_OUTPUT_HIGH; 
}

__RAM_FUNC void Write_Data_By_Page(uint8_t *data, uint8_t size)
{
    HAL_StatusTypeDef ret = HAL_OK;  
    uint8_t p_send_packet[256] = {0};

    //读状态寄存器，等待不busy、写禁止
    Check_Write();
    //写使能
    Write_Enable();
    //读状态寄存器，等待写使能  
    Wait_Write_Enable();

    //页编程操作数据包包头
    uint32_t flash_addr = g_flash_protect * SPI_PAGE_SIZE;
    p_send_packet[0] = Page_Program;
    p_send_packet[1] = ((uint8_t)(flash_addr & 0xFF));
    p_send_packet[2] = ((uint8_t)((flash_addr & 0xFF00) >> 8));
    p_send_packet[3] = ((uint8_t)((flash_addr & 0xFF0000) >> 16));
    memcpy(&p_send_packet[4], data, size);

    SPI_CS_OUTPUT_LOW;

    ret = HAL_SPI_Transmit(&spi_master,p_send_packet,(4 + size),500);
    if(ret != HAL_OK)
    {
        xy_assert(0);
    }

    SPI_CS_OUTPUT_HIGH; 
    HAL_Delay(1);
}

//写入当前的月 日 时 分 秒 并读出，验证，置入全局供AT命令查询
__RAM_FUNC void User_SPI_Func(void)
{
    User_SPI_Init();
    //读状态寄存器，等待不busy，写禁止
    Check_Write();
    //写使能
    Write_Enable();
    //读状态寄存器，等待写使能  
    Wait_Write_Enable();
    //扇区擦除
    Sector_erase();

    Write_Data_By_Page((uint8_t *)&Time,40);
    //打开写保护
    Write_Disable();

    Read_Data_By_Page((uint8_t *)&spi_test_time,40);

    if(memcmp(&Time, &spi_test_time, 40) != 0)
    {
        xy_assert(0);
    }

    g_flash_protect++;

    if(g_flash_protect > 511)
    {
        g_flash_protect = 0;
    }

    User_SPI_Deinit();
}
#endif