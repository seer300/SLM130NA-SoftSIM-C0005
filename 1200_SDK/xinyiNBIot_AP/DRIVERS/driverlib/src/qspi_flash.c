#include "qspi_flash.h"
#include <stddef.h>

/**
  * @brief  qspi memcpy.
  * @param  dest is the destination address.
  * @param  src is the source address.
  * @param  size is the length of data bytes.
  * @retval None
  */
void *qspi_memcpy(unsigned char *dest, const unsigned char *src, size_t size)
{
	unsigned char *dptr = dest;
	const unsigned char *ptr = src;
	const unsigned char *end = src + size;

	while (ptr < end)
		*dptr++ = *ptr++;

	return dest;
}

/**
  * @brief  qspi memset.
  * @param  inptr is the start address with pointer.
  * @param  ch is the setting format data.
  * @param  size is the length of data bytes.
  * @retval None
  */
void *qspi_memset(char *inptr, int ch, size_t size)
{
	char *ptr = inptr;
	char *end = ptr + size;

	while (ptr < end)
		*ptr++ = ch;

	return ptr;
}

/**
  * @brief  convert command to address
  * @param  addr_buf is the address buffer.
  * @param  addr_width is the address width.
  * @retval address
  */
static unsigned int qspi_apb_cmd2addr(const unsigned char *addr_buf, unsigned int addr_width)
{
	unsigned int addr;

	addr = (addr_buf[0] << 16) | (addr_buf[1] << 8) | addr_buf[2];

	if (addr_width == 4)
		addr = (addr << 8) | addr_buf[3];

	return addr;
}

/**
  * @brief  enable the qspi controller.
  * @retval None
  */
void qspi_apb_controller_enable(void)
{
	unsigned int reg;
	reg = readl(QSPI_REG_CONFIG);
	reg |= QSPI_REG_CONFIG_ENABLE;
	writel(reg, QSPI_REG_CONFIG);
}

/**
  * @brief  disable the qspi controller.
  * @retval None
  */
void qspi_apb_controller_disable(void)
{
	unsigned int reg;
	reg = readl(QSPI_REG_CONFIG);
	reg &= ~QSPI_REG_CONFIG_ENABLE;
	writel(reg, QSPI_REG_CONFIG);
}

void qspi_wait_indirect_complete(void)
{
    while(!(HWREG(QSPI_BASE + 0x6C) & 0x20));  //wait indirect complete status

    HWREG(QSPI_BASE + 0x6C) |=0x20;  //clear indirect complete status
}

/**
  * @brief  qspi wait idle.
  * @retval None
  */
void qspi_wait_idle(void)
{
	while(QSPI_REG_IS_IDLE() == 0)
    {
    }
}

/**
  * @brief  qspi read buffer wait idle.
  * @retval None
  */
void qspi_rbuf_wait_idle(void)
{
	while(readb(QSPI_REG_RBUF_FILL_LEVEL));
}

/**
  * @brief  qspi write buffer wait idle.
  * @retval None
  */
void qspi_wbuf_wait_idle(void)
{
	while(readb(QSPI_REG_WBUF_FILL_LEVEL));
}

/**
  * @brief  qspi write write burst done, and clear the done flag.
  * @retval None
  */
void qspi_wait_writeburst_done(void)
{
    while(!(HWREGB(QSPI_REG_WR_BST_CTRL) & 0x20));
    
    HWREGB(QSPI_REG_WR_BST_CTRL) |= 0x20;
}

/**
  * @brief  config the qspi baudrate.
  * @param  ref_clk_hz is the reference clock.
  * @param  sclk_hz is the QSPI clock rate.
  * @retval None
  */
void qspi_apb_config_baudrate_div(unsigned int ref_clk_hz, unsigned int sclk_hz)
{
	unsigned int reg;
	unsigned int div;

	qspi_apb_controller_disable();
	reg = readl(QSPI_REG_CONFIG);
	reg &= ~(QSPI_REG_CONFIG_BAUD_MASK << QSPI_REG_CONFIG_BAUD_LSB);

	/*
	 * The baud_div field in the config reg is 4 bits, and the ref clock is
	 * divided by 2 * (baud_div + 1). Round up the divider to ensure the
	 * SPI clock rate is less than or equal to the requested clock rate.
	 */
	div = DIV_ROUND_UP(ref_clk_hz, sclk_hz * 2) - 1;

	/* ensure the baud rate doesn't exceed the max value */
	if (div > QSPI_REG_CONFIG_BAUD_MASK)
		div = QSPI_REG_CONFIG_BAUD_MASK;

	//debug("%s: ref_clk %dHz sclk %dHz Div 0x%x, actual %dHz\n", __func__,
	//      ref_clk_hz, sclk_hz, div, ref_clk_hz / (2 * (div + 1)));

	reg |= (div << QSPI_REG_CONFIG_BAUD_LSB);
	writel(reg, QSPI_REG_CONFIG);

	qspi_apb_controller_enable();
}

/**
  * @brief  config the qspi delay.
  * @param  ref_clk_hz is the reference clock.
  * @param  sclk_hz is the QSPI clock rate.
  * @param  tshsl_ns is the added delay in io_clk for the length that the master mode chip select outputs are de-asserted between transactions.
  * @param  tsd2d_ns is the delay cycles of chip select de-Assert different slaves delay between one chip select being de-activated and the activation of another.
  * @param  tchsh_ns is the delay cycles of chip select end of transfer delay between last bit of current transaction and de-asserting the device chip select.
  * @param  tslch_ns is the delay cycles of chip select start transfer in master reference clocks between setting n_ss_out low and first bit transfer.
  * @retval None
  */
void qspi_apb_delay(unsigned int ref_clk, unsigned int sclk_hz, unsigned int tshsl_ns, unsigned int tsd2d_ns, unsigned int tchsh_ns, unsigned int tslch_ns)
{
	unsigned int ref_clk_ns;
	unsigned int sclk_ns;
	unsigned int tshsl, tchsh, tslch, tsd2d;
	unsigned int reg;

	qspi_apb_controller_disable();

	/* Convert to ns. */
	ref_clk_ns = DIV_ROUND_UP(1000000000, ref_clk);

	/* Convert to ns. */
	sclk_ns = DIV_ROUND_UP(1000000000, sclk_hz);

	/* The controller adds additional delay to that programmed in the reg */
	if (tshsl_ns >= sclk_ns + ref_clk_ns)
		tshsl_ns -= sclk_ns + ref_clk_ns;
	if (tchsh_ns >= sclk_ns + 3 * ref_clk_ns)
		tchsh_ns -= sclk_ns + 3 * ref_clk_ns;
	tshsl = DIV_ROUND_UP(tshsl_ns, ref_clk_ns);
	tchsh = DIV_ROUND_UP(tchsh_ns, ref_clk_ns);
	tslch = DIV_ROUND_UP(tslch_ns, ref_clk_ns);
	tsd2d = DIV_ROUND_UP(tsd2d_ns, ref_clk_ns);

	reg = ((tshsl & QSPI_REG_DELAY_TSHSL_MASK)
			<< QSPI_REG_DELAY_TSHSL_LSB);
	reg |= ((tchsh & QSPI_REG_DELAY_TCHSH_MASK)
			<< QSPI_REG_DELAY_TCHSH_LSB);
	reg |= ((tslch & QSPI_REG_DELAY_TSLCH_MASK)
			<< QSPI_REG_DELAY_TSLCH_LSB);
	reg |= ((tsd2d & QSPI_REG_DELAY_TSD2D_MASK)
			<< QSPI_REG_DELAY_TSD2D_LSB);
	writel(reg, QSPI_REG_DELAY);

    //HWREGB(QSPI_BASE + 0x08) = 1;  //loopback enable

    //HWREGB(QSPI_BASE + 0x09) = 3;  //3  //delay the read data capturing logic

	qspi_apb_controller_enable();
}

/**
  * @brief  init qspi controller.
  * @param  plat is the QSPI_FLASH_Def pointer for flash driver information.
  * @retval None
  */
void qspi_apb_controller_init(QSPI_FLASH_Def *plat)
{
	unsigned reg;

	qspi_apb_controller_disable();

	/* Configure the device size and address bytes */
	reg = readl(QSPI_REG_SIZE);
	/* Clear the previous value */
	reg &= ~((unsigned long)QSPI_REG_SIZE_PAGE_MASK << QSPI_REG_SIZE_PAGE_LSB);
	reg &= ~(QSPI_REG_SIZE_BLOCK_MASK << QSPI_REG_SIZE_BLOCK_LSB);
	reg &= ~QSPI_REG_SIZE_ADDRESS_MASK;
	reg |= (plat->page_size << QSPI_REG_SIZE_PAGE_LSB);
	reg |= (plat->block_size << QSPI_REG_SIZE_BLOCK_LSB);
	reg |= (plat->addr_bytes - 1);
	writel(reg, QSPI_REG_SIZE);

	/* Configure the remap address register, no remap */
	writel(0, QSPI_REG_REMAP);

	/* Indirect mode configurations */
//	writel((plat->sram_size/2), plat->regbase + CQSPI_REG_SRAMPARTITION);

	/* Disable all interrupts */
	writel(0, QSPI_REG_IRQMASK);

	qspi_apb_controller_enable();
}

/**
  * @brief  execute the qspi control flash command.
  * @param  reg is the control command.
  * @retval None
  */
int qspi_apb_exec_flash_cmd(unsigned int reg)
{
	/* Write the CMDCTRL without start execution. */
	writel(reg, QSPI_REG_CMDCTRL);
	/* Start execute */
	reg |= QSPI_REG_CMDCTRL_EXECUTE;
	writel(reg, QSPI_REG_CMDCTRL);

	reg = readl(QSPI_REG_CMDCTRL);
	while((reg & QSPI_REG_CMDCTRL_INPROGRESS) != 0)
    {
        reg = readl(QSPI_REG_CMDCTRL);
    }

	/* Polling QSPI idle status. */
	qspi_wait_idle();

	return 0;
}

/**
  * @brief  read the flash control information with qspi control flash command.
            For commands RDID, RDSR, etc.
  * @param  cmdlen is the control command length.
  * @param  cmdbuf is the control command buffer.
  * @param  rxlen is the rx buffer length.
  * @param  rxbuf is the rx buffer.
  * @retval 0:success, or status of execute command.
  */
int qspi_apb_command_read(unsigned int cmdlen, const uint8_t *cmdbuf, unsigned int rxlen, uint8_t *rxbuf)
{
	unsigned int reg;
	unsigned int read_len;
	int status;

	if (!cmdlen || rxlen > QSPI_STIG_DATA_LEN_MAX || rxbuf == NULL) {
		//printf("QSPI: Invalid input arguments cmdlen %d rxlen %d\n",
		//       cmdlen, rxlen);
		return -22;
	}

	reg = cmdbuf[0] << QSPI_REG_CMDCTRL_OPCODE_LSB;

	reg |= (0x1 << QSPI_REG_CMDCTRL_RD_EN_LSB);

	/* 0 means 1 byte. */
	reg |= (((rxlen - 1) & QSPI_REG_CMDCTRL_RD_BYTES_MASK)
		<< QSPI_REG_CMDCTRL_RD_BYTES_LSB);
	status = qspi_apb_exec_flash_cmd(reg);
	if (status != 0)
		return status;

	reg = readl(QSPI_REG_CMDREADDATALOWER);

	/* Put the read value into rx_buf */
	read_len = (rxlen > 4) ? 4 : rxlen;
	qspi_memcpy(rxbuf, (unsigned char *)&reg, read_len);
	rxbuf += read_len;

	if (rxlen > 4) {
		reg = readl(QSPI_REG_CMDREADDATAUPPER);

		read_len = rxlen - read_len;
		qspi_memcpy(rxbuf, (unsigned char *)&reg, read_len);
	}
	return 0;
}

/**
  * @brief  write the flash control information with qspi control flash command.
            For commands: WRSR, WREN, WRDI, CHIP_ERASE, BE, etc.
  * @param  cmdlen is the control command length.
  * @param  cmdbuf is the control command buffer.
  * @param  txlen is the tx buffer length.
  * @param  txbuf is the tx buffer.
  * @retval status of execute command.
  */
int qspi_apb_command_write(unsigned int cmdlen, const uint8_t *cmdbuf, unsigned int txlen,  const uint8_t *txbuf)
{
	unsigned int reg = 0;
	unsigned int addr_value;
	unsigned int wr_data;
	unsigned int wr_len;

	if (!cmdlen || cmdlen > 5 || txlen > 8 || cmdbuf == NULL) {
		//printf("QSPI: Invalid input arguments cmdlen %d txlen %d\n",
		//       cmdlen, txlen);
		return -22;
	}

	reg |= cmdbuf[0] << QSPI_REG_CMDCTRL_OPCODE_LSB;

	if (cmdlen == 4 || cmdlen == 5) {
		/* Command with address */
		reg |= (0x1 << QSPI_REG_CMDCTRL_ADDR_EN_LSB);
		/* Number of bytes to write. */
		reg |= ((cmdlen - 2) & QSPI_REG_CMDCTRL_ADD_BYTES_MASK)
			<< QSPI_REG_CMDCTRL_ADD_BYTES_LSB;
		/* Get address */
		addr_value = qspi_apb_cmd2addr(&cmdbuf[1],
			cmdlen >= 5 ? 4 : 3);

		writel(addr_value, QSPI_REG_CMDADDRESS);
	}

	if (txlen) {
		/* writing data = yes */
		reg |= (0x1 << QSPI_REG_CMDCTRL_WR_EN_LSB);
		reg |= ((txlen - 1) & QSPI_REG_CMDCTRL_WR_BYTES_MASK)
			<< QSPI_REG_CMDCTRL_WR_BYTES_LSB;

		wr_len = txlen > 4 ? 4 : txlen;
		qspi_memcpy((unsigned char *)&wr_data, txbuf, wr_len);
		writel(wr_data, QSPI_REG_CMDWRITEDATALOWER);

		if (txlen > 4) {
			txbuf += wr_len;
			wr_len = txlen - wr_len;
			qspi_memcpy((unsigned char *)&wr_data, txbuf, wr_len);
			writel(wr_data, QSPI_REG_CMDWRITEDATAUPPER);
		}
	}

	/* Execute the command */
	return qspi_apb_exec_flash_cmd(reg);
}

/**
  * @brief  enable qspi indirect mode.
  * @retval None.
  */
void qspi_apb_indirect_enable(void)
{
    /* Setup the indirect trigger range */
    writel(0x0F, QSPI_REG_BST_RANGE);
}

/**
  * @brief  disable qspi indirect mode.
  * @retval None.
  */
void qspi_apb_indirect_disable(void)
{
    /* ZI the indirect trigger range */
    writel(0x00, QSPI_REG_BST_RANGE);
}

/**
  * @brief  config write data to flash with indirect mode.
  * @param  plat is the QSPI_FLASH_Def pointer for flash driver information.
  * @param  dst is the destination address.
  * @param  n_bytes is the indirect write transfer bytes.
  * @retval None
  */
void qspi_apb_indirect_write(QSPI_FLASH_Def *plat, unsigned int dst, unsigned int n_bytes)
{
    unsigned int addr = dst - (unsigned int)plat->ahbbase;
    
    /* Setup the indirect trigger address */
    writel((uint32_t)(dst & 0x00FFFFFC), QSPI_REG_BST_BASE_ADDR);

    /* Configure the indirect write transfer bytes */
    writel(n_bytes, QSPI_REG_WR_BST_NUM);

    /* Setup write address */
    writel(addr, QSPI_REG_WR_BST_MEM_ADDR);

    /* Start the indirect write transfer */
    writel(QSPI_REG_WR_BST_CTRL_START, QSPI_REG_WR_BST_CTRL);
}

/**
  * @brief  config read data from flash with indirect mode.
  * @param  plat is the QSPI_FLASH_Def pointer for flash driver information.
  * @param  src is the source address.
  * @param  n_bytes is the indirect read transfer bytes.
  * @retval None
  */
void qspi_apb_indirect_read(QSPI_FLASH_Def *plat, unsigned int src, unsigned int n_bytes)
{
    unsigned int addr = src - (unsigned int)plat->ahbbase;

    /* Setup the indirect trigger address */
    writel((uint32_t)(src & 0x00FFFFFC), QSPI_REG_BST_BASE_ADDR);
    
    /* Configure the indirect read transfer bytes */
    writel(n_bytes, QSPI_REG_RD_BST_NUM);

    /* Setup read address */
    writel(addr, QSPI_REG_RD_BST_MEM_ADDR);

    /* Start the indirect read transfer */
    writel(QSPI_REG_RD_BST_CTRL_START, QSPI_REG_RD_BST_CTRL);
}

/**
  * @brief  config qspi read instruction.
  * @param  devId is the qspi slave device ID.
  * @param  config is the Read Instruction Register value.
  * @retval None
  */
void qspi_apb_set_read_instruction(unsigned char devId, unsigned int config)
{
    /* Device Read Instruction Register */
	writel(config, QSPI_REG_RD_INSTR_DEV0 + (devId<<2));
}

/**
  * @brief  config qspi write instruction.
  * @param  devId is the qspi slave device ID.
  * @param  config is the Write Instruction Register value.
  * @retval None
  */
void qspi_apb_set_write_instruction(unsigned char devId, unsigned int config)
{
    /* Device Write Instruction Register */
    writel(config, QSPI_REG_WR_INSTR_DEV0 + (devId<<2));
}

/**
  * @brief  Registers an interrupt handler for qspi interrupt.
  * @param  g_pRAMVectors is the global interrupt vectors table.
  * @param  pfnHandler is a pointer to the function to be called when the qspi interrupt occurs.
  * @retval None
  */
void QSPIIntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
    IntRegister(INT_QSPI, g_pRAMVectors, pfnHandler);

    IntEnable(INT_QSPI);
}

/**
  * @brief  Unregisters an interrupt handler for the qspi interrupt.
  * @param  g_pRAMVectors is the global interrupt vectors table.
  * @retval None
  */
void QSPIIntUnregister(unsigned long *g_pRAMVectors)
{
    IntDisable(INT_QSPI);

    IntUnregister(INT_QSPI, g_pRAMVectors);
}

/**
  * @brief  config the qspi DMA ctrl.
  * @param  ucConfig is enable the DMA or disable the DMA.
  * @retval None
  */
void qspi_dma_config(unsigned char ucConfig)
{
    if(ucConfig) {
        HWREG(QSPI_REG_CONFIG) |= QSPI_REG_CONFIG_DMA;
    } else {
        HWREG(QSPI_REG_CONFIG) &= ~QSPI_REG_CONFIG_DMA;
    }
}

/**
  * @brief  set the DMA burst size.
  * @param  ucBstsize is the DMA burst size.
            The size is : 2^dma_bst_size bytes
  * @retval None
  */
void qspi_dma_bstsize_set(unsigned char ucBstsize)
{
    HWREGB(QSPI_REG_DMA_BST_SIZE) = ucBstsize;
}

/**
  * @brief  get the DMA burst size.
          The size is : 2^dma_bst_size bytes
  * @retval the DMA burst size
  */
unsigned char qspi_dma_bstsize_get(void)
{
    return HWREGB(QSPI_REG_DMA_BST_SIZE);
}

/**
  * @brief  enable the function of suspend commend check.
  * @retval None
  */
void qspi_sus_cmd_check_enable(void)
{
    HWREGB(QSPI_SUS_CTRL) = SUS_CTRL_CMD_CHECK_EN;
}

/**
  * @brief  set qspi suspend command.
  * @param  sus_cmd is the qspi suspend command.
  * @retval None
  */
void qspi_sus_cmd_set(unsigned int sus_cmd)
{
    HWREG(QSPI_SUS_CMD0) = sus_cmd;
}

/**
  * @brief  set qspi suspend cs.
  * @param  sus_cs is the qspi suspend cs.
  * @retval None
  */
void qspi_sus_cs_set(unsigned char sus_cs)
{
    HWREG(QSPI_SUS_CTRL) = (HWREG(QSPI_SUS_CTRL) & ~SUS_CTRL_CS_Msk) | (sus_cs << SUS_CTRL_CS_Pos);
}

/**
  * @brief  config qspi block all devices.
  * @param  flag of the qspi block all devices.
  * @retval None
  */
void qspi_block_alldevice_set(unsigned char flag)
{
    HWREG(QSPI_SUS_CTRL) = (HWREG(QSPI_SUS_CTRL) & ~SUS_CTRL_BLOCK_ALLDEVICE_Msk) | (flag << SUS_CTRL_BLOCK_ALLDEVICE_Pos);
}

/**
  * @brief  get qspi block all devices flag.
  * @retval the flag of the qspi block all devices.
  */
unsigned char qspi_block_alldevice_get(void)
{
    return (((HWREG(QSPI_SUS_CTRL) & SUS_CTRL_BLOCK_ALLDEVICE_Msk) == SUS_CTRL_BLOCK_ALLDEVICE_Msk)? 1:0);
}

/**
  * @brief  set qspi tsus count.
  * @param  tresume_cnt is the qspi tresume count.
  * @param  tsus_cnt is the qspi tsus count.
  * @retval None
  */
void qspi_tsus_cnt_set(unsigned char tresume_cnt, unsigned int tsus_cnt)
{
    HWREG(QSPI_TSUS_CNT) = (tresume_cnt << 24) | (tsus_cnt & 0x00FFFFFF);
}

/**
  * @brief  config qspi suspend operation code.
  * @param  sus_opcode is the qspi suspend operation code.
  * @retval None
  */
void qspi_sus_opcode_set(unsigned char sus_opcode)
{
    HWREGB(QSPI_SUS_OPCODE) = sus_opcode;
}

/**
  * @brief  config qspi resume operation code.
  * @param  resume_opcode is the qspi resume operation code.
  * @retval None
  */
void qspi_resume_opcode_set(unsigned char resume_opcode)
{
    HWREGB(QSPI_RESUME_OPCODE) = resume_opcode;
}

/**
  * @brief  config the qspi suspend timeout count.
  * @param  tsus_timeout_cnt is the qspi suspend timeout count.
  * @retval None
  */
void qspi_tsus_timeout_cnt_set(unsigned char tsus_timeout_cnt)
{
    HWREG(QSPI_TSUS_TIMEOUT_CNT) = tsus_timeout_cnt;
}

/**
  * @brief  clear the suspend commend hit event flag.
  * @retval None
  */
void qspi_clear_sus_cmd_hit(void)
{
    HWREG(QSPI_SUS_CTRL) |= SUS_CTRL_SUSPEND_CMD_HIT;

    while((HWREG(QSPI_SUS_CTRL) & SUS_CTRL_SUSPEND_CMD_HIT) == SUS_CTRL_SUSPEND_CMD_HIT);
}





void qspi_ap_erase_write_req(void)
{
	HWREGB(QSPI_SUS_AP_CTRL0) |= 0x01;
}

void qspi_ap_dma_erase_write_req(void)
{
	HWREGB(QSPI_SUS_AP_CTRL0) |= 0x02;
}

void qspi_ap_wait_erase_write_req_ack(void)
{
	while(!(HWREGB(QSPI_SUS_AP_INT_STAT0) & 0x02));
}

void qspi_cp_send_erase_write_req_ack(void)
{
	HWREGB(QSPI_SUS_CP_CTRL2) |= 0x01;
}

void qspi_cp_wait_erase_write_done(void)
{
	while(!(HWREGB(QSPI_SUS_CP_INT_STAT0) & 0x04));
}

void qspi_ap_send_erase_write_done(void)
{
	HWREGB(QSPI_SUS_AP_CTRL3) |= 0x01;
}

void qspi_cp_erase_write_req(void)
{
	HWREGB(QSPI_SUS_CP_CTRL0) |= 0x01;
}

void qspi_ap_send_erase_write_req_ack(void)
{
	HWREGB(QSPI_SUS_AP_CTRL2) |= 0x01;
}

void qspi_ap_wait_erase_write_done(void)
{
	while(!(HWREGB(QSPI_SUS_AP_INT_STAT0) & 0x04));
}

void qspi_cp_wait_erase_write_req_ack(void)
{
	while(!(HWREGB(QSPI_SUS_CP_INT_STAT0) & 0x02));
}

void qspi_cp_send_erase_write_done(void)
{
	HWREGB(QSPI_SUS_CP_CTRL3) |= 0x01;
}
