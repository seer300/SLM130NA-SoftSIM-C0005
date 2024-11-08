/*************************************************************************************/
#include "U_eeprom_driver.h"
#include "U_timer1uS_driver.h" // Device header
#include "mcu_adapt.h"

/*------------------------------ macro Define -------------------------------------*/

s_g_EE_t s_g_EE = {cst_EE_M_IDLE};
EE_Message g_EE_Msg; //Message信息结构
u8* g_pMsg; //全局Message指针

//static u8 s_g_EE_timeout_num = 0; //超时次数
//static u8 s_g_I2C_State = cst_I2C_S0; //子状态机状态
//u8* g_pData; //全局数据传递指针
//u8* g_pData_Temp; //用于保存申请者原始数据指针
//u8 ee_len;//数据长度
//u8 ee = 0;
//u8 total_len;       //数据总长度
//u8* g_pMsg; //全局Message指针
//u32 Timer1uSBase;
//u32 odd_timer = 0;
//u32 readend_timer = 0;
//static u16 s_g_I2C_IO_Idx = 0; //申请空间下标
//static u16 s_g_I2C_Len = 0; //要读些的字节数
//static u16 s_g_StartPageAddr = 0; //起始页的相对地址
//static u16 s_g_StartPageNum = 0; //起始页要写入数据数量
//static u16 s_g_PageNum = 0; //要写入的页数
//static u16 s_g_TurnPage = 0;
//static u16 s_g_LastPage_Num = 0; //不足一页的数据个数
//static u16 s_g_MultPageflag = ERROR; //跨页写入标志
//TWO_BYTE_TO_WORD target_addr; //目标地址(2字节)
//EE_Message g_EE_Msg; //Message信息结构
//M0P_I2C_TypeDef *hi2c1;

/*------------------------------ Function Define -------------------------------------*/
static void PointToIicSeqread(void); //启动I2C读EEP
static void PointToIicSeqwrite(void); //启动I2C写EEP
 void GetIicResult(void); // 返回I2C状态机结果给申请者指针
static u8 I2cCheckCompleted(void); //查寻I2C状态机是否完成
static u8 I2cCheckRorW(void); //查寻I2C状态机是读还是写
static u8 CheckTimer(u32 timer);
static void SetTimer(u32 timer);

/*************************************************************************************
 * @fun_name:	EepInit(void)
 * @brief   :
 * @param[in] :
 * @param[out] :None
 * @retval     :None
 * @other     :  I2C初始化
 *************************************************************************************/
void EepInit(void)
{
	McuGpioModeSet(EE_POWER_PIN, 0x00); //EE控制脚,推挽输出
	McuGpioWrite(EE_POWER_PIN, 0);
	McuI2cMasterSet2(EE_USE_IIC, I2C_CLK, EE_SCL_PIN, EE_SDA_PIN);
}

/*************************************************************************************
 * @fun_name:	EepMachineDriver(void)
 * @brief   :
 * @param[in] :	 None
 * @param[out] :None
 * @retval     :    None
 * @other     :  前台主状态机
 *************************************************************************************/

extern void I2c0RxIrqHandle(void);
extern void I2c0TxIrqHandle(void);

void EepMachineDriver(void)
{
	uint16_t offset;
	uint16_t write_size;
	uint16_t remain_size;

    switch (s_g_EE.main_state) //IDLE状态
    {
        case cst_EE_M_IDLE:
            break;

        case cst_EE_M_WRITE_S0: //启动写任务
        {
            McuI2cTxIrqReg(EE_USE_IIC, I2c0TxIrqHandle);
			McuGpioWrite(EE_POWER_PIN, 1);
			s_g_EE.main_state = cst_EE_M_WRITE_S1;
        }

        case cst_EE_M_WRITE_S1: //循环写任务
		{
        	if(s_g_EE.txcnt < s_g_EE.txlen)
        	{
        		remain_size = s_g_EE.txlen - s_g_EE.txcnt;
        		offset = s_g_EE.ee_addr & (EEPageSize - 1);
				if(remain_size <= (EEPageSize - offset))
					write_size = remain_size;
				else
					write_size = EEPageSize - offset;

				s_g_EE.has_txcnt = s_g_EE.txcnt + write_size;
				McuI2cMasterWrite_It(EE_USE_IIC, Write_Addr, I2C_DATA_ADDR_SIZE, s_g_EE.ee_addr, s_g_EE.data_ptr, write_size, 500);
				s_g_EE.ee_addr += write_size;  //下一次传输开始地址
				s_g_EE.main_state = cst_EE_M_WRITE_S2;
        	}
		}
		break;

        case cst_EE_M_WRITE_S2: //等待一页写完或者整个写完
		{
			if(s_g_EE.txcnt == s_g_EE.has_txcnt)
			{
				if(s_g_EE.txcnt == s_g_EE.txlen)
				{
					uint32_t tickstart = Get_Tick();
					while(!Check_Ms_Timeout(tickstart, 2));
					McuGpioWrite(EE_POWER_PIN, 0);
//					McuI2cTxIrqUnReg(EE_USE_IIC);
					g_EE_Msg.structs.Completed = SUCCESS;
					s_g_EE.main_state = cst_EE_M_IDLE;
				}
				else
				{
					s_g_EE.main_state = cst_EE_M_WRITE_S1;
				}
			}
		}
		break;

        case cst_EE_M_READ_S0: //启动读任务
        {
            McuI2cRxIrqReg(EE_USE_IIC, I2c0RxIrqHandle);
			McuGpioWrite(EE_POWER_PIN, 1);
			McuI2cMasterRead_It(0, Read_Addr, I2C_DATA_ADDR_SIZE, s_g_EE.ee_addr, s_g_EE.data_ptr, s_g_EE.rxlen, 500);
			s_g_EE.main_state = cst_EE_M_READ_S1;
        }
        break;

        case cst_EE_M_READ_S1: //等待读任务完成
		{
			if(s_g_EE.rxcnt == s_g_EE.rxlen)
			{
				McuGpioWrite(EE_POWER_PIN, 0);
//				McuI2cRxIrqUnReg(EE_USE_IIC);
				g_EE_Msg.structs.Completed = SUCCESS;
				s_g_EE.main_state = cst_EE_M_IDLE;
			}
		}
		break;

        default:
        {
        	s_g_EE.main_state = cst_EE_M_IDLE;
            break;
        }
    }
}

/*************************************************************************************
 * @fun_name:	void  SetTimer(u32 timer)
 * @brief   :
 * @param[in] :	 None
 * @param[out] :None
 * @retval     :    None
 * @other     :    内部接口
 *************************************************************************************/
void  SetTimer(u32 timer)
{

}
/*************************************************************************************
* @fun_name:	void  SetTimer(u32 timer)
* @brief   :
* @param[in] :	 None
* @param[out] :None
* @retval     :    None
* @other     :    内部接口
*************************************************************************************/
u8 CheckTimer(u32 timer)
{

}
/*************************************************************************************
 * @fun_name:	void I2cIsrMachine(void)
 * @brief   :
 * @param[in] :	 None
 * @param[out] :None
 * @retval     :    None
 * @other     :    内部接口
 *************************************************************************************/
void I2c0_IRQHandler(void)
{

}

/************************************************************************************
 * @brief  LPUART中断服务函数
 * @return NA
************************************************************************************/

__RAM_FUNC void I2c0RxIrqHandle(void)
{
    uint8_t sta = 0, data = 0;
    sta = McuI2cRead(EE_USE_IIC, &data);//读取数据
    if(sta)
    {
    	*s_g_EE.data_ptr = data;
    	s_g_EE.rxcnt++;
    	s_g_EE.data_ptr++;
    }
}

__RAM_FUNC void I2c0TxIrqHandle(void)
{
	s_g_EE.txcnt++;
	s_g_EE.data_ptr++;
    if(s_g_EE.txcnt != (s_g_EE.has_txcnt)) //发送完成判断
    {
		 //发送数据以触发TXFIFO阈值中断
		(s_g_EE.txcnt == (s_g_EE.has_txcnt - 1))? \
		I2C_PutData(I2C1_BASE, ((*s_g_EE.data_ptr) | 0x0200)) : \
		I2C_PutData(I2C1_BASE, *s_g_EE.data_ptr); // 最后1字节数据产生停止条件
    }
}

/*************************************************************************************
 * @fun_name:	void I2C1_EV_IRQHandler(void)
 * @brief   :
 * @param[in] :	 None
 * @param[out] :None
 * @retval     :    None
 * @other     :  I2C1中断
 *************************************************************************************/
//void I2c0_IRQHandler(void)
//{
//	I2cIsrMachine();
//}

/*************************************************************************************
 * @fun_name:	u8 EepIfIdle (void)
 * @brief   :
 * @param[in] :	 None
 * @param[out] :TASK_IDLE:空闲;TASK_BUSY:忙碌
 * @retval     :    None
 * @other     :   工作接口
 *************************************************************************************/
u8 EepIfIdle(void)
{
    if (s_g_EE.main_state == cst_EE_M_IDLE)// && I2c_GET_FLAG(hi2c1, I2C_ISR_IDLE) == TRUE)
        return 1;
    else
        return 0;
}

/*************************************************************************************
 * @fun_name:	u8 EepRead(u16 EEaddr,u8 * Data,u8 Num,u8* pMsg)
 * @brief   :      申请启动连续读EEPROM任务
 * @param[in] :	 存储首字节地址(2字节)EEaddr,读出地址要存储的位置指针data,读出的字节数num,读取过程中发生的状态信息指针pMsg
 * @param[out] :SUCCESS:数据有效性判断成功，允许读取；ERROR：数据有效性判断失败，不允许读取
                          读取的数据内容 (*data)，传递出的message内容 (*pMsg)
 * @retval     :    None
 * @other     :    工作接口
 *************************************************************************************/
u8 EepRead(u16 EEaddr, u8* Data, u16 Num, u8* pMsg)
{
	if(s_g_EE.main_state != cst_EE_M_IDLE)
		return ERROR;

    g_EE_Msg.MessageInfo = 0x40;
    *pMsg = 0;

    if (EEaddr + Num <= (EE_ADDR_MAX + 1))
    {
        if (Num > 0 && Num <= READ_LIMITSIZE_MAX) //???????
        {
        	s_g_EE.main_state = cst_EE_M_READ_S0; //这里要不要锁中断
        	s_g_EE.ee_addr = EEaddr;
        	s_g_EE.data_ptr = Data;
        	s_g_EE.rxcnt = 0;
        	s_g_EE.rxlen = Num;
        	g_pMsg = pMsg;

            return SUCCESS;
        }
        else
            return ERROR;
    }
    else
        return ERROR;
}
/*************************************************************************************
 * @fun_name:	void PointToIicSeqread(void)
 * @brief   :        启动I2C读EEPROM中断状态机
 * @param[in] :	 None
 * @param[out] :None
 * @retval     :    None
 * @other     :    内部接口
 *************************************************************************************/
void PointToIicSeqread(void)
{

}
/*************************************************************************************
 * @fun_name:	u8 EepWrite (u8* Data,u16 EEaddr,u16 Num,u8* pMsg)
 * @brief   :       申请启动连续写EEPROM任务
 * @param[in] :	存储写入数据的位置指针data，存储首字节地址（2个字节）EEaddr，写入的字节数num,写入过程中发生的状态信息指针pMsg
 * @param[out] :_SUCCESS:数据有效性判断成功，允许写入；_ERROR：数据有效性判断失败，不允许写入
                          传递出的message内容 (*pMsg)
 * @retval     :    None
 * @other     :   工作接口
 *************************************************************************************/
u8 EepWrite(u8* Data, u16 EEaddr, u16 Num, u8* pMsg)
{
	if(s_g_EE.main_state != cst_EE_M_IDLE)
		return ERROR;

    g_EE_Msg.MessageInfo = 0x60;
    *pMsg = 0;

    if (EEaddr + Num <= (EE_ADDR_MAX + 1))
    {
        if (Num > 0 && Num <= WRITE_LIMITSIZE_MAX)
        {
			s_g_EE.main_state = cst_EE_M_WRITE_S0; //这里要不要锁中断
			s_g_EE.ee_addr = EEaddr;
			s_g_EE.data_ptr = Data;
			s_g_EE.txlen = Num;
			s_g_EE.txcnt = 0;
			s_g_EE.has_txcnt = 0;
			g_pMsg = pMsg;

            return SUCCESS;
        }
        else
            return ERROR;
    }
    else
        return ERROR;
}

/*************************************************************************************
 * @fun_name:	void PointToIicSeqwrite(void)
 * @brief   :        启动I2C写EEPROM中断状态机
 * @param[in] :	 None
 * @param[out] :None
 * @retval     :    None
 * @other     :    内部接口
 *************************************************************************************/
void PointToIicSeqwrite(void)
{

}
/*************************************************************************************
 * @fun_name:	u8 EepCheckMsg (u8 Msg)
 * @brief   :       根据自己定义的Msg查询任务是否完成
 * @param[in] :	申请者自己定义的状态信息变量Msg
 * @param[out] :SUCCESS:任务完成；ERROR：任务未完成
 * @retval     :    None
 * @other     :     处于MainSpace，第二类接口：工作接口
 *************************************************************************************/
u8 EepCheckMsg(u8 Msg)
{
    if ((SUCCESS == I2cCheckCompleted()) && EepIfIdle())
        return SUCCESS;
    else
        return ERROR;
}

/*************************************************************************************
 * @fun_name:	u8 EepCheckState(u8 Msg)
* @brief   :        根据自己定义的Msg查询EEPROM是否异常
 * @param[in] :	 申请者自己定义的状态信息变量Msg
 * @param[out] :SUCCESS:EEPROM正常；ERROR_IES：EEPROM异常
 * @retval     :    None
 * @other     :    处于MainSpace，第二类接口：工作接口
 *************************************************************************************/
u8 EepCheckState(void)
{
    return g_EE_Msg.structs.EE_normal;
}

/*************************************************************************************
 * @fun_name:	void EepPreSleep(void)
 * @brief   :        休眠EEP模块
 * @param[in] :	 None
 * @param[out] :None
 * @retval     :    None
 * @other     :   处于MainSpace，第三类接口：休眠接口
 *************************************************************************************/
void EepPreSleep(void)
{

}

/*************************************************************************************
 * @fun_name:	u8 EepIfSleep(void)
 * @brief   :        查询EEPROM模块是否可以休眠
 * @param[in] :	 None
 * @param[out] :TRUE:EEPROM模块可以休眠；FALSE：EEPROM模块不可以休眠
 * @retval     :    None
 * @other     :   处于MainSpace，第三类接口：休眠接口
 *************************************************************************************/
u8 EepIfSleep(void)
{
    if(cst_EE_M_IDLE == s_g_EE.main_state)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*************************************************************************************
 * @fun_name:	void EepWakeSleep(void)
 * @brief   :        唤醒EEPROM模块
 * @param[in] :	 None
 * @param[out] :None
 * @retval     :    None
 * @other     :    处于MainSpace，第四类接口：唤醒接口
 *************************************************************************************/
void EepWakeSleep(void)
{

}

/********************************************************
 * @fun_name:	void GetIicResult(void)
 * @brief   :   返回I2C中断状态机任务结果给申请者指针
 * @param[in] :
 * @param[out] :
 * @retval     :    None
 * @other     :    内部接口
 *******************************************************/
void GetIicResult(void)
{
    *g_pMsg = g_EE_Msg.MessageInfo;
}

/*************************************************************************************
 * @fun_name:	u8 I2cCheckCompleted(void)
 * @brief   :        查询中断状态机任务是否完成
 * @param[in] :	 None
 * @param[out] :SUCCESS:任务完成；ERROR：任务未完成
 * @retval     :    None
 * @other     :    内部接口
 *************************************************************************************/
u8 I2cCheckCompleted(void)
{
    return g_EE_Msg.structs.Completed;
}

/*************************************************************************************
 * @fun_name:	u8 I2cCheckRorW(void)
 * @brief   :        查询中断状态机是读任务还是写任务
 * @param[in] :	 None
 * @param[out] :ERROR:读任务；SUCCESS：写任务
 * @retval     :    None
 * @other     :    内部接口
 *************************************************************************************/
u8 I2cCheckRorW(void)
{
    return g_EE_Msg.structs.EorW;
}











