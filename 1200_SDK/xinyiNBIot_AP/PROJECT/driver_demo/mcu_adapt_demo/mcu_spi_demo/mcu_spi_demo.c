#include "mcu_adapt.h"
#include "xy_system.h"
#include "system.h"

#define SPI_LEN_MAX (1024)  //spi最大收发长度，单位字节
#define SPI_PAGE_SIZE (256) //页长度，单位字节

//W25Q64指令
#define Write_Enable    (0x06) //写使能
#define Read_Status_Reg (0x05) //读状态寄存器
#define Sector_Erase    (0x20) //扇区擦除
#define Page_Program    (0x02) //页编程
#define Read_Data       (0x03) //读数据

uint8_t spi_instr[4] = {0};
uint8_t spi_regval[4] = {0};
uint8_t *p_send_packet = NULL;
uint8_t *p_recv_packet = NULL;
uint8_t spi_tx_buff[SPI_LEN_MAX] = {0};

//SPI类型选择，0：csp1_spi 1:spi
uint8_t g_spi_num = 0;
//频率需要根据相应spi类型配置
uint32_t g_spi_freq = 1000;
//工作模式
uint32_t g_spi_workmode = 0;



__RAM_FUNC void Register_check_write(void)
{
    //读状态寄存器，等待不busy、写禁止
    McuSpiCsReset(g_spi_num);
    do{
        spi_instr[0] = Read_Status_Reg;
        McuSpiMasterWriteRead(g_spi_num, spi_instr, spi_regval, 2);
    } while (spi_regval[1] & 0x03);
    McuSpiCsSet(g_spi_num);

    HAL_Delay(1);
}
__RAM_FUNC void Register_check_busy(void)
{
    //读状态寄存器，等待不busy
    McuSpiCsReset(g_spi_num);
    do{
        spi_instr[0] = Read_Status_Reg;
        McuSpiMasterWriteRead(g_spi_num, spi_instr, spi_regval, 2);
    } while (spi_regval[1] & 0x01);
    McuSpiCsSet(g_spi_num);

    HAL_Delay(10);
}
__RAM_FUNC void Write_enable(void)
{
    //写使能
    spi_instr[0] = Write_Enable;

    McuSpiCsReset(g_spi_num);
    McuSpiMasterWrite(g_spi_num, spi_instr, 1);
    McuSpiCsSet(g_spi_num);

    HAL_Delay(1);
}
__RAM_FUNC void Wait_write_enabled(void)
{
    //读状态寄存器，等待写使能  
    McuSpiCsReset(g_spi_num);
    do{
        spi_instr[0] = Read_Status_Reg;
        McuSpiMasterWriteRead(g_spi_num, spi_instr, spi_regval, 2);
    } while ((spi_regval[1] & 0x02) == 0);
    McuSpiCsSet(g_spi_num);

    HAL_Delay(1);
}

__RAM_FUNC void Sector_erase(void)
{
    //扇区擦除
    spi_instr[0] = Sector_Erase;
    spi_instr[1] = 0x00; //flash地址：bit[23:16]
    spi_instr[2] = 0x00; //flash地址：bit[15:8]
    spi_instr[3] = 0x00; //flash地址：bit[7:0]

    McuSpiCsReset(g_spi_num);
    McuSpiMasterWrite(g_spi_num, spi_instr, 4);
    McuSpiCsSet(g_spi_num);

    HAL_Delay(1);
}
//读取芯片数据
__RAM_FUNC void Read_chip_data()
{
    //读状态寄存器，等待不busy
    Register_check_busy();
    //读数据操作数据包包头
    spi_instr[0] = Read_Data;
    spi_instr[1] = 0x00; //flash地址：bit[23:16]
    spi_instr[2] = 0x00; //flash地址：bit[15:8]
    spi_instr[3] = 0x00; //flash地址：bit[7:0]
    //将读数据操作数据包头，dummy数据打包成发送数据包，并进行读数据操作
    p_send_packet = xy_malloc(4 + SPI_LEN_MAX);
    p_recv_packet = xy_malloc(4 + SPI_LEN_MAX);
    memset(p_send_packet, 0x00, 4 + SPI_LEN_MAX);
    memcpy(p_send_packet, spi_instr, 4);

    McuSpiCsReset(g_spi_num);
    McuSpiMasterWriteRead(g_spi_num, p_send_packet, p_recv_packet, 4 + SPI_LEN_MAX);
    McuSpiCsSet(g_spi_num);

    if (memcmp(spi_tx_buff, p_recv_packet+4, SPI_LEN_MAX) == 0)
    {
        const uint8_t uart_message[50] = {0};
        snprintf((char*)uart_message, 50, "W25Qxx Write and Read %dByte Succeed!\r\n", SPI_LEN_MAX);

        McuUartSet(1, 115200, 8, 2, 0, MCU_GPIO18, MCU_GPIO19);//txd -> gpio18，可观察结果
        McuUartTxEn(1);
        McuUartWriteFram(1, uart_message, strlen((char*)uart_message));
    }

    xy_free(p_send_packet);
    xy_free(p_recv_packet);
}

__RAM_FUNC void write_data()
{
    uint32_t send_cnt_integer = SPI_LEN_MAX / SPI_PAGE_SIZE;
    uint32_t send_cnt_remainder = SPI_LEN_MAX % SPI_PAGE_SIZE;
    //整数部分
    for (uint32_t i = 0; i < send_cnt_integer; i++)
    {
        //读状态寄存器，等待不busy、写禁止
        Register_check_write();
        //写使能
        Write_enable();
        //读状态寄存器，等待写使能  
        Wait_write_enabled();

        // 页编程操作数据包包头
        uint32_t flash_addr = i * SPI_PAGE_SIZE;
        spi_instr[0] = Page_Program;
        spi_instr[1] = (uint8_t)(flash_addr & 0xFF);             //flash地址：bit[23:16]
        spi_instr[2] = (uint8_t)((flash_addr & 0xFF00) >> 8);    //flash地址：bit[15:8]
        spi_instr[3] = (uint8_t)((flash_addr & 0xFF0000) >> 16); //flash地址：bit[7:0]

        //将页编程操作数据包头和发送数据打包成发送数据包，并进行页编程操作
        p_send_packet = xy_malloc(4 + SPI_PAGE_SIZE);
        memcpy(p_send_packet, spi_instr, 4);
        memcpy(p_send_packet + 4, &(spi_tx_buff[i * SPI_PAGE_SIZE]), SPI_PAGE_SIZE);

        McuSpiCsReset(g_spi_num);
        McuSpiMasterWrite(g_spi_num, p_send_packet, 4 + SPI_PAGE_SIZE);
        McuSpiCsSet(g_spi_num);

        xy_free(p_send_packet);
        HAL_Delay(1);
    }
    //小数部分
    {
        //读状态寄存器，等待不busy、写禁止
        Register_check_write();
        //写使能
        Write_enable();
        //读状态寄存器，等待写使能  
        Wait_write_enabled();

        // 页编程操作数据包包头
        uint32_t flash_addr = send_cnt_integer * SPI_PAGE_SIZE;
        spi_instr[0] = Page_Program;
        spi_instr[1] = (uint8_t)(flash_addr & 0xFF);             //flash地址：bit[23:16]
        spi_instr[2] = (uint8_t)((flash_addr & 0xFF00) >> 8);    //flash地址：bit[15:8]
        spi_instr[3] = (uint8_t)((flash_addr & 0xFF0000) >> 16); //flash地址：bit[7:0]

        //将页编程操作数据包头和发送数据打包成发送数据包，并进行页编程操作
        p_send_packet = xy_malloc(4 + send_cnt_remainder);
        memcpy(p_send_packet, spi_instr, 4);
        memcpy(p_send_packet + 4, &(spi_tx_buff[send_cnt_integer * SPI_PAGE_SIZE]), send_cnt_remainder);
        
        McuSpiCsReset(g_spi_num);
        McuSpiMasterWrite(g_spi_num, p_send_packet, 4 + send_cnt_remainder);
        McuSpiCsSet(g_spi_num);

        xy_free(p_send_packet);
        HAL_Delay(1);
    }    
}

__RAM_FUNC int main(void)
{
    SystemInit();
    EnablePrimask();
    //初始化发送数据，共SPI_LEN_MAX个
    uint32_t integer = SPI_LEN_MAX / 256;
    uint32_t remainder = SPI_LEN_MAX % 256;
    for (uint32_t i = 0; i < integer; i++)
    {
        for(uint32_t j = 0; j < 256; j++)
        {
            spi_tx_buff[i * 256 + j] = (uint8_t)j;
        }
    }
    for(uint32_t i = 0; i < remainder; i++)
    {
        spi_tx_buff[integer * 256 + i] = (uint8_t)i;
    }

    //初始化spi接口，1000KHz,工作模式0
    McuSpiMasterSet2(g_spi_num, g_spi_freq, g_spi_workmode, 25, 26, 52, 53);
    McuSpiEn(g_spi_num);

    //读状态寄存器，等待不busy、写禁止
    Register_check_write();
    //写使能
    Write_enable();
    //读状态寄存器，等待写使能  
    Wait_write_enabled();
    //扇区擦除
    Sector_erase();
    //按页写1000字节数据
    write_data();
    //读取芯片数据
    Read_chip_data();
    while (1)
    {
        ;
    }
}

