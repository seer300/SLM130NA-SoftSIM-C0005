#include "user_config.h"
#include "mcu_adapt.h"
#include "xy_system.h"
#include "system.h"
    

/************************************************************************************************************
SPI_NORFLASH_TEST
本测试使用XY1200的SPI外设作为主机，W25Q16作为从机，在主函数中依次对W25Q16各个扇区进行擦除、读写测试，并对每个扇区的测
试结果进行打印输出。此测试主要是对mcu_spi.c中的接口耗时、功耗开销、稳定性、启停CP核的健壮性进行压测。

测试时，需要外接串口进打印测试结果，具体为将GPIO14接串口工具的RX引脚，可与I2C读写E2同时测试。

测试指令：AT+DEMOCFG=SPI,spi_num,spi_freq,spi_workmode //选择待测的SPI，以及速率模式，相关参数取值具体参考mcu_spi.c
         AT+DEMOCFG=BITMAP,256  //SPI单独测试
         AT+DEMOCFG=BITMAP,259  //SPI结合随机起停CP测试
         AT+DEMOCFG=BITMAP,264  //SPI、I2C联合测试
         AT+DEMOCFG=BITMAP,267  //SPI、I2C结合随机起停CP联合测试
*************************************************************************************************************/

#define FLASH_LEN_MAX   (2*1024*1024)                       //W25Q16最大存储空间，单位字节
#define SPI_PAGE_SIZE   (256)                               //页长度，单位字节
#define SPI_LEN_MAX     (256+4)                             //spi单次最大收发长度（页长度+4字节flash命令），单位字节
#define SPI_WR_TIMES    (FLASH_LEN_MAX / SPI_SECTOR_SIZE)   //SPI读写完全部FLASH存储空间需要的次数
#define SPI_SECTOR_SIZE (4*1024)                            //SPI 扇区大小，单位字节

//W25Q64指令
#define Write_Enable    (0x06) //写使能
#define Read_Status_Reg (0x05) //读状态寄存器
#define Sector_Erase    (0x20) //扇区擦除,4KB
#define Page_Program    (0x02) //页编程
#define Read_Data       (0x03) //读数据


//SPI类型选择
uint8_t g_spi_num = 0;       //硬件SPI接口选择，0：选择CSP_SPI进行NORFLASH读写测试，1：选择SPI进行NORFLASH读写测试
uint32_t g_spi_freq = 2000;   //频率(单位KHZ)
uint32_t g_spi_workmode = 0;    //SPI工作模式

uint8_t spi_instr[4] = {0};
uint8_t spi_regval[4] = {0};

uint8_t spi_tx_buff[SPI_LEN_MAX] = {0};
uint8_t spi_rx_buff[SPI_LEN_MAX] = {0};

static uint32_t SPIflash_test_cnt = 1;  //记录SPI_norflash的测试次数
static uint16_t g_ewr_times = 0;         //记录操作norflash的次数，每次操作一个扇区，不得大于SPI_WR_TIMES

void Register_check_write(void)
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
void Register_check_busy(void)
{
    //读状态寄存器，等待不busy
    McuSpiCsReset(g_spi_num);
    do{
        spi_instr[0] = Read_Status_Reg;
        McuSpiMasterWriteRead(g_spi_num, spi_instr, spi_regval, 2);
    } while (spi_regval[1] & 0x01);
    McuSpiCsSet(g_spi_num);

    HAL_Delay(1);
}
void Write_enable(void)
{
    //写使能
    spi_instr[0] = Write_Enable;  

    McuSpiCsReset(g_spi_num);
    McuSpiMasterWrite(g_spi_num, spi_instr, 1);
    McuSpiCsSet(g_spi_num);

    HAL_Delay(20);
}
void Wait_write_enabled(void)
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


void Page_Read_data(uint32_t flash_page_addr)
{
    uint8_t page_instr[4] = {0};
    uint8_t spi_tx_temp_buff[SPI_LEN_MAX] = {0};

    //读数据操作数据包包头
    page_instr[0] = Read_Data;
    page_instr[1] = (uint8_t)((flash_page_addr & 0xFF0000) >> 16);  //flash地址：bit[23:16]
    page_instr[2] = (uint8_t)((flash_page_addr & 0xFF00) >> 8);     //flash地址：bit[15:8]
    page_instr[3] = (uint8_t)(flash_page_addr & 0xFF);              //flash地址：bit[7:0]

    //读取寄存器当前状态，等待不busy
    Register_check_busy();
      
    memcpy(spi_tx_temp_buff, page_instr, 4);   
    McuSpiCsReset(g_spi_num);
    McuSpiMasterWriteRead(g_spi_num, spi_tx_temp_buff,spi_rx_buff, SPI_LEN_MAX);
    McuSpiCsSet(g_spi_num);
}

extern void PrepareSendData(uint32_t length, uint8_t *ptxdata, uint8_t inc);
void Page_program_handle(uint32_t flash_page_addr)
{
    uint8_t page_write_instr[4] = {0};

    //准备发送数据    
    srand(Get_Tick());//产生1-10之间的随机数
    uint8_t inc = (uint8_t)(rand() % 10 + 1);  
    PrepareSendData(SPI_LEN_MAX, spi_tx_buff, inc);

    //读取寄存器当前状态，等待不busy、写禁止
    Register_check_write();
    //写使能寄存器
    Write_enable();
    //等待写使能生效
    Wait_write_enabled();

    page_write_instr[0] = Page_Program;
    page_write_instr[1] = (uint8_t)((flash_page_addr & 0xFF0000) >> 16);  //flash地址：bit[23:16]
    page_write_instr[2] = (uint8_t)((flash_page_addr & 0xFF00) >> 8);     //flash地址：bit[15:8]
    page_write_instr[3] = (uint8_t)(flash_page_addr & 0xFF);              //flash地址：bit[7:0]            

    //将页编程操作数据包头和发送数据打包成发送数据包，并进行页编程操作
    memcpy(spi_tx_buff, page_write_instr, 4);
    McuSpiCsReset(g_spi_num);
    McuSpiMasterWrite(g_spi_num, spi_tx_buff, SPI_LEN_MAX);
    McuSpiCsSet(g_spi_num);

    HAL_Delay(5);
}


void SPI_Sector_Erase(uint32_t sector_addr)
{
    uint8_t sector_instr[4] = {0};

    //读取寄存器当前状态，等待不busy、写禁止
    Register_check_write();
    //写使能寄存器
    Write_enable();
    //等待写使能生效
    Wait_write_enabled();

    //扇区擦除
    sector_instr[0] = Sector_Erase;
    sector_instr[1] = (uint8_t)((sector_addr >> 16) & (0xFF)); //flash地址：bit[23:16]
    sector_instr[2] = (uint8_t)((sector_addr >> 8) & (0xFF));  //flash地址：bit[15:8]
    sector_instr[3] = (uint8_t)(sector_addr & (0xFF));         //flash地址：bit[7:0]
    
    //扇区擦除
    McuSpiCsReset(g_spi_num);
    McuSpiMasterWrite(g_spi_num, sector_instr, 4);
    McuSpiCsSet(g_spi_num);    

    HAL_Delay(200);
}


uint32_t spi_write_read_datacmp(void)
{
    if(memcmp((spi_tx_buff+4), (spi_rx_buff+4), SPI_PAGE_SIZE) == 0)
    {
        return 1;
    }  
    else
    {
        return 0;
    }
}

void SPI_test(void)
{
    uint32_t j;
    uint32_t sector_addr,page_addr;
    uint32_t norflashflag =0;  //0：flash操作异常，1：flash操作正常

    //flash扇区擦除
    sector_addr = g_ewr_times * SPI_SECTOR_SIZE;
    
    xy_printf("\r\n->NorFlash_E_W_R_SECTOR: No.%4d, SectorAddr: 0x%4x", g_ewr_times + 1, sector_addr);

    //扇区擦除
    SPI_Sector_Erase(sector_addr);

    //按页写入读取，并比对写入与读取的数据是否一致
    for(j = 0; j<SPI_SECTOR_SIZE/SPI_PAGE_SIZE; j++)
    {       
        page_addr = j * SPI_PAGE_SIZE + sector_addr;
    
        xy_printf("\r\n->NorFlash_E_W_R_SECTOR: No.%4d,SectorAddr: 0x%4x,PAGE: No.%4d,PageAddr: 0x%4x", g_ewr_times + 1, sector_addr, j + 1, page_addr);
        //生成随机数据，并写入flash
        Page_program_handle(page_addr);

        //读取数据
        Page_Read_data(page_addr);
        
        (spi_write_read_datacmp()==1) ? (norflashflag = 1) : (norflashflag = 0);

    }
 
    xy_assert(norflashflag == 1);

    //读写次数管理
    (g_ewr_times == (SPI_WR_TIMES - 1)) ? (g_ewr_times = 0) : (g_ewr_times++);
    
}


void SPI_Work(void)
{
    if(g_spi_num > 2 || g_spi_freq > 6000 || g_spi_workmode > 3)
    {
        xy_printf("\r\nerror: g_spi_num =%d or g_spi_freq =%d or g_spi_workmode =%d is illegal, %d\r\n", g_spi_num,g_spi_freq,g_spi_workmode);
        return;
    }

    // xy_printf("\r\nSPI_WORK START");
    // xy_printf("\r\n===============No.%d SPI%d_NORFLASH W&R Test===============", SPIflash_test_cnt, g_spi_num);
    // xy_printf("\r\n->NORFLASH_LEN_MAX: %4dMByte", FLASH_LEN_MAX/1024/1024);
    // xy_printf("\r\n->NORFLASH_PAGE_SIZE: %4dByte", SPI_PAGE_SIZE);
    // xy_printf("\r\n->NORFLASH_SECTOR_SIZE: %4dByte", SPI_SECTOR_SIZE);    

    //初始化spi接口
    McuSpiMasterSet2(g_spi_num, g_spi_freq, g_spi_workmode, 25, 26, 52, 53);
    McuSpiEn(g_spi_num);

    // 开始擦除、写入、读取数据测试
    SPI_test();

    // xy_printf("\r\n===============No.%d SPI%d_NORFLASH W&R Test===============", SPIflash_test_cnt, g_spi_num);
    // xy_printf("\r\nSPI_WORK END\r\n");
    
    //测试次数管理
    (SPIflash_test_cnt == 0xFFFFFFFF) ? (SPIflash_test_cnt = 1) : (SPIflash_test_cnt++);
}
