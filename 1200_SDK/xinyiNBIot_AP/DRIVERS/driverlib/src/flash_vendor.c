#include "flash_vendor.h"
#include "xy_memmap.h"
#include "sys_mem.h"
#include "utc.h"
#include "prcm.h"

void DMAChannelTransfer_ForFlash(unsigned long ulChannelNum, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned char ucMemType, unsigned char ucDmaAccessType)
{
  volatile uint32_t *ulSrcAddr_Align = (uint32_t *)ulSrcAddr;
  volatile uint32_t src_remainder = ulSrcAddr % 4;
  volatile uint32_t des_remainder = ulDstAddr % 4;

  if (ulLenByte == 0)
    return;

  if (ulSrcAddr >= FLASH_BASE && ulSrcAddr <= FLASH_BASE + FLASH_LENGTH)
  {

    uint32_t *temp_recv_buffer = (uint32_t *)ulDstAddr;

    if (src_remainder != 0)
    {
      ulSrcAddr_Align = (uint32_t *)(ulSrcAddr - src_remainder); //
      ulLenByte = ulLenByte + src_remainder;
      temp_recv_buffer = (uint32_t *)xy_malloc_r(ulLenByte);
    }
    else
    {
      if (des_remainder != 0)
      {
        temp_recv_buffer = (uint32_t *)xy_malloc_r(ulLenByte);
      }
    }

    if(ucDmaAccessType != 0)
    {
      qspi_apb_indirect_read((QSPI_FLASH_Def *)&xinyi_flash, (unsigned int)ulSrcAddr_Align, ulLenByte);
    }

    DMAChannelTransferSet(ulChannelNum, (void *)ulSrcAddr_Align, (void *)temp_recv_buffer, ulLenByte, ucMemType);
    DMAChannelTransferStart(ulChannelNum);
    DMAChannelWaitIdle(ulChannelNum);
    DMAIntClear(ulChannelNum);

    if (temp_recv_buffer != (uint32_t *)ulDstAddr)
    {
      if (src_remainder != 0)
      {
        memcpy((void *)ulDstAddr, (void *)((uint8_t *)temp_recv_buffer + src_remainder), ulLenByte - src_remainder);
      }
      else
      {
        memcpy((void *)ulDstAddr, (void *)temp_recv_buffer, ulLenByte);
      }

      xy_free_r(temp_recv_buffer);
    }

  }
  else
  {
    uint32_t *temp_send_buffer = (uint32_t *)ulSrcAddr;

    if (des_remainder != 0)
    {
      temp_send_buffer = (uint32_t *)xy_malloc_r(ulLenByte + des_remainder);
      memcpy((void *)temp_send_buffer, (void *)((uint8_t *)ulDstAddr - des_remainder), des_remainder);
      memcpy((void *)((uint8_t *)temp_send_buffer + des_remainder), (void *)ulSrcAddr, ulLenByte);

      if(ucDmaAccessType != 0)
      {
        qspi_apb_indirect_write((QSPI_FLASH_Def *)&xinyi_flash, (unsigned int)((uint8_t *)ulDstAddr - des_remainder), ulLenByte + des_remainder);
      }
      DMAChannelTransferSet(ulChannelNum, (void *)temp_send_buffer, (void *)((uint8_t *)ulDstAddr - des_remainder), ulLenByte + des_remainder, ucMemType);
    }
    else
    {
      if (src_remainder != 0)
      {
        temp_send_buffer = (uint32_t *)xy_malloc_r(ulLenByte);
        memcpy((void *)temp_send_buffer, (void *)ulSrcAddr, ulLenByte);
      }

      if(ucDmaAccessType != 0)
      {
        qspi_apb_indirect_write((QSPI_FLASH_Def *)&xinyi_flash, ulDstAddr, ulLenByte);
      }
      DMAChannelTransferSet(ulChannelNum, (void *)temp_send_buffer, (void *)ulDstAddr, ulLenByte, ucMemType);
    }

    DMAChannelTransferStart(ulChannelNum);
    DMAChannelWaitIdle(ulChannelNum);
    DMAIntClear(ulChannelNum);

    if((unsigned long)temp_send_buffer != ulSrcAddr)
    {
      xy_free_r(temp_send_buffer);
    }
  }
}

void flash_vendor_delay(unsigned long uldelay)
{
  volatile unsigned long i;

  for (i = 0; i < uldelay; i++)
  {
  }
}

unsigned char FLASH_Get_FlashType(void)
{
    unsigned char rdidType;

    rdidType = HWREGB(AON_AONGPREG1) & AONGPREG1_FLASH_RDID;

	return rdidType;
}

/**
 * @brief  config flash init.
 * @param  flash_vendor is the QSPI_FLASH_Def pointer for flash driver information.
 * @param  ref_clk_hz is the peripheral reference clock.
 * @param  sclk_hz is the qspi clock rate.
 * @retval None
 */
void FLASH_Init(QSPI_FLASH_Def *flash_vendor, unsigned int ref_clk_hz, unsigned int sclk_hz)
{
  flash_vendor->regbase = (unsigned char *)QSPI_BASE;
  flash_vendor->ahbbase = (unsigned char *)QSPI_DATA_BASE;
  flash_vendor->page_size = 256;
  flash_vendor->block_size = 16;
  flash_vendor->tshsl_ns = 60; // CS# High Time (read/write), >= 20ns
  flash_vendor->tsd2d_ns = 12;
  flash_vendor->tchsh_ns = 12;              // CS# Active Hold Time, >= 5ns
  flash_vendor->tslch_ns = 12;              // CS# Active Setup Time, >= 5ns
  flash_vendor->flash_type = FLASH_Get_FlashType();
  
  if(flash_vendor->flash_type == FLASH_XM25U32)
  {
     flash_vendor->otp_base = OTP_BASE_ADDR_XM25U32;
  }
  else
  {
      flash_vendor->otp_base = OTP_BASE_ADDR_GD25Q16;
  }
  flash_vendor->addr_bytes = 3;
  
  qspi_apb_controller_init(flash_vendor);
  qspi_apb_config_baudrate_div(ref_clk_hz, sclk_hz);

  //    qspi_apb_delay(ref_clk_hz, sclk_hz, flash_vendor->tshsl_ns, flash_vendor->tsd2d_ns, flash_vendor->tchsh_ns, flash_vendor->tslch_ns);

  qspi_apb_controller_disable();
  HWREG(QSPI_REG_DELAY) = QSPI_DELAY_DEFAULT;
  // loopback mode  
  HWREGB(QSPI_BASE + 0x09) = 4;
  HWREGB(QSPI_BASE + 0x08) |= 0x01;
  HWREG(QSPI_REG_CLK_CTRL) = 0;//disable force_clk_ena for power saving
  qspi_apb_controller_enable();
}

unsigned char FLASH_ReadStatusRegIdx(unsigned char ucStatusRegIdx)
{
  unsigned char flash_cmd;
  unsigned char flash_cmd_rw;

  if (STATUS_REG_S7_S0 == ucStatusRegIdx)
  {
    flash_cmd = FLASH_CMD_READ_STATUS_REG1;
  }
  else if (STATUS_REG_S15_S8 == ucStatusRegIdx)
  {
    flash_cmd = FLASH_CMD_READ_STATUS_REG2;
  }
  else
  {
    flash_cmd = FLASH_CMD_READ_STATUS_REG3;
  }

  flash_cmd_rw = 0x00;

  qspi_apb_command_read(1, &flash_cmd, 1, &flash_cmd_rw);

  return flash_cmd_rw;
}

void FLASH_WriteEnable(void)
{
  unsigned char flash_cmd;
  unsigned char flash_cmd_rw;

  flash_cmd = FLASH_CMD_WRITE_ENABLE;
  qspi_apb_command_write(1, &flash_cmd, 0, &flash_cmd_rw);

  flash_cmd = FLASH_CMD_READ_STATUS_REG1;
  flash_cmd_rw = 0x00;
  qspi_apb_command_read(1, &flash_cmd, 1, &flash_cmd_rw);

  while ((flash_cmd_rw & STATUS_REG1_WEL) == 0x00)
  {
    qspi_apb_command_read(1, &flash_cmd, 1, &flash_cmd_rw);
  }
}

void FLASH_WriteStatusReg(unsigned char ucStatusRegVal0, unsigned char ucStatusRegVal1)
{
  unsigned char flash_cmd;
  unsigned char flash_cmd_rw[2];

  FLASH_WriteEnable();

  flash_cmd = FLASH_CMD_WRITE_STATUS_REG;
  flash_cmd_rw[0] = ucStatusRegVal0;
  flash_cmd_rw[1] = ucStatusRegVal1;

  // 01H S7-S0 S15-S8
  qspi_apb_command_write(1, &flash_cmd, 2, &flash_cmd_rw[0]);
}

void FLASH_WriteStatusRegIdx(unsigned char ucStatusRegIdx, unsigned char ucStatusRegVal)
{
  unsigned char flash_cmd;
  unsigned char flash_cmd_rw;

  FLASH_WriteEnable();

  if (STATUS_REG_S7_S0 == ucStatusRegIdx)
  {
    flash_cmd = FLASH_CMD_WRITE_STATUS_REG1;
  }
  else if (STATUS_REG_S15_S8 == ucStatusRegIdx)
  {
    flash_cmd = FLASH_CMD_WRITE_STATUS_REG2;
  }
  else
  {
    flash_cmd = FLASH_CMD_WRITE_STATUS_REG3;
  }

  flash_cmd_rw = ucStatusRegVal;

  // 01H S7-S0; 31H S15-S8; 11H S23-S16
  qspi_apb_command_write(1, &flash_cmd, 1, &flash_cmd_rw);
}

void FLASH_StigSendCmd(unsigned char ucCmd)
{
  unsigned char flash_cmd;
  unsigned char flash_cmd_rw;

  flash_cmd = ucCmd;
  qspi_apb_command_write(1, &flash_cmd, 0, &flash_cmd_rw);
}

/**
 * @brief  erase the whole chip.
 * @retval None
 */
void FLASH_ChipErase(void)
{
  unsigned char flash_cmd;
  unsigned char flash_cmd_rw;

  FLASH_WriteEnable();

  flash_cmd = FLASH_CMD_CHIP_ERASE;
  qspi_apb_command_write(1, &flash_cmd, 0, &flash_cmd_rw);
}

/**
 * @brief  erase one sector(4kB).
 * @param  address is the original address.
 * @retval None
 */
void FLASH_SectorErase(unsigned int address)
{
  unsigned char flash_cmd[4];
  unsigned char flash_cmd_rw;

  FLASH_WriteEnable();

  flash_cmd[0] = FLASH_CMD_SECTOR_ERASE;
  flash_cmd[1] = (address >> 16) & 0xFF;
  flash_cmd[2] = (address >> 8) & 0xFF;
  flash_cmd[3] = (address >> 0) & 0xFF;
  qspi_apb_command_write(4, &flash_cmd[0], 0, &flash_cmd_rw);
}

/**
 * @brief  erase one block(32kB).
 * @param  address is the original address.
 * @retval None
 */
void FLASH_BlockErase32K(unsigned int address)
{
  unsigned char flash_cmd[4];
  unsigned char flash_cmd_rw;

  FLASH_WriteEnable();

  flash_cmd[0] = FLASH_CMD_BLOCK_ERASE_32K;
  flash_cmd[1] = (address >> 16) & 0xFF;
  flash_cmd[2] = (address >> 8) & 0xFF;
  flash_cmd[3] = (address >> 0) & 0xFF;
  qspi_apb_command_write(4, &flash_cmd[0], 0, &flash_cmd_rw);
}

/**
 * @brief  erase one block(64kB).
 * @param  address is the original address.
 * @retval None
 */
void FLASH_BlockErase64K(unsigned int address)
{
  unsigned char flash_cmd[4];
  unsigned char flash_cmd_rw;

  FLASH_WriteEnable();

  flash_cmd[0] = FLASH_CMD_BLOCK_ERASE_64K;
  flash_cmd[1] = (address >> 16) & 0xFF;
  flash_cmd[2] = (address >> 8) & 0xFF;
  flash_cmd[3] = (address >> 0) & 0xFF;
  qspi_apb_command_write(4, &flash_cmd[0], 0, &flash_cmd_rw);
}

/**
  * @brief  write flash data.
  * @param  flash_vendor is the QSPI_FLASH_Def pointer for flash driver information.
  * @param  ulSrcAddr is the source address.
  * @param  ulDstAddr is the destination address.
  * @param  ulLenByte is data length of bytes.
  * @param  ulChannelNum is the DMA channel number.
  * @param  ucMemType is the memory type as following:
              MEMORY_TYPE_AP
              MEMORY_TYPE_CP
  * @retval None
  */
void FLASH_WriteData(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum, unsigned char ucMemType)
{
  unsigned long ulBlockNum;
  unsigned long ulLeftByte;
  unsigned long i;
  unsigned char ucDmaAccessType = 1;

  (void)flash_vendor;

  if (ucMemType & 0x80)
  {
    ucDmaAccessType = 0;
  }

  ucMemType = ucMemType & 0x0F;

  DMAChannelConfigure(ulChannelNum, DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_16W | DMAC_CTRL_WORD_SIZE_32b);

  if (ucDmaAccessType)
  {
    qspi_apb_indirect_enable();
  }
  else
  {
    qspi_apb_indirect_disable();
  }

  ulBlockNum = ulLenByte >> 15;

  ulLeftByte = ulLenByte & 0x00007FFF;

  for (i = 0; i < ulBlockNum; i++)
  {

    DMAChannelTransfer_ForFlash(ulChannelNum, ulSrcAddr, ulDstAddr, FLASH_INDIRECT_MAX_BYTE, ucMemType, ucDmaAccessType);

    qspi_wbuf_wait_idle();

    qspi_wait_idle();

    if (ucDmaAccessType)
    {
      qspi_wait_writeburst_done();
    }

    ulDstAddr += FLASH_INDIRECT_MAX_BYTE;
    ulSrcAddr += FLASH_INDIRECT_MAX_BYTE;
  }

  if (ulLeftByte != 0)
  {

    DMAChannelTransfer_ForFlash(ulChannelNum, ulSrcAddr, ulDstAddr, ulLeftByte, ucMemType, ucDmaAccessType);

    qspi_wbuf_wait_idle();

    qspi_wait_idle();

    if (ucDmaAccessType)
    {
      qspi_wait_writeburst_done();
    }
  }
}

/**
  * @brief  read flash data.
  * @param  flash_vendor is the QSPI_FLASH_Def pointer for flash driver information.
  * @param  ulSrcAddr is the source address.
  * @param  ulDstAddr is the destination address.
  * @param  ulLenByte is data length of bytes.
  * @param  ulChannelNum is the DMA channel number.
  * @param  ucMemType is the memory type as following:
              MEMORY_TYPE_AP
              MEMORY_TYPE_CP
  * @retval None
  */
void FLASH_ReadData(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum, unsigned char ucMemType)
{
  unsigned long ulBlockNum;
  unsigned long ulLeftByte;
  unsigned long i;
  unsigned char ucDmaAccessType = 1;
  (void)flash_vendor;

  if (ucMemType & 0x80)
  {
    ucDmaAccessType = 0;
  }

  ucMemType = ucMemType & 0x0F;

  DMAChannelConfigure(ulChannelNum, DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_16W | DMAC_CTRL_WORD_SIZE_32b);

  if (ucDmaAccessType)
  {
    qspi_apb_indirect_enable();
  }
  else
  {
    qspi_apb_indirect_disable();
  }

  ulBlockNum = ulLenByte >> 15;

  ulLeftByte = ulLenByte & 0x00007FFF;

  for (i = 0; i < ulBlockNum; i++)
  {

    DMAChannelTransfer_ForFlash(ulChannelNum, ulSrcAddr, ulDstAddr, FLASH_INDIRECT_MAX_BYTE, ucMemType, ucDmaAccessType);

    qspi_rbuf_wait_idle();
    qspi_wait_idle();

    ulDstAddr += FLASH_INDIRECT_MAX_BYTE;
    ulSrcAddr += FLASH_INDIRECT_MAX_BYTE;
  }

  if (ulLeftByte != 0)
  {

    DMAChannelTransfer_ForFlash(ulChannelNum, ulSrcAddr, ulDstAddr, ulLeftByte, ucMemType, ucDmaAccessType);

    qspi_rbuf_wait_idle();
    qspi_wait_idle();
  }
}

void FLASH_OTPReadData(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum, unsigned char ucMemType)
{
    volatile unsigned long reg_value;
    
    reg_value = HWREG(QSPI_REG_RD_INSTR_DEV0);
    
    FLASH_SetReadMode(QSPI_DEV0, QSPI_READ_SECURITY);
    FLASH_ReadData(flash_vendor, ulSrcAddr, ulDstAddr, ulLenByte, ulChannelNum, ucMemType);
    
    FLASH_SetReadMode(QSPI_DEV0, reg_value);
}


void FLASH_OTPWriteData(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum,unsigned char ucMemType)
{
    volatile unsigned long reg_value;
	
    if(ulLenByte >  1024)
    {
         return;
    }
    
    reg_value = HWREG(QSPI_REG_WR_INSTR_DEV0);
    
    FLASH_SetWriteMode(QSPI_DEV0, QSPI_WRITE_SECURITY);
    FLASH_WriteData(flash_vendor, ulSrcAddr, ulDstAddr, ulLenByte, ulChannelNum, ucMemType);

    FLASH_SetWriteMode(QSPI_DEV0, reg_value);

}

void FLASH_OTPUpdateData(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum, unsigned char ucMemType)
{
    volatile unsigned long reg_value1,reg_value2;
    
    reg_value1 = HWREG(QSPI_REG_WR_INSTR_DEV0);
    reg_value2 = HWREG(QSPI_REG_RD_INSTR_DEV0);
    
    FLASH_SetWriteMode(QSPI_DEV0, QSPI_WRITE_SECURITY);
    FLASH_SetReadMode(QSPI_DEV0, QSPI_READ_SECURITY);    

    FLASH_UpdateOtpData(flash_vendor, ulSrcAddr, ulDstAddr, ulLenByte, ulChannelNum, ucMemType);
    FLASH_SetWriteMode(QSPI_DEV0, reg_value1);
    FLASH_SetReadMode(QSPI_DEV0, reg_value2);
}

void FLASH_UpdateOtpData(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum, unsigned char ucMemType)
{
  unsigned long ulExchangeDomain[256];

  if ((ulDstAddr & 0x00000FFF) + ulLenByte > 0x1000)
  {
    return;
  }

  // Copy Destination Data to Buffer
  FLASH_ReadData(flash_vendor, (ulDstAddr & 0xFFFFF000), (unsigned int)ulExchangeDomain, 1024, ulChannelNum, ucMemType);
  FLASH_WaitIdle();

  // Update Special Words in tmp
  DMAChannelTransfer_ForFlash(ulChannelNum, ulSrcAddr, (unsigned int)ulExchangeDomain + (ulDstAddr & 0x00000FFF), ulLenByte, ucMemType, 1);

  // Erase Destination Domain
  FLASH_SectorErase(ulDstAddr);
  FLASH_WaitIdle();

  // Update Flash Destination Domain
  FLASH_WriteData(flash_vendor, (unsigned int)ulExchangeDomain, (ulDstAddr & 0xFFFFF000), 1024, ulChannelNum, ucMemType);
  FLASH_WaitIdle();
}

/**
  * @brief  update flash data with erase and write flash.
  * @param  flash_vendor is the QSPI_FLASH_Def pointer for flash driver information.
  * @param  ulSrcAddr is the source address.
  * @param  ulDstAddr is the destination address.
  * @param  ulLenByte is data length of bytes.
  * @param  ulChannelNum is the DMA channel number.
  * @param  ucMemType is the memory type as following:
              MEMORY_TYPE_AP
              MEMORY_TYPE_CP
  * @retval None
  */
void FLASH_UpdateData(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum, unsigned char ucMemType)
{
  unsigned long ulExchangeDomain[1024];

  if ((ulDstAddr & 0x00000FFF) + ulLenByte > 0x1000)
  {
    return;
  }

  // Copy Destination Data to Buffer
  FLASH_ReadData(flash_vendor, (ulDstAddr & 0xFFFFF000), (unsigned int)ulExchangeDomain, 0x1000, ulChannelNum, ucMemType);
  FLASH_WaitIdle();

  // Update Special Words in tmp
  DMAChannelTransfer_ForFlash(ulChannelNum, ulSrcAddr, (unsigned int)ulExchangeDomain + (ulDstAddr & 0x00000FFF), ulLenByte, ucMemType, 1);

  // Erase Destination Domain
  FLASH_SectorErase(ulDstAddr);
  FLASH_WaitIdle();

  // Update Flash Destination Domain
  FLASH_WriteData(flash_vendor, (unsigned int)ulExchangeDomain, (ulDstAddr & 0xFFFFF000), 0x1000, ulChannelNum, ucMemType);
  FLASH_WaitIdle();
}

/**
  * @brief  fast write flash data with no wait DMA idle.
  * @param  flash_vendor is the QSPI_FLASH_Def pointer for flash driver information.
  * @param  ulSrcAddr is the source address.
  * @param  ulDstAddr is the destination address.
  * @param  ulLenByte is data length of bytes.
  * @param  ulChannelNum is the DMA channel number.
  * @param  ucMemType is the memory type as following:
              MEMORY_TYPE_AP
              MEMORY_TYPE_CP
  * @retval None
  */

void FLASH_FAST_WriteData(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum, unsigned char ucMemType)
{
  while (DMAChannelTransferRemainCNT(ulChannelNum))
    ;

  if (DMAIntStatus(ulChannelNum))
  {
    DMAIntClear(ulChannelNum);
  }

  qspi_wbuf_wait_idle();
  qspi_wait_idle();

  //	// indirect mode
  //	if(HWREG(QSPI_REG_BST_RANGE))
  //	{
  //		qspi_wait_writeburst_done();
  //	}

  FLASH_WaitIdle();

  qspi_apb_indirect_enable();

  qspi_apb_indirect_write(flash_vendor, ulDstAddr, ulLenByte);

  DMAChannelTransferSet(ulChannelNum, (void *)ulSrcAddr, (void *)ulDstAddr, ulLenByte, ucMemType);

  DMAChannelTransferStart(ulChannelNum);
}

/**
 * @brief  flash wait idle.
 * @retval None
 */
void FLASH_WaitIdle(void)
{
  unsigned char flash_cmd;
  unsigned char flash_cmd_rw;

  //    qspi_rbuf_wait_idle();
  //    qspi_wbuf_wait_idle();
  //    qspi_wait_idle();

  flash_cmd = FLASH_CMD_READ_STATUS_REG1;
  flash_cmd_rw = 0x00;
  qspi_apb_command_read(1, &flash_cmd, 1, &flash_cmd_rw);

  while ((flash_cmd_rw & STATUS_REG1_WIP) != 0x00)
  {
    qspi_apb_command_read(1, &flash_cmd, 1, &flash_cmd_rw);
  }
}

void FLASH_WaitIdleNoSus(void)
{
  unsigned char flash_cmd;
  unsigned char flash_cmd_rw[2];

  qspi_rbuf_wait_idle();
  qspi_wbuf_wait_idle();
  qspi_wait_idle();

  do
  {
    flash_cmd = FLASH_CMD_READ_STATUS_REG1;
    flash_cmd_rw[0] = 0x00;
    qspi_apb_command_read(1, &flash_cmd, 1, &flash_cmd_rw[0]);

    flash_cmd = FLASH_CMD_READ_STATUS_REG2;
    flash_cmd_rw[1] = 0x00;
    qspi_apb_command_read(1, &flash_cmd, 1, &flash_cmd_rw[1]);
  } while (((flash_cmd_rw[0] & STATUS_REG1_WIP) != 0x00) || ((flash_cmd_rw[1] & STATUS_REG2_SUS) != 0x00));
}


/* cmp is 0 or 1, see  FLASH_PROTECT_MODE for detail */
void FLASH_SetProtectMode(FLASH_PROTECT_MODE protect_mode, FLASH_PROTECT_CMP cmp)
{
	volatile unsigned char reg1, reg2,unchanged=0;
	unsigned int flash_rdid = 0;
    flash_rdid = FLASH_GetDeviceID();

	/* set cmp */
	reg2 = FLASH_ReadStatusRegIdx(STATUS_REG_S15_S8);
	if(((reg2 >> 6) & 1) != cmp)
	{
		reg2 &= ~(((unsigned char)1) << 6);
		reg2 |= ((unsigned char)cmp & 1) << 6;
	}
	else
	{
	  unchanged += 1;
	}
	/* set BP0 ~ BP4 */
	reg1 = FLASH_ReadStatusRegIdx(STATUS_REG_S7_S0);
	if(((reg1 >> 2) & 0x1F) != protect_mode)
	{
		reg1 &= ~(((unsigned char)0x1F) << 2);
		reg1 |= ((unsigned char)protect_mode & 0x1F) << 2;
	}
	else
	{
	  unchanged += 1;
	}

	if(unchanged>=2)
	{
		return;
	}
	else
	{
	
		if(flash_rdid == 0xC86516)
		{
	      FLASH_WriteStatusRegIdx(STATUS_REG_S7_S0,reg1);
	      FLASH_WaitIdle();
	      FLASH_WriteStatusRegIdx(STATUS_REG_S15_S8,reg2);
		  FLASH_WaitIdle();
	    }
		else
		{
		  FLASH_WriteStatusReg(reg1, reg2);
		  FLASH_WaitIdle();
		}
	}
}

flash_protect FLASH_ProtectDisable(void)
{
    flash_protect  protect = {0};
    protect.mode = FLASH_GetProtectMode();
    protect.cmp = FLASH_GetProtectCmp();
    FLASH_SetProtectMode(FLASH_4M_PROTECT_MODE_0, PROTECT_CMP_0);   
    while(FLASH_GetProtectMode() != FLASH_4M_PROTECT_MODE_0);
    while(FLASH_GetProtectCmp() != PROTECT_CMP_0);  
    return protect;
}

unsigned char FLASH_GetProtectMode(void)
{
    unsigned char mode=0;
    mode = (FLASH_GetStatusReg1()& 0x7C)>>2;
    return mode;
}

unsigned char FLASH_GetProtectCmp(void)
{
    unsigned char cmp=0;
    cmp = (FLASH_GetStatusReg2()& 0x40)>>6;
    return cmp;
}
/**
 * @brief  fast enable the QE flag.
 * @param  flash_vendor is the QSPI_FLASH_Def pointer for flash driver information.
 * @retval None
 */
void FLASH_EnableQEFlag(QSPI_FLASH_Def *flash_vendor)
{
  unsigned char flash_cmd;
  unsigned char flash_cmd_rw[2];
  unsigned int flash_rdid = 0;
  flash_rdid = FLASH_GetDeviceID();
  (void)flash_cmd;
  (void)flash_vendor;

  flash_cmd_rw[1] = FLASH_ReadStatusRegIdx(STATUS_REG_S15_S8);

  if ((flash_cmd_rw[1] & STATUS_REG2_QE) == STATUS_REG2_QE)
  {
    return;
  }

    if(flash_rdid == 0xC86516)  //for flash GD25WQ32E, 9FH: C8 65 16
    {
					// 31H S15-S8
		FLASH_WriteStatusRegIdx(STATUS_REG_S15_S8, (flash_cmd_rw[1] | STATUS_REG2_QE));

    }
	else  //GD25WQ16E, 9FH: C8 65 15     XM25LU32E, 9FH: 20 50 16   
	{
			// Read S7-S0
			flash_cmd_rw[0] = FLASH_ReadStatusRegIdx(STATUS_REG_S7_S0);
			
			// 01H S7-S0 S15-S8
			FLASH_WriteStatusReg(flash_cmd_rw[0], (flash_cmd_rw[1] | STATUS_REG2_QE));
	}
}

/**
 * @brief  flash enter XIP mode.
 * @param  xip_dummy is the XIP mode bits.
 * @retval None
 */
void FLASH_EnterXIPMode(QSPI_FLASH_Def *flash_vendor, char xip_dummy)
{
  volatile unsigned int reg;

  // Disable Direct/Indirect Access

  FLASH_SetReadMode(QSPI_DEV0, QSPI_READ_XIP);

  writeb(xip_dummy, QSPI_REG_MODE_BIT_DEV0);

  /* enter XiP mode and enable direct mode */
  reg = readl(QSPI_REG_CONFIG);
  reg |= QSPI_REG_CONFIG_ENABLE;
  reg |= QSPI_REG_CONFIG_DIRECT;
  reg |= QSPI_REG_CONFIG_XIP_NEXT_READ;

  writel(reg, QSPI_REG_CONFIG);

  flash_vendor_delay(5);

  reg = HWREG(flash_vendor->ahbbase);

  flash_vendor_delay(5);
}

/**
 * @brief  flash exit XIP mode.
 * @retval None
 */
void FLASH_ExitXIPMode(QSPI_FLASH_Def *flash_vendor)
{
  volatile unsigned int reg;

  writeb(0x00, QSPI_REG_MODE_BIT_DEV0);

  reg = readl(QSPI_REG_CONFIG);
  reg |= QSPI_REG_CONFIG_ENABLE;
  reg |= QSPI_REG_CONFIG_DIRECT;
  reg &= (~QSPI_REG_CONFIG_XIP_NEXT_READ);

  writel(reg, QSPI_REG_CONFIG);

  flash_vendor_delay(10);

  reg = HWREG(flash_vendor->ahbbase);

  flash_vendor_delay(10);
}

/**
 * @brief  set flash read mode.
 * @param  devId is the qspi slave device ID.
 * @param  mode is the flash read mode.
 * @retval None
 */
void FLASH_SetReadMode(unsigned char devId, unsigned int mode)
{
  qspi_apb_set_read_instruction(devId, mode);
}

/**
 * @brief  set flash write mode.
 * @param  devId is the qspi slave device ID.
 * @param  mode is the flash write mode.
 * @retval None
 */
void FLASH_SetWriteMode(unsigned char devId, unsigned int mode)
{
  qspi_apb_set_write_instruction(devId, mode);
}

/**
 * @brief  get flash RDID(Read Identification).
 * @retval None
 */
void FLASH_GetRDID(unsigned char *pucRdid)
{
  unsigned char flash_cmd;
  unsigned char flash_cmd_rw[4];

  flash_cmd = FLASH_CMD_READ_DEVICEID;
  flash_cmd_rw[0] = 0x00;
  flash_cmd_rw[1] = 0x00;
  flash_cmd_rw[2] = 0x00;
  qspi_apb_command_read(1, &flash_cmd, 3, &flash_cmd_rw[0]);

  pucRdid[0] = flash_cmd_rw[0];
  pucRdid[1] = flash_cmd_rw[1];
  pucRdid[2] = flash_cmd_rw[2];
}

/**
 * @brief  erase flash security data with address.
 * @param  ulEraseAddr is the erase data destination address.
 * @retval None
 */
void FLASH_SecurityErase(unsigned long ulEraseAddr)
{
  unsigned char flash_cmd[4];
  unsigned char flash_cmd_rw;

  FLASH_WriteEnable();

  flash_cmd[0] = FLASH_CMD_SECURITY_ERASE;
  flash_cmd[1] = (ulEraseAddr >> 16) & 0xFF;
  flash_cmd[2] = (ulEraseAddr >> 8) & 0xFF;
  flash_cmd[3] = (ulEraseAddr >> 0) & 0xFF;

  qspi_apb_command_write(4, &flash_cmd[0], 0, &flash_cmd_rw);
}

/**
  * @brief  get flash UniqueID 128 bits.
  * @param  flash_vendor is the QSPI_FLASH_Def pointer for flash driver information.
  * @param  pucUniqueId is the UniqueID data pointer.
  * @param  ucMemType is the memory type as following:
              MEMORY_TYPE_AP
              MEMORY_TYPE_CP
  * @retval None
  */
void FLASH_GetUniqueID128(QSPI_FLASH_Def *flash_vendor, unsigned char *pucUniqueId, unsigned char ucMemType)
{
  volatile unsigned long reg_val;

  reg_val = HWREG(QSPI_REG_RD_INSTR_DEV0);

  HWREG(QSPI_REG_RD_INSTR_DEV0) = QSPI_READ_UID;

  FLASH_ReadData(flash_vendor, QSPI_DATA_BASE, (unsigned long)pucUniqueId, 16, FLASH_DMA_CHANNEL, ucMemType);
  FLASH_WaitIdle();

  HWREG(QSPI_REG_RD_INSTR_DEV0) = reg_val;
}

/**
 * @brief  get the flash DeviceID.
 * @retval the flash DeviceID
 */
unsigned long FLASH_GetDeviceID(void)
{
  unsigned char flash_cmd;
  unsigned char flash_cmd_rw[4];

  flash_cmd = FLASH_CMD_READ_DEVICEID;
  flash_cmd_rw[0] = 0x00;
  flash_cmd_rw[1] = 0x00;
  flash_cmd_rw[2] = 0x00;
  qspi_apb_command_read(1, &flash_cmd, 3, &flash_cmd_rw[0]);

  return (flash_cmd_rw[0] << 16 | flash_cmd_rw[1] << 8 | flash_cmd_rw[2]);
}


void FLASH_LowPowerEnter(void)
{
    unsigned char flash_cmd;
    unsigned char flash_cmd_rw;
    
    flash_cmd = FLASH_CMD_LOW_POWER_ENTER;
    qspi_apb_command_write(1, &flash_cmd, 0, &flash_cmd_rw);
}

extern int qspi_apb_exec_flash_cmd(unsigned int reg);
void FLASH_LowPowerEnter_Ext(void)
{
    unsigned int reg = FLASH_CMD_LOW_POWER_ENTER << QSPI_REG_CMDCTRL_OPCODE_LSB;
    qspi_apb_exec_flash_cmd(reg);
}

void FLASH_LowPowerExit(void)
{
    unsigned char flash_cmd;
    unsigned char flash_cmd_rw;
    
    flash_cmd = FLASH_CMD_LOW_POWER_EXIT;
    qspi_apb_command_write(1, &flash_cmd, 0, &flash_cmd_rw);
}

void FLASH_LowPowerExit_Ext(void)
{
    unsigned int reg = FLASH_CMD_LOW_POWER_EXIT << QSPI_REG_CMDCTRL_OPCODE_LSB;
    qspi_apb_exec_flash_cmd(reg);
}
/**
 * @brief  suspend the programing or erasing flash.
 * @retval None
 */
void FLASH_PE_Suspend(void)
{
  unsigned char flash_cmd;
  unsigned char flash_cmd_rw;

  flash_cmd = FLASH_CMD_PE_SUSPEND;
  qspi_apb_command_write(1, &flash_cmd, 0, &flash_cmd_rw);
}

/**
 * @brief  resume the program or erase flash.
 * @retval None
 */
void FLASH_PE_Resume(void)
{
  unsigned char flash_cmd;
  unsigned char flash_cmd_rw;

  flash_cmd = FLASH_CMD_PE_RESUME;
  qspi_apb_command_write(1, &flash_cmd, 0, &flash_cmd_rw);
}

unsigned char FLASH_isXIPMode(void)
{
  return ((HWREG(QSPI_BASE) & 0x20) ? 1 : 0);
}

unsigned char FLASH_need_Resume(QSPI_FLASH_Def *flash_vendor)
{
  volatile unsigned char flashStatusReg2 = 0;
  volatile unsigned char SUS1 = 0, SUS2 = 0;
  unsigned int flash_rdid = 0;
  flash_rdid = FLASH_GetDeviceID();
  (void)flash_vendor;

  flashStatusReg2 = FLASH_GetStatusReg2();
    if(flash_rdid == 0xC86516)  //for flash GD25WQ32E, 9FH: C8 65 16
    {
			SUS1 = (flashStatusReg2 & 0x80);
			SUS2 = (flashStatusReg2 & 0x04);
			if (SUS1 == 0x80 || SUS2 == 0x04) {
					return 1;
			} else {
					return 0;
			}
		}
		else  //for flash GD25WQ16E, 9FH: C86515，XM25Q32
		{
		  SUS1 = (flashStatusReg2 & 0x80);
			if (SUS1 == 0x80) {
					return 1;
			} else {
					return 0;
			}
		}
}

/**
 * @brief  get the flash status register1.
 * @retval None
 */
unsigned char FLASH_GetStatusReg1(void)
{
  unsigned char flash_cmd;
  unsigned char flash_cmd_rw;

  flash_cmd = FLASH_CMD_READ_STATUS_REG1;
  flash_cmd_rw = 0x00;
  qspi_apb_command_read(1, &flash_cmd, 1, &flash_cmd_rw);
  return flash_cmd_rw;
}

/**
 * @brief  get the flash status register2.
 * @retval None
 */
unsigned char FLASH_GetStatusReg2(void)
{
  unsigned char flash_cmd;
  unsigned char flash_cmd_rw;

  flash_cmd = FLASH_CMD_READ_STATUS_REG2;
  flash_cmd_rw = 0x00;
  qspi_apb_command_read(1, &flash_cmd, 1, &flash_cmd_rw);
  return flash_cmd_rw;
}

/**
 * @brief  enable the psram reset.
 * @retval None
 */
void PSRAM_ResetEnable(void)
{
  unsigned char flash_cmd[4];
  unsigned char flash_cmd_rw;

  flash_cmd[0] = PSRAM_CMD_RSTEN;

  qspi_apb_command_write(1, &flash_cmd[0], 0, &flash_cmd_rw);
}

/**
 * @brief  reset the psram.
 * @retval None
 */
void PSRAM_Reset(void)
{
  unsigned char flash_cmd[4];
  unsigned char flash_cmd_rw;

  flash_cmd[0] = PSRAM_CMD_RST;

  qspi_apb_command_write(1, &flash_cmd[0], 0, &flash_cmd_rw);
}

/**
 * @brief  flash enter XIP mode in qspi multiple device mode.
 * @retval None
 */
void Multiple_EnterXIPMode(void)
{
  volatile unsigned int reg;

  FLASH_SetReadMode(QSPI_DEV0, QSPI_READ_XIP);

  HWREG(QSPI_BASE + 0x38) = 0xA0; // mode bits

  HWREGB(QSPI_BASE + 0x41) = 0x0F; // DAC XIP control
  flash_vendor_delay(10);

  reg = HWREG(QSPI_DATA_BASE);
  (void)reg;

  flash_vendor_delay(100);

  HWREGB(QSPI_BASE + 0x41) = 0x1F; // DAC XIP control

  flash_vendor_delay(100);
}

/**
 * @brief  flash exit XIP mode in qspi multiple device mode.
 * @retval None
 */
void Multiple_ExitXIPMode(void)
{
  volatile unsigned int reg;

  HWREG(QSPI_BASE + 0x38) = 0x00; // mode bits
  flash_vendor_delay(10);

  reg = HWREG(QSPI_DATA_BASE);
  (void)reg;
  flash_vendor_delay(10);

  HWREGB(QSPI_BASE + 0x41) = 0x0F; // DAC XIP control
  flash_vendor_delay(10);

  FLASH_SetReadMode(QSPI_DEV0, QSPI_READ_QUAD);
  FLASH_SetWriteMode(QSPI_DEV0, QSPI_WRITE_QUAD);
  qspi_wait_idle();
}

/**
 * @brief  psram hardware init.
 * @retval None
 */
void PSRAM_Init(void)
{
  //*****************************************************************************
  // QSPI Baudrate Setting and PSRAM initialization.
  // sclk = 1/4 ref_clk
  // ss_out:
  //  0:flash0, 1:psram0 2: psram1 3: flash1
  // device range:
  //  0x30000000~0x303FFFFF: flash0
  //  0x30400000~0x305FFFFF: psram0
  //*****************************************************************************
  HWREG(QSPI_BASE) &= ~(1 << 8);                                       // 0: only select one device which control by dev_cs signals;  1: can select multiple device
  HWREG(QSPI_BASE) = (HWREG(QSPI_BASE) & ~(0x0F << 12)) | (0x1 << 12); // select device1
  flash_vendor_delay(100);

  // TCEMAX count Register
  HWREG(QSPI_BASE + 0x3C) = 80; // Define the number of io_clk cycles to de-assert chip select when CS# active

  HWREGB(QSPI_BASE + 0x40) = 0x0D; // DEV_CTRL

  HWREGB(QSPI_BASE + 0x42) = 0x6F;  // read predicted
  HWREG(QSPI_BASE + 0x34) = 0x0F00; // device size

  HWREG(QSPI_BASE) |= (1 << 2); // enable AHB decoder

  flash_vendor_delay(100);

  HWREGB(QSPI_BASE + 0x41) = 0x06; // multi_rdinstr_en  | multi_wrinstr_en

  // HWREG(QSPI_BASE + 0x28) |= (1<<25);  //write disable polling

  HWREGB(QSPI_BASE + 0x01) = (HWREGB(QSPI_BASE + 0x01) & 0xF9) | 0x02; // set default instruction is 1
  flash_vendor_delay(100);

  PSRAM_ResetEnable();
  PSRAM_Reset();

  FLASH_SetReadMode(QSPI_DEV1, PSRAM_QUAD_READ);
  FLASH_SetWriteMode(QSPI_DEV1, PSRAM_QUAD_WRITE);

  HWREG(QSPI_BASE) = (HWREG(QSPI_BASE) & ~(0x0F << 12)); // device CS

  HWREGB(QSPI_BASE + 0x01) = (HWREGB(QSPI_BASE + 0x01) & 0xF9); // set default instruction is 0

  flash_vendor_delay(100);
}

/**
 * @brief  poweron flash vcc and delay
 * @retval None
 */
void Flash_Poweron_Delay(void)
{
    //if not powered on,power flash and delay
    if((AONPRCM->FLASHPMUX_CFG & 0x07) != 0x2)
	{
		PRCM_FLASH_VCC_On();
        //different flash type,need different flash delay cnt
        //for GD32,flash vcc off -->> on,delay 8ms
        utc_cnt_delay(10000/30);//10ms/30us
        //Reset qspi controller reg;
	    HWREGB(COREPRCM_BASE + 0x04) |= 0x4;
        utc_cnt_delay(1);
	    HWREGB(COREPRCM_BASE + 0x04) &= ~0x4;
        utc_cnt_delay(1);
        FLASH_Init((QSPI_FLASH_Def *)&xinyi_flash, 38400000, 9600000);
	    FLASH_SetReadMode(QSPI_DEV0, QSPI_READ_QUAD);
	    FLASH_SetWriteMode(QSPI_DEV0, QSPI_WRITE_QUAD);
	    utc_cnt_delay(2);//
	}
	
}

/**
 * @brief  poweroff flash vcc
 * @retval None
 */
void Flash_Poweroff(void)
{
    PRCM_FLASH_VCC_Off();
}