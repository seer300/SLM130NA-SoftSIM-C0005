#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "hal_gpio.h"
#include "hal_lpuart.h"
#include "driver_test_common.h"
#include "prcm.h"


#if (MasterSlave_UART == 1)

#define GPIO_CSP_TXD	GPIO_CSP1_TXD
#define GPIO_CSP_RXD	GPIO_CSP1_RXD 
#define HAL_CSP_INSTANCE	HAL_CSP1

#define HAL_CSP_ErrorCallback(__HANDLE__)  HAL_CSP1_ErrorCallback(__HANDLE__)
#define HAL_CSP_RxCpltCallback(__HANDLE__)	HAL_CSP1_RxCpltCallback(__HANDLE__)

#elif (MasterSlave_UART == 2)

#define GPIO_CSP_TXD	GPIO_CSP2_TXD
#define GPIO_CSP_RXD	GPIO_CSP2_RXD 
#define HAL_CSP_INSTANCE	HAL_CSP2

#define HAL_CSP_ErrorCallback(__HANDLE__)  HAL_CSP2_ErrorCallback(__HANDLE__)
#define HAL_CSP_RxCpltCallback(__HANDLE__)	HAL_CSP2_RxCpltCallback(__HANDLE__)

#else

#define GPIO_CSP_TXD	(255)
#define GPIO_CSP_RXD	(255) 
#define HAL_CSP_INSTANCE	(NULL)

#endif

extern HAL_I2C_HandleTypeDef *g_i2c_handle[]; //指向初始化句柄
extern HAL_SPI_HandleTypeDef *g_spi_handle; //指向当前句柄
extern HAL_CSP_HandleTypeDef *g_csp_handle[]; //指向初始化句柄

/****************配置驱动测试时的打印信息*******************/
int32_t InputDebugLevel = 1;
int32_t TestTimes = 0; //每一组配置的测试次数
int32_t Switchclksys =0; //是否进行时钟源切换

/****************************驱动测试时，主MCU与从MCU通信使用****************************/

volatile uint8_t MasterSlave_RxCplt_Flag = 0; //主机串口或者从机串口接收完成标志位

HAL_CSP_HandleTypeDef MasterSlave_UART_Handle = {0}; //主机或者从机通信串口


__RAM_FUNC void HAL_CSP_ErrorCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
}

__RAM_FUNC void HAL_CSP_RxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	UNUSED_ARG(hcsp);
	MasterSlave_RxCplt_Flag = 1;
}



void MasterSlave_UART_Init(void)//主机或者从机通信串口初始化
{
	//gpio初始化
	HAL_GPIO_InitTypeDef gpio_init = {0};

	gpio_init.Pin = GPIO_PAD_NUM_20;
	gpio_init.Mode = GPIO_MODE_HW_PER;
	gpio_init.PinRemap = GPIO_CSP_TXD;
	HAL_GPIO_Init(&gpio_init);

	gpio_init.Pin = GPIO_PAD_NUM_21;
    gpio_init.Mode = GPIO_MODE_HW_PER;
	gpio_init.PinRemap = GPIO_CSP_RXD;
	HAL_GPIO_Init(&gpio_init);

	//初始化CSP为UART
	MasterSlave_UART_Handle.Instance = HAL_CSP_INSTANCE;
	MasterSlave_UART_Handle.CSP_UART_Init.BaudRate = 9600;
	MasterSlave_UART_Handle.CSP_UART_Init.WordLength = HAL_CSP_UART_WORDLENGTH_8;
	MasterSlave_UART_Handle.CSP_UART_Init.StopBits = HAL_CSP_UART_STOPBITS_1;
	HAL_CSP_UART_Init(&MasterSlave_UART_Handle);
}

uint8_t MasterSlave_UART_GetInit(void)//返回主机或者从机通信串口是否初始化
{
	if(MasterSlave_UART_Handle.Instance != NULL)
	{
		return 1;
	}
	return 0;
}
/****************************驱动测试时，主MCU与从MCU通信使用****************************/

/**********************主MCU与上位机通信时，输出的格式化数据处理接口**********************/

/**
 * @brief  主机往PC机阻塞发送数据API，一直发送数据直到数据发完
 * 
 * @param  pData 发送数据缓冲区指针.
 * @param  Size  发送数据字节长度.
 */
void Master_Output_to_PC(char *pData, uint16_t Size)
{
    (void)pData, (void)Size;
#if (AT_LPUART == 1)
	extern HAL_LPUART_HandleTypeDef g_at_lpuart; //在at_uart.c中定义
	HAL_LPUART_Transmit(&g_at_lpuart, (uint8_t *)pData, Size, HAL_MAX_DELAY);
#endif
}

void Output_Print_ArgStruct(void *ArgStruct)
{
	uint32_t *pArgStruct = ArgStruct;
	char str[1024] = {0};
	sprintf(str, "\r\n");
	while (*pArgStruct != MaxArg)
	{
		sprintf(str + strlen(str), "0x%08lx/", *pArgStruct);
		pArgStruct++;
	}
	sprintf(str + strlen(str), "\r\n");
	Master_Output_to_PC(str, strlen(str));
}

void Output_Print_Hex(uint8_t *pData, uint16_t length)
{
	char str[16];
	uint8_t *pDataTmp = pData;
	Master_Output_to_PC("\r\n", strlen("\r\n"));
	for (uint32_t i = 0; i < length; i++)
	{
		memset(str, 0, 16);
		sprintf(str, "%02x ", *pDataTmp);
		Master_Output_to_PC(str, strlen(str));
		pDataTmp++;
	}
	Master_Output_to_PC("\r\n", strlen("\r\n"));
}

void Output_Print_Reg(uint32_t pRegAddr, uint32_t length)
{
	char str[16];
	uint32_t *pRegAddrTmp = (uint32_t *)pRegAddr;
	Master_Output_to_PC("\r\n", strlen("\r\n"));
	for (uint32_t i = 0; i < length; i++)
	{
		if ((i % 4) == 0)
		{
			memset(str, 0, 16);
			sprintf(str, "0x%08lx: ", (uint32_t)pRegAddrTmp);
			Master_Output_to_PC(str, strlen(str));
		}

		do
		{
			memset(str, 0, 16);
			sprintf(str, "0x%08lx ", *pRegAddrTmp);
			Master_Output_to_PC(str, strlen(str));
		} while (0);

		if (((i + 1) % 4) == 0)
		{
			Master_Output_to_PC("\r\n", strlen("\r\n"));
		}

		pRegAddrTmp++;
	}
	Master_Output_to_PC("\r\n", strlen("\r\n"));
}

void Output_Print_Str(char *fmt, ...)
{
	char buffer[1024] = {0};
	va_list arg_list;
	va_start(arg_list, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg_list);
	va_end(arg_list);
	Master_Output_to_PC(buffer, strlen(buffer));
}
/**********************主MCU与上位机通信时，输出的格式化数据处理接口**********************/

/************************提取字符串中的数字接口***********************/
int32_t get_num_from_cmd(uint8_t *str)
{
	char *cpy_str = (char *)xy_malloc(strlen((char *)str));
	memcpy(cpy_str, str, strlen((char *)str));

	char *tail = (char *)cpy_str;
	char *head = (char *)cpy_str;
	int ret = -1;

	while (*tail != '>')
		tail++;
	*tail = 0;
	while (*head < '0' || *head > '9')
		head++;

	ret = atoi(head);

	xy_free(cpy_str);

	return ret;
}
/************************提取字符串中的数字接口***********************/

/*********************************************驱动测试时，主机处理数据的一些接口*******************************************/
static int8_t *pLengthOfEveryArgArray = NULL;
static int8_t *pIndexOfEveryArgArray = NULL;
static uint32_t **pBackupArgAaary = NULL;

// static函数
static uint8_t GetArgIndex(int8_t *pLengthOfEveryArgArray, int8_t *pIndexOfEveryArgArray)
{
	for (uint8_t i = 0; i < strlen((char *)pLengthOfEveryArgArray); i++)
	{
		if (*(pIndexOfEveryArgArray + i) < *(pLengthOfEveryArgArray + i) - 1)
		{
			(*(pIndexOfEveryArgArray + i))++;
			return 1;
		}
		else
		{
			*(pIndexOfEveryArgArray + i) = 0;
		}
	}
	return 0;
}

//全局函数
void InitArgStruct(void *pArgStruct, ...)
{
	uint8_t ArgCountOfArray = 0;

	va_list valist;
	//计算入参数组个数
	va_start(valist, pArgStruct);
	while (va_arg(valist, uint32_t *) != MaxArgArray)
	{
		ArgCountOfArray++;
	}
	va_end(valist);
	// ArgCountOfArray包含MaxArgArray
	ArgCountOfArray++;

	//根据参数个数申请一段内存，用于记录每个入参数组的地址
	pBackupArgAaary = (uint32_t **)xy_malloc(ArgCountOfArray * 4);
	memset(pBackupArgAaary, 0, ArgCountOfArray);
	//根据参数个数申请一段内存，用于记录各个参数的可选数量，即每个入参数组的大小
	pLengthOfEveryArgArray = (int8_t *)xy_malloc(ArgCountOfArray);
	memset(pLengthOfEveryArgArray, 0, ArgCountOfArray);
	//根据参数个数申请一段内存，用于记录当前测试的各个参数
	pIndexOfEveryArgArray = (int8_t *)xy_malloc(ArgCountOfArray);
	memset(pIndexOfEveryArgArray, 0, ArgCountOfArray);
	*pIndexOfEveryArgArray = -1;

	//轮询所有入参，赋值给pBackupArgAaary
	va_start(valist, pArgStruct);
	for (uint8_t i = 0; i < ArgCountOfArray; i++)
	{
		*(pBackupArgAaary + i) = va_arg(valist, uint32_t *);
	}
	va_end(valist);

	//轮询每个入参数组，计算数组长度
	for (uint8_t i = 0; i < ArgCountOfArray - 1; i++)
	{
		uint8_t j = 0;
		while (*(*(pBackupArgAaary + i) + j) != MaxArg)
		{
			(*(pLengthOfEveryArgArray + i))++;
			j++;
		}
	}
}

void DeInitArgStruct(void *pArgStruct)
{
	UNUSED_ARG(pArgStruct);
	xy_free(pLengthOfEveryArgArray);
	xy_free(pIndexOfEveryArgArray);
	xy_free(pBackupArgAaary);
	pLengthOfEveryArgArray = NULL;
	pIndexOfEveryArgArray = NULL;
	pBackupArgAaary = NULL;
}

uint8_t GetArgStruct(void *pArgStruct)
{
	uint32_t *pArgStructTmp = pArgStruct;

	if (GetArgIndex(pLengthOfEveryArgArray, pIndexOfEveryArgArray))
	{
		for (uint8_t i = 0; i < strlen((char *)pLengthOfEveryArgArray); i++)
		{
			*(uint32_t *)pArgStructTmp = (uint32_t) * (*(pBackupArgAaary + i) + *(pIndexOfEveryArgArray + i));
			pArgStructTmp++;
		}
		*(uint32_t *)pArgStructTmp = MaxArg;
		return 1;
	}
	else
	{
		return 0;
	}
}

uint32_t GetTotalCaseNun(void)
{
	int8_t *pLengthOfEveryArgArrayTmp = pLengthOfEveryArgArray;
	uint32_t num = 1;

	for (uint8_t i = 0; i < strlen((char *)pLengthOfEveryArgArray); i++)
	{
		num = num * (*pLengthOfEveryArgArrayTmp);
		pLengthOfEveryArgArrayTmp++;
	}

	return num;
}

void GetCmdDataToSlave(void *pBuf, uint32_t id, uint32_t size, uint32_t DebugLevel, void *pArgStruct)
{
	uint32_t *pArgStructTmp = (uint32_t *)pArgStruct;
	uint32_t *pBufTmp = (uint32_t *)pBuf;

	memcpy(pBufTmp, &id, 4); //把id的值复制到pBuf
	pBufTmp++;
	memcpy(pBufTmp, &size, 4);
	pBufTmp++;
	memcpy(pBufTmp, &DebugLevel, 4);
	pBufTmp++;

	memcpy(pBufTmp, pArgStructTmp, size);
}
/*********************************************驱动测试时，主机处理数据的一些接口*******************************************/

/*********************************************驱动测试时，从机处理数据的一些接口*******************************************/
void GetArgStructFromMaster(void *pBuf, uint32_t *id, uint32_t *size, int32_t *DebugLevel, void *pArgStruct)
{
	uint32_t *pBufTmp = (uint32_t *)pBuf;
	uint32_t *pArgStructTmp = (uint32_t *)pArgStruct;

	*id = *pBufTmp;
	pBufTmp++;
	*size = *pBufTmp;
	pBufTmp++;
	*DebugLevel = *pBufTmp;
	pBufTmp++;

	memcpy(pArgStructTmp, pBufTmp, *size);
}
/*********************************************驱动测试时，去初始化用的一些接口*******************************************/


HAL_StatusTypeDef HAL_I2C_Drivertest_DeInit(HAL_I2C_HandleTypeDef *hi2c)
{
    assert_param(IS_I2C_INSTANCE(hi2c->Instance));

	if (hi2c == NULL)
	{
		return HAL_ERROR;
	}

	if (hi2c->State == HAL_I2C_STATE_RESET)
	{
		return HAL_ERROR;
	}

	hi2c->State = HAL_I2C_STATE_BUSY;

	//配置时钟源，使能I2C时钟
	if (hi2c->Instance == HAL_I2C1)
	{
		PRCM_I2C1refclkSet(I2C1_REFCLK_SEL_LSioclk);
		PRCM_ClockEnable(CORE_CKG_CTL_I2C1_EN);
	}
	else if (hi2c->Instance == HAL_I2C2)
	{
		PRCM_I2C2refclkSet(I2C2_REFCLK_SEL_LSioclk);
		PRCM_ClockEnable(CORE_CKG_CTL_I2C2_EN);
	}

	//软复位I2C
	I2C_DeInit(hi2c->Instance);

	//关闭所有中断源
	I2C_IntDisable(hi2c->Instance, I2C_INT_ALL);

	//清所有中断标志位
	I2C_IntClear(hi2c->Instance, I2C_INT_ALL);

	//复位寄存器
	hi2c->Instance->CTL = 0x400;
    hi2c->Instance->ADDR = 0x00;

	//释放外设IO分配
	if (hi2c->Instance == HAL_I2C1)
	{
		GPIO_AllocateRemove(GPIO_I2C1_SCL);
		GPIO_AllocateRemove(GPIO_I2C1_SDA);
	}
	else if (hi2c->Instance == HAL_I2C2)
	{
		GPIO_AllocateRemove(GPIO_I2C2_SCL);
		GPIO_AllocateRemove(GPIO_I2C2_SDA);
	}

	//失能外设时钟
	if (hi2c->Instance == HAL_I2C1)
	{
		PRCM_ClockDisable(CORE_CKG_CTL_I2C1_EN);
	}
	else if (hi2c->Instance == HAL_I2C2)
	{
		PRCM_ClockDisable(CORE_CKG_CTL_I2C2_EN);
	}

    //释放句柄
	switch((uint32_t)hi2c->Instance)
	{
		case (uint32_t)HAL_I2C1	:{ g_i2c_handle[0] = NULL; break;}
		case (uint32_t)HAL_I2C2	:{ g_i2c_handle[1] = NULL; break;}
		default :return HAL_ERROR;
	}

	hi2c->XferDir = HAL_I2C_DIR_NONE;
	hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
	hi2c->State = HAL_I2C_STATE_RESET;

	__HAL_UNLOCK(hi2c);

	return HAL_OK;
}

static void HAL_CSP_Drivertest_ClockSet(HAL_CSP_HandleTypeDef *hcsp, HAL_FunctionalState state)
{
    //使能CSP时钟
    if(state == HAL_ENABLE)
    {
        if (hcsp->Instance == HAL_CSP1)
        {
            PRCM_ClockEnable(CORE_CKG_CTL_CSP1_EN);
        }
        else if (hcsp->Instance == HAL_CSP2)
        {
            PRCM_ClockEnable(CORE_CKG_CTL_CSP2_EN);
        }
        else if (hcsp->Instance == HAL_CSP3)
        {
            PRCM_ClockEnable(CORE_CKG_CTL_CSP3_EN);
        }
    }
    //失能CSP时钟
    else if(state == HAL_DISABLE)
    {
        if (hcsp->Instance == HAL_CSP1)
        {
            PRCM_ClockDisable(CORE_CKG_CTL_CSP1_EN);
        }
        else if (hcsp->Instance == HAL_CSP2)
        {
            PRCM_ClockDisable(CORE_CKG_CTL_CSP2_EN);
        }
        else if (hcsp->Instance == HAL_CSP3)
        {
            PRCM_ClockDisable(CORE_CKG_CTL_CSP3_EN);
        }
    }
}

HAL_StatusTypeDef HAL_CSP_Drivertest_DeInit(HAL_CSP_HandleTypeDef *hcsp)
{
    assert_param(IS_CSP_INSTANCE(hcsp->Instance));

    if (hcsp == NULL)
	{
		return HAL_ERROR;
	}

	if (hcsp->gState == HAL_CSP_STATE_RESET)
	{
		return HAL_ERROR;
	}

	hcsp->gState = HAL_CSP_STATE_BUSY;

	//使能CSP时钟
	HAL_CSP_Drivertest_ClockSet(hcsp, HAL_ENABLE);

    //关闭所有中断源
	CSP_IntDisable(hcsp->Instance, CSP_INT_ALL);

	//清所有中断标志位
	CSP_IntClear(hcsp->Instance, CSP_INT_ALL);

    //清FIFO
	CSP_TXFifoClear(hcsp->Instance);
	CSP_RXFifoClear(hcsp->Instance);

	//复位寄存器
	hcsp->Instance->MODE1 = 0x0;
	hcsp->Instance->MODE2 = 0x0;
	hcsp->Instance->TX_FRAME_CTRL = 0x0;
	hcsp->Instance->RX_FRAME_CTRL = 0x0;
	hcsp->Instance->SYNC_SLAVE_CFG = 0x0;
	hcsp->Instance->AYSNC_PARAM_REG = 0xFFFF;
	hcsp->Instance->IRDA_X_MODE_DIV = 0xAA;
	hcsp->Instance->SM_CFG = 0x1C18;
	hcsp->Instance->TX_DMA_IO_CTRL = 0x0;
	hcsp->Instance->TX_DMA_IO_LEN = 0xFFFFFFFF;
	hcsp->Instance->TX_FIFO_CTRL = 0x0;
	hcsp->Instance->TX_FIFO_LEVEL_CHK = 0x0;
	hcsp->Instance->TX_FIFO_OP = 0x0;
	hcsp->Instance->RX_DMA_IO_CTRL = 0x0;
	hcsp->Instance->RX_DMA_IO_LEN = 0xFFFFFFFF;
	hcsp->Instance->RX_FIFO_CTRL = 0x0;
	hcsp->Instance->RX_FIFO_LEVEL_CHK = 0x0;
	hcsp->Instance->RX_FIFO_OP = 0x0;

	//释放外设IO分配
	if (hcsp->Instance == HAL_CSP1)
	{
		GPIO_AllocateRemove(GPIO_CSP1_SCLK);
		GPIO_AllocateRemove(GPIO_CSP1_TXD);
		GPIO_AllocateRemove(GPIO_CSP1_TFS);
		GPIO_AllocateRemove(GPIO_CSP1_RXD);
		GPIO_AllocateRemove(GPIO_CSP1_RFS);
	}
	else if (hcsp->Instance == HAL_CSP2)
	{
		GPIO_AllocateRemove(GPIO_CSP2_SCLK);
		GPIO_AllocateRemove(GPIO_CSP2_TXD);
		GPIO_AllocateRemove(GPIO_CSP2_TFS);
		GPIO_AllocateRemove(GPIO_CSP2_RXD);
		GPIO_AllocateRemove(GPIO_CSP2_RFS);
	}
	else if (hcsp->Instance == HAL_CSP3)
	{
		GPIO_AllocateRemove(GPIO_CSP3_SCLK);
		GPIO_AllocateRemove(GPIO_CSP3_TXD);
		GPIO_AllocateRemove(GPIO_CSP3_TFS);
		GPIO_AllocateRemove(GPIO_CSP3_RXD);
		GPIO_AllocateRemove(GPIO_CSP3_RFS);
	}

    //关闭CSP
    CSP_Disable(hcsp->Instance);

    //失能外设时钟
    HAL_CSP_Drivertest_ClockSet(hcsp, HAL_DISABLE);

    //释放句柄
	switch((uint32_t)hcsp->Instance)
	{
		case (uint32_t)HAL_CSP1	:{ g_csp_handle[0] = NULL; break;}
		case (uint32_t)HAL_CSP2	:{ g_csp_handle[1] = NULL; break;}
		case (uint32_t)HAL_CSP3	:{ g_csp_handle[2] = NULL; break;}
		default :return HAL_ERROR;
	}

	hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
	hcsp->gState = HAL_CSP_STATE_RESET;
	hcsp->RxState = HAL_CSP_STATE_RESET;

	__HAL_UNLOCK(hcsp);
	__HAL_UNLOCK_RX(hcsp);

	return HAL_OK;
}


typedef enum
{
    HAL_UART_INTTYPE_FRAME_ERR = UART_INT_RX_FRAME_ERR,       /*!< UART帧错误中断 */
    HAL_UART_INTTYPE_PARITY_ERR = UART_INT_RX_PAR_ERR,        /*!< UART校验错误中断 */
    HAL_UART_INTTYPE_CTS = UART_INT_FLOW_CTL,                 /*!< UART CTS有效中断 */
    HAL_UART_INTTYPE_RX_FIFO_OVF = UART_INT_RXFIFO_OVFLW,     /*!< UART接收溢出中断 */
    HAL_UART_INTTYPE_RX_TIMEOUT = UART_INT_TIMEOUT,           /*!< UART接收超时中断 */
    HAL_UART_INTTYPE_RX_FIFO_THD = UART_INT_RXFIFO_TRIGGER,   /*!< UART接收阈值中断，当RXFIFO现存量处于接收阈值区间时触发，接收阈值区间范围是小于上限、大于等于下限 */
    HAL_UART_INTTYPE_TX_FIFO_THD = UART_INT_TXFIFO_TRIGGER,   /*!< UART发送阈值中断，当TXFIFO现存量处于发送阈值区间时触发，发送阈值区间范围是小于等于上限、大于下限 */
	HAL_UART_INTTYPE_TX_FIFO_EMPTY = UART_INT_TXFIFO_EMPTY    /*!< LPUARTTXFIFO空中断 */
} HAL_UART_IntTypeDef;


void UART_GPIO_Remove(void)
{
    //释放外设IO分配
	GPIO_AllocateRemove(GPIO_UART2_TXD);
	GPIO_AllocateRemove(GPIO_UART2_RTS);
	GPIO_AllocateRemove(GPIO_UART2_RXD);
	GPIO_AllocateRemove(GPIO_UART2_CTS);    
}
typedef enum
{
    HAL_SPI_INTTYPE_HARDWARE_FAULT = SPI_INT_MODE_FAIL,        /*!< SPI硬件故障中断，主机或从机模式下CS引脚意外拉高 */
    HAL_SPI_INTTYPE_RX_FIFO_OVF = SPI_INT_RX_OVERFLOW,         /*!< SPI RXFIFO溢出中断 */
    HAL_SPI_INTTYPE_TX_FIFO_THD = SPI_INT_TX_FIFO_NFULL,       /*!< SPI TXFIFO阈值中断，当TXFIFO现存量小于阈值时触发 */
    HAL_SPI_INTTYPE_RX_FIFO_THD = SPI_INT_RX_FIFO_NEMPTY       /*!< SPI RXFIFO阈值中断，当RXFIFO现存量大于等于阈值时触发 */
}HAL_SPI_IntTypeDef;

static void HAL_SPI_Drivertest_Disable_IT(HAL_SPI_HandleTypeDef *hspi, HAL_SPI_IntTypeDef IntFlag)
{
    (hspi->Instance->IDIS) |= IntFlag;
}

static void HAL_SPI_Drivertest_Clear_IT(HAL_SPI_HandleTypeDef *hspi, HAL_SPI_IntTypeDef IntFlag)
{
    (hspi->Instance->INT_STATUS) |= IntFlag;
}

HAL_StatusTypeDef HAL_SPI_Drivertest_DeInit(HAL_SPI_HandleTypeDef *hspi)
{
    assert_param(IS_SPI_INSTANCE(hspi->Instance));

	if (hspi == NULL)
    {
		return HAL_ERROR;
    }

	if (hspi->State == HAL_SPI_STATE_RESET)
	{
		return HAL_ERROR;
	}

	hspi->State = HAL_SPI_STATE_BUSY;

	//使能SPI外设时钟
	PRCM_ClockEnable(CORE_CKG_CTL_SPI_EN);

    //关闭所有中断源
    HAL_SPI_Drivertest_Disable_IT(hspi, SPI_INT_ALL);

    //清所有中断标志位
    HAL_SPI_Drivertest_Clear_IT(hspi, SPI_INT_ALL);

    //清FIFO
	SPI_TxFifoReset();
	SPI_RxFifoReset();

	//复位寄存器
	SPI_ConfigSetExpClk(SPI_CONFIG_CLK_DIV_2, SPI_FRF_MOTO_MODE_0, SPI_CONFIG_MODE_SLAVE, SPI_CONFIG_WORD_SIZE_BITS_8);
	SPI_SetDelay(0, 0, 0, 0);
	SPI_IdleCountSet(0xFF);
	SPI_SetTxFifoThreshold(0x1);
	SPI_SetRxFifoThreshold(0x1);
	SPI_TxFifoDisable();
	SPI_RxFifoDisable();

	//释放外设IO分配
	GPIO_AllocateRemove(GPIO_SPI_SCLK);
	GPIO_AllocateRemove(GPIO_SPI_MOSI);
	GPIO_AllocateRemove(GPIO_SPI_MISO);
	GPIO_AllocateRemove(GPIO_SPI_SS_N);
	GPIO_AllocateRemove(GPIO_SPI_SS1_N);
	GPIO_AllocateRemove(GPIO_SPI_CS_N);
    GPIO_InputPeriSelectCmd(GPIO_SPI_CS_N, DISABLE);//对于有输入、输出功能的PAD需要失能输入选择

	//关闭SPI
	SPI_Disable();

	//失能外设时钟
	PRCM_ClockDisable(CORE_CKG_CTL_SPI_EN);

    //释放句柄
    g_spi_handle = NULL;

	hspi->ErrorCode = HAL_SPI_ERROR_NONE;
	hspi->State = HAL_SPI_STATE_RESET;

	__HAL_UNLOCK(hspi);

	return HAL_OK;
}


