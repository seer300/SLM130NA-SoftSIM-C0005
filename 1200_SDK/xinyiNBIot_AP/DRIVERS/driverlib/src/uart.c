#include "csp.h"
#include "uart.h"
#include "interrupt.h"
#include <stdio.h>
#include <stdarg.h>

/**
  * @brief  Enable the g_at_lpuart module.
  * @param  ulBase: the base address of g_at_lpuart module
  * @retval None
  */
void UARTEnable(unsigned long ulBase)
{
	HWREGB(ulBase + UART_CTL) |= UART_CTL_ENA_Msk;
}

/**
  * @brief  Disable the g_at_lpuart module.
  * @param  ulBase: the base address of g_at_lpuart module
  * @retval None
  */
void UARTDisable(unsigned long ulBase)
{
	HWREGB(ulBase + UART_CTL) &= ~UART_CTL_ENA_Msk;
}


unsigned char UART_Enable_Get(unsigned long ulBase)
{
    return ((HWREGB(ulBase + UART_CTL) & UART_CTL_ENA_Msk) ? 1 : 0);
}


/**
  * @brief Enables the FIFO of g_at_lpuart.
  * @param ulBase: the base address of g_at_lpuart module
  *   This parameter can be one of the following values:
  *     @arg UART_FIFO_RX: Receiver FIFO
  *     @arg UART_FIFO_TX: Transmitter FIFO
  *     @arg UART_FIFO_ALL: Receiver and Transmitter FIFO
  * @retval None
  */
void UARTFIFOEnable(unsigned long ulBase, unsigned char ucFIFOFlags)
{
	/* Enable the RX or TX FIFO */
	if(ucFIFOFlags == UART_FIFO_RX)
	{
	    HWREGB(ulBase + UART_CTL) |= UART_CTL_RX_ENA_Msk;
	}
	else if(ucFIFOFlags == UART_FIFO_TX)
	{
		HWREGB(ulBase + UART_CTL) |= UART_CTL_TX_ENA_Msk;
	}
    /* Enable the ALL FIFO */
	else
	{
	    HWREGB(ulBase + UART_CTL) |= (UART_CTL_RX_ENA_Msk | UART_CTL_TX_ENA_Msk);
	}
}

/**
  * @brief Disable the FIFO of g_at_lpuart.
  * @param ulBase: the base address of g_at_lpuart module
  *   This parameter can be one of the following values:
  *     @arg UART_FIFO_RX: Receiver FIFO
  *     @arg UART_FIFO_TX: Transmitter FIFO
  *     @arg UART_FIFO_ALL: Receiver and Transmitter FIFO
  * @retval None
  */
void UARTFIFODisable(unsigned long ulBase, unsigned char ucFIFOFlags)
{

	/* Disable the RX or TX FIFO */
	if(ucFIFOFlags == UART_FIFO_RX)
	{
	    HWREGB(ulBase + UART_CTL) &= ~UART_CTL_RX_ENA_Msk;
	}
	else if(ucFIFOFlags == UART_FIFO_TX)
	{
		HWREGB(ulBase + UART_CTL) &= ~UART_CTL_TX_ENA_Msk;
	}
    /* Disable the ALL FIFO */
	else
	{
	    HWREGB(ulBase + UART_CTL) &= ~(UART_CTL_RX_ENA_Msk | UART_CTL_TX_ENA_Msk);
	}
}



unsigned char UART_TxEnable_Get(unsigned long ulBase)
{
    return ((HWREGB(ulBase + UART_CTL) & UART_CTL_TX_ENA_Msk) ? 1 : 0);
}


/**
  * @brief Enable DMA transfer function.
  * @param ulBase: the base address of g_at_lpuart module
  * @retval None
  */
void UARTDmaTransferEnable(unsigned long ulBase)
{
	HWREGB(ulBase + UART_FIFO_TRIGGER) |= UART_FIFO_DMA_EN_MSK;
}

/**
  * @brief Disable DMA transfer function.
  * @param ulBase: the base address of g_at_lpuart module
  * @retval None
  */
void UARTDmaTransferDisable(unsigned long ulBase)
{
	HWREGB(ulBase + UART_FIFO_TRIGGER) &= ~UART_FIFO_DMA_EN_MSK;
}

/**
  * @brief Sets the RXFIFO level.
  * @param ulBase: the base address of g_at_lpuart module.
  * @param ulRxLevel: the receive FIFO interrupt level.
  *   This parameter can be one of the following values:
  *     @arg UART_FIFO_LEVEL_RX1_4: 0~25% of full
  *     @arg UART_FIFO_LEVEL_RX2_4: 25~50% of full
  *     @arg UART_FIFO_LEVEL_RX3_4: 50~75% of full
  *     @arg UART_FIFO_LEVEL_RX4_4: 75~100% of full
  * @retval None
  */
void UART_RXFIFO_LevelSet(unsigned long ulBase, unsigned long ulRxLevel)
{
	/* Set the FIFO trigger levels. */
	HWREGB(ulBase + UART_FIFO_TRIGGER) = (HWREGB(ulBase + UART_FIFO_TRIGGER) & ~UART_FIFO_LEVEL_RX_Msk) | ulRxLevel;
}

/**
  * @brief Sets the FIFO level.
  * @param ulBase: the base address of g_at_lpuart module.
  * @param  ulTxLevel: the transmit FIFO interrupt level.
  *   This parameter can be one of the following values:
  *     @arg UART_FIFO_LEVEL_TX1_4: 0~25% of full
  *     @arg UART_FIFO_LEVEL_TX2_4: 25~50% of full
  *     @arg UART_FIFO_LEVEL_TX3_4: 50~75% of full
  *     @arg UART_FIFO_LEVEL_TX4_4: 75~100% of full
  * @retval None
  */
void UART_TXFIFO_LevelSet(unsigned long ulBase, unsigned long ulTxLevel)
{
	/* Set the FIFO trigger levels. */
	HWREGB(ulBase + UART_FIFO_TRIGGER) = (HWREGB(ulBase + UART_FIFO_TRIGGER) & ~UART_FIFO_LEVEL_TX_Msk) | ulTxLevel;
}

/**
  * @brief Confiugres the tx wait time.
  * @param ulBase: the base address of g_at_lpuart module.
  * @param ucValue: the wait time between each frame trasmission.
  *   The actual time = ucValue * (1 / baudrate).
  * @retval None
  */
void UARTTxWaitSet(unsigned long ulBase, unsigned char ucValue)
{
	HWREGB(ulBase + UART_TX_WAIT) = ucValue;
}

/**
  * @brief Waits for trasmission to complete.
  * @param ulBase: the base address of g_at_lpuart module.
  * @retval None
  */
void UARTWaitTxDone(unsigned long ulBase)
{
	while(!(HWREGB(ulBase + UART_STAT_TXFIFO) & UART_STAT_TXFIFO_EMPTY_Msk));
	while((HWREGB(ulBase + UART_STAT_TXFIFO) & UART_STAT_TXFSM_FLAG_Msk) == UART_STAT_TXFSM_FLAG_Msk);    //TX shift fifo not empty

}

/**
  * @brief Sets the configuration of g_at_lpuart.
  * @param ulBase: the base address of g_at_lpuart module.
  * @param ulPclk: the rate of the clock supplied to the g_at_lpuart module.
  * @param ulBaud：the g_at_lpuart communication baud rate.
  * @param ulConfig：the data format for g_at_lpuart (number of data bits, endian,
  *	       and parity).Default is 2 stop bits.
  *   This parameter can be any combination of the following values:
  *     @arg UART_CTL_CHAR_LEN_6
  *     @arg UART_CTL_CHAR_LEN_7
  *     @arg UART_CTL_CHAR_LEN_8
  *     @arg UART_CTL_PARITY_NONE
  *     @arg UART_CTL_PARITY_EVEN
  *     @arg UART_CTL_PARITY_ODD
  * @retval None
  */
void UARTConfigSetExpClk(unsigned long ulBase, unsigned long ulPclk, unsigned long ulBaudrate, unsigned long ulConfig)
{
	unsigned long ulBDIV, ulBQOT, ulBREM;

	/* Calculate the best BDIV */
	for(ulBDIV = 1; ulBDIV < 255; ulBDIV++)
	{
        if(ulPclk / ulBaudrate < 7282 * ulBDIV)
            break;
    }

    /* Calculate the quotient & remiander */
    ulBQOT = ulPclk /(ulBaudrate * ulBDIV);
    ulBREM = ((9 * ulPclk + ulBaudrate * ulBDIV / 2) /(ulBaudrate * ulBDIV)) - 9 * ulBQOT;

    // 数字仿真发现余数为9有问题，暂时通过把9改为8进行规避
    if (ulBREM == 9) ulBREM -= 1;

    UARTDisable(ulBase);
	UARTFIFODisable(ulBase, UART_FIFO_ALL);

    /* Disable ABD function */
    HWREGB(ulBase + UART_ABD_ENA) &= ~UART_ABD_ENA_Msk;

    /* Set the baud rate */
    HWREG(ulBase + UART_BAUD_DIV) = (ulBDIV-1)<<24 | ulBREM<<16 | ulBQOT;

    /* Set data length(8bits), parity check. Default 2 stop bits, little endian.*/
    HWREGB(ulBase + UART_CTL) = ( HWREGB(ulBase + UART_CTL) & ~(UART_CTL_CHAR_LEN_Msk | UART_CTL_ENDIAN_Msk | UART_CTL_PARITY_Msk)) | ulConfig;

    /* Set TX wait time =  1 baudrate cycle */
	HWREGB(ulBase + UART_TX_WAIT) = 0x01;

	/* Enable RX, TX, UART */
	HWREGB(ulBase + UART_CTL) |= (UART_CTL_ENA_Msk | UART_CTL_RX_ENA_Msk | UART_CTL_TX_ENA_Msk);
}


void UART_ConfigRegister_Set(unsigned long ulBase, unsigned long ulConfig)
{
    HWREGB(ulBase + UART_CTL) = (HWREGB(ulBase + UART_CTL) & ~(UART_CTL_CHAR_LEN_Msk | UART_CTL_ENDIAN_Msk | UART_CTL_PARITY_Msk)) | ulConfig;
}


/**
  * @brief Gets the current configuration of g_at_lpuart.
  * @param ulBase: a pointer to storage the base address of g_at_lpuart module.
  * @param ulPclk: a pointer to storage the rate of the clock supplied.
  * @param ulBaud：a pointer to storage for the data format.
  * @param ulConfig：store the data format for g_at_lpuart (number of data bits, endian,
  *	       and parity).Default is 2 stop bits.
  * @retval None
  */
void UARTConfigGetExpClk(unsigned long ulBase, unsigned long ulPclk, unsigned long *pulBaud, unsigned long *pulConfig)
{
	unsigned long ulBDIV, ulBQOT, ulBREM, ulTemp;


    /* Compute the baud rate */
	ulTemp = HWREG(ulBase + UART_BAUD_DIV);
    ulBQOT = ulTemp & 0x1FFF;
    ulBREM = (ulTemp >> 16) & 0x0F;
	ulBDIV = (ulTemp >> 24) & 0xFF  ;

    *pulBaud = (ulPclk * 9) / ((ulBDIV + 1) * (ulBQOT * 9 + ulBREM));

    /* Get the data length, endian, and parity */
	*pulConfig = (HWREGB(ulBase + UART_CTL) & (UART_CTL_CHAR_LEN_Msk | UART_CTL_ENDIAN_Msk | UART_CTL_PARITY_Msk));
}

/**
  * @brief LPUART Baudrate Auto Detection.
  * @param ulBase: the base address of g_at_lpuart module.
  * @param ulConfig：the data format for g_at_lpuart (number of data bits, endian,
  *	       and parity).Default is 2 stop bits.
  *   This parameter can be any combination of the following values:
  *     @arg UART_CTL_CHAR_LEN_6
  *     @arg UART_CTL_CHAR_LEN_7
  *     @arg UART_CTL_CHAR_LEN_8
  *     @arg UART_CTL_PARITY_NONE
  *     @arg UART_CTL_PARITY_EVEN
  *     @arg UART_CTL_PARITY_ODD
  * @retval None
  */
void UARTAutoBaudrate(unsigned long ulBase, unsigned long ulConfig)
{
	/* Disable UART */
	UARTDisable(ulBase);

	/* Set endian(little), data length(8bits), and parity check. Default 2 stop bits */
	HWREGB(ulBase + UART_CTL) = (HWREGB(ulBase + UART_CTL) & ~(UART_CTL_CHAR_LEN_Msk | UART_CTL_ENDIAN_Msk | UART_CTL_PARITY_Msk)) | ulConfig;

    /* Set TX wait time =  1 baudrate cycle */
	HWREGB(ulBase + UART_TX_WAIT) = 0x01;

	/* Enable ABD function */
	HWREGB(ulBase + UART_ABD_ENA) |= UART_ABD_ENA_Msk;

	/* Enable RX, TX, UART */
    HWREGB(ulBase + UART_CTL) |= (UART_CTL_ENA_Msk | UART_CTL_RX_ENA_Msk | UART_CTL_TX_ENA_Msk);
}

/**
  * @brief Get the result of Baudrate Auto Detection
  * @param ulBase: the base address of g_at_lpuart module.
  * @retval The ABD END status.
  */
unsigned char UARTABDEndStatus(unsigned long ulBase)
{
	return ((HWREGB(ulBase + UART_FLAG_STATUS) & UART_FLAG_STATUS_ABD_END_Msk)? 1 : 0);
}

/**
  * @brief Enable individual UART interrupt sources.
  * @param ulBase: the base address of g_at_lpuart module.
  * @param ulIntFlags: the bit mask of the interrupt sources to be enable
  *   This parameter can be any combination of the following values:
  *     @arg UART_INT_RX_FRAME_ERR: Frame Error interrupt
  *     @arg UART_INT_RX_PAR_ERR: Parity Error interrupt
  *     @arg UART_INT_RXFIFO_EMPTY: Receiver FIFO Empty interrupt
  *     @arg UART_INT_RXFIFO_OVFLW: Receiver FIFO Overflow interrupt
  *     @arg UART_INT_TXFIFO_EMPTY: Transmitter FIFO Empty interrupt
  *     @arg UART_INT_TXFIFO_FULL: Transmitter FIFO Full interrupt
  *     @arg UART_INT_TXFIFO_OVFLW: Transmitter FIFO Overflow interrupt
  *     @arg UART_INT_WAKEUP: Wakeup interrupt
  *     @arg UART_INT_RXFIFO_TRIGGER: The Receiver FIFO Trigger interrupt
  *     @arg UART_INT_TXFIFO_TRIGGER: The Transmitter FIFO Trigger interrupt
  *     @arg UART_INT_FLOW_CTL: The flow control interrupt
  *     @arg UART_INT_RXFIFO_ALMOST_FULL: Transmitter FIFO almost Full interrupt
  *     @arg UART_INT_TIMEOUT: Timeout interrupt
  *     @arg UART_INT_ALL
  * @retval None
  */
void UARTIntEnable(unsigned long ulBase, unsigned long ulIntFlags)
{
    /* Enable the specified interrupts */
    HWREGH(ulBase + UART_INT_MASK) |= ulIntFlags;
}

/**
  * @brief Disable individual UART interrupt sources.
  * @param ulBase: the base address of g_at_lpuart module.
  * @param ulIntFlags: the bit mask of the interrupt sources to be disabled.
  *   This parameter has the same definition as ulIntFlags of UARTIntEnable().
  * @retval None
  */
void UARTIntDisable(unsigned long ulBase, unsigned long ulIntFlags)
{
    HWREGH(ulBase + UART_INT_MASK) &= ~ulIntFlags;
}

/**
  * @brief Gets the current interrupt status and clear them.
  * @note The specified UART interrupt sources are cleared, so that they
  *   no longer assert and returns the interrupt status for the specified
  *   UART. This function must be called in the interrupt handler to keep
  *   the interrupt from being recognized again immediately upon exit.
  * @param ulBase: the base address of g_at_lpuart module.
  * @retval The current interrupt status. Enumerated as a bit field of values
  *   described in UARTIntEnable().
  */
unsigned short UARTIntRead(unsigned long ulBase)
{
	volatile unsigned long ulReg;

	/* Return either the interrupt status or the raw interrupt status as requested */
	ulReg =  HWREGH(ulBase + UART_INT_STAT);

	return ulReg;
}

/**
  * @brief Clear the current interrupt status .
  * @param UARTx: Select the UART peripheral..
  * @retval None.
  */
void UART_IntClear(unsigned long ulBase,uint32_t RegValue)
{
	/* Return either the interrupt status or the raw interrupt status as requested */
	HWREGH(ulBase + UART_INT_STAT) = RegValue;

}
/**
  * @brief  Gets the interrupt that is enabled currently.
  * @param  ulBase: the base address of g_at_lpuart module
  * @retval The current enable interrupt status. Enumerated as a bit field of values
  *   described in UARTIntEnable()
  */
unsigned short UARTIntMasked(unsigned long ulBase)
{
	/* Return either the interrupt status or the raw interrupt status as requested */
	return HWREGH(ulBase + UART_INT_MASK);
}

/**
  * @brief  Determines whether the g_at_lpuart receiver is idle or not.
  * @param  ulBase: the base address of g_at_lpuart module
  * @retval the g_at_lpuart receiver status.
  */
unsigned char UARTRxIdle(unsigned long ulBase)
{
	return (HWREGB(ulBase + UART_FLAG_STATUS)& UART_FLAG_STATUS_RX_IDLE_Msk ? true : false );
}

/**
  * @brief Flushes the g_at_lpuart FIFO.
  * @note This function flushes the Receiver or Transmitter FIFO, the data in FIFO
  *   will be discarded and the FIFO become empty .
  * @param ulBase: the base address of g_at_lpuart module.
  * @param ucFIFOFlags: Receiver or Transmitter FIFO flag.
  *   This parameter can be one of the following values:
  *     @arg UART_FIFO_RX: Receiver FIFO
  *     @arg UART_FIFO_TX: Transmitter FIFO
  *     @arg UART_FIFO_ALL: Receiver and Transmitter FIFO
  * @retval None
  */
void UARTFIFOFlush(unsigned long ulBase, unsigned char ucFIFOFlags)
{
    /* Flush the RX or TX FIFO */
	if(ucFIFOFlags == UART_FIFO_RX)
	{
	    HWREGB(ulBase + UART_FIFO_FLUSH) |= UART_FIFO_FLUSH_RX_Msk;
	}
	else if(ucFIFOFlags == UART_FIFO_TX)
	{
		HWREGB(ulBase + UART_FIFO_FLUSH) |= UART_FIFO_FLUSH_TX_Msk;
	}
    /* Flush the ALL FIFO */
	else
	{
	    HWREGB(ulBase + UART_FIFO_FLUSH) |= (UART_FIFO_FLUSH_RX_Msk | UART_FIFO_FLUSH_TX_Msk);
	}
}

/**
  * @brief Gets the RX FIFO status of g_at_lpuart.
  * @note This function is getting the current Receiver FIFO status.
  * @param ulBase: the base address of g_at_lpuart module.
  * @param  ucFlagType specifies the flag to check.
  *   This parameter can be one of the following values:
  *   @arg UART_FIFO_EMPTY: UART Fifo empty flag
  *   @arg UART_FIFO_FULL: UART Fifo full flag
  *   @arg UART_FIFO_LEVEL: UART Fifo level flag
  *   @arg UART_FIFO_DATA_LEN: The Fifo bytes data number left;when fifo is full, this is 0
  * @retval The current fifo state
  */
unsigned char UARTRxFifoStatusGet(unsigned long ulBase, unsigned char ucFlagType)
{
	if(ucFlagType == UART_FIFO_EMPTY)
	{
		return ((HWREGB(ulBase + UART_STAT_RXFIFO) & UART_STAT_RXFIFO_EMPTY_Msk) ? 1 : 0);
	}
	else if(ucFlagType == UART_FIFO_FULL)
	{
		return ((HWREGB(ulBase + UART_STAT_RXFIFO) & UART_STAT_RXFIFO_FULL_Msk) ? 1 : 0);
	}
	else if(ucFlagType == UART_FIFO_LEVEL)
	{
		return ((HWREGB(ulBase + UART_STAT_RXFIFO) & UART_STAT_RXFIFO_LEVEL_Msk) >> UART_STAT_RXFIFO_LEVEL_Pos);
	}
	else
	{
		return HWREGB(ulBase + UART_RXFIFO_BYTE_LEVEL);
	}
}

/**
  * @brief Gets the TX FIFO status of g_at_lpuart.
  * @note This function is getting the current Transmitter FIFO status.
  * @param ulBase: the base address of g_at_lpuart module.
  * @param  ucFlagType specifies the flag to check.
  *   This parameter can be one of the following values:
  *   @arg UART_FIFO_EMPTY: UART Fifo empty flag
  *   @arg UART_FIFO_FULL: UART Fifo full flag
  *   @arg UART_FIFO_LEVEL: UART Fifo level flag
  *   @arg UART_FIFO_DATA_LEN: The Fifo bytes data number left;when fifo is full, this is 0
  * @retval The current fifo state
  */
unsigned char UARTTxFifoStatusGet(unsigned long ulBase, unsigned char ucFlagType)
{
	if(ucFlagType == UART_FIFO_EMPTY)
	{
		return ((HWREGB(ulBase + UART_STAT_TXFIFO) & UART_STAT_TXFIFO_EMPTY_Msk) ? 1 : 0);
	}
	else if(ucFlagType == UART_FIFO_FULL)
	{
		return ((HWREGB(ulBase + UART_STAT_TXFIFO) & UART_STAT_TXFIFO_FULL_Msk) ? 1 : 0);
	}
	else if(ucFlagType == UART_FIFO_LEVEL)
	{
		return ((HWREGB(ulBase + UART_STAT_TXFIFO) & UART_STAT_TXFIFO_LEVEL_Msk) >> UART_STAT_TXFIFO_LEVEL_Pos);
	}
	else
	{
		return HWREGB(ulBase + UART_TXFIFO_BYTE_LEVEL);
	}
}


/**
 * @brief 返回UART是否发送完成
 * @param ulBase 基地址
 * @note  0：未完成，1：已完成
 */
unsigned char UART_TxDoneStatus_Get(unsigned long ulBase)
{
    unsigned char txfifo_empty_flag = (HWREGB(ulBase + UART_STAT_TXFIFO) & UART_STAT_TXFIFO_EMPTY_Msk) ? 1 : 0;
    unsigned char txfsm_idle_flag   = (HWREGB(ulBase + UART_STAT_TXFIFO) & UART_STAT_TXFSM_FLAG_Msk)   ? 0 : 1;
    if( txfifo_empty_flag && txfsm_idle_flag )
    {
        return 1;
    }

    return 0;
}



/**
  * @brief Gets the status of g_at_lpuart FIFO.
  * @note This function is getting the current Receiver or Transmitter
  *   FIFO status.
  * @param ulBase: the base address of g_at_lpuart module.
  * @param ucFIFOFlags: Receiver or Transmitter FIFO flag.
  *   This parameter can be one of the following values:
  *     @arg UART_FIFO_RX: Receiver FIFO
  *     @arg UART_FIFO_TX: Transmitter FIFO
  * @retval  The current bytes in Receiver or Transmitter.
  */
unsigned char UARTFIFOByteLevel(unsigned long ulBase, unsigned char ucFIFOFlags)
{
	if(ucFIFOFlags == UART_FIFO_RX)
	{
		return HWREGB(ulBase + UART_RXFIFO_BYTE_LEVEL);
	}
	else
	{
		return HWREGB(ulBase + UART_TXFIFO_BYTE_LEVEL);
	}

}

/* Receive error data when the data has frame or parity err.*/
void  UART_Enable_FrameParityErrToWFIFO(unsigned long ulBase)
{
	HWREG(ulBase + UART_TRANS_CFG) |= FRAME_PARITY_ERR_TO_WFIFO_EN_Msk;
}

/* Don't receive error data when the data has frame or parity err.*/
void  UART_Disable_FrameParityErrToWFIFO(unsigned long ulBase)
{
	HWREG(ulBase + UART_TRANS_CFG) &= ~FRAME_PARITY_ERR_TO_WFIFO_EN_Msk;
}


/**
 * @brief 返回LPUART的TXFIO是否满。
 * @note  当LPUART的参考时钟为32K时必须使用该接口，此时TX大部分状态更新慢，需要等3个32k周期。
 *        只有txfifo_level寄存器和txfifo_byte_level寄存器的更新是快的。
 *        因此LPUART的发送状态只能使用这两个寄存器做判断。
 * @return 1:txfifo满了，0:txfifo没满
 */
unsigned char LPUART_IS_TXFIFO_FULL(void)
{
    if((UARTTxFifoStatusGet(UART1_BASE, UART_FIFO_LEVEL) == 0x03) && (UARTTxFifoStatusGet(UART1_BASE, UART_FIFO_DATA_LEN) == 0x00))
    {
        return 1;
    }
    return 0;
}

/**
 * @brief 返回LPUART的TXFIO是否空。
 * @note  当LPUART的参考时钟为32K时必须使用该接口，此时TX大部分状态更新慢，需要等3个32k周期。
 *        只有txfifo_level寄存器和txfifo_byte_level寄存器的更新是快的。
 *        因此LPUART的发送状态只能使用这两个寄存器做判断。
 * @return 1:txfifo空了，0:txfifo没空
 */
unsigned char LPUART_IS_TXFIFO_EMPTY(void)
{
    if((UARTTxFifoStatusGet(UART1_BASE, UART_FIFO_LEVEL) == 0x00) && (UARTTxFifoStatusGet(UART1_BASE, UART_FIFO_DATA_LEN) == 0x00))
    {
        return 1;
    }
    return 0;
}


/**
  * @brief Sets the configuration of wakeup mode.
  * @param ulBase: the base address of g_at_lpuart module.
  * @param ucPattern1: the matching pattern1.
  * @param ucPattern2: the matching pattern2.
  * @param ulConfig: the configuration of wakeup mode.
  *   This parameter can be any combination of the following values:
  *     @arg UART_WAKEUP_PATTERN_ANY_BYTE: matching any byte of data
  *     @arg UART_WAKEUP_PATTERN_ONE_BYTE: matching one-byte pattern
  *     @arg UART_WAKEUP_PATTERN_TWO_BYTE: matching continuous two byte of  pattern
  *     @arg UART_WAKEUP_DATA_CLEAR_DIS: data is saved in FIFO
  *     @arg UART_WAKEUP_DATA_CLEAR_ENA: data is not saved in FIFO
  *     @arg UART_WAKEUP_CHECK_LEVEL_1_4:
  *     @arg UART_WAKEUP_CHECK_LEVEL_2_4:
  *     @arg UART_WAKEUP_CHECK_LEVEL_3_4:
  *     @arg UART_WAKEUP_CHECK_DIS:
  * @retval None
  */
void UARTWakeUpModeConfig(unsigned long ulBase, unsigned char ucConfig, unsigned char ucPattern1, unsigned char ucPattern2)
{
	unsigned short usPattern = 0;

	usPattern = (ucPattern2 << 8) | ucPattern1;
    /* Set the wakeup Mode,wakeup pattern and enable wakeup */
	HWREG(ulBase + UART_WAKEUP_MODE) = ((usPattern << 8 ) & UART_WAKEUP_PATTERN_Msk)| ucConfig;
}

/**
  * @brief Enable wakeup mode.
  * @note This function must be called after UARTWakeUpModeConfig.
  * @param ulBase: the base address of g_at_lpuart module
  * @retval None
  */
void UARTWakeUpModeEnable(unsigned long ulBase)
{
	HWREG(ulBase + UART_WAKEUP_MODE) |= UART_WAKEUP_ENABLE_Msk;
}

/**
  * @brief Disables wakeup mode.
  * @param ulBase: the base address of g_at_lpuart module
  * @retval None
  */
void UARTWakeUpModeDisable(unsigned long ulBase)
{
	HWREG(ulBase + UART_WAKEUP_MODE) &= ~UART_WAKEUP_ENABLE_Msk;
}

/**
  * @brief Sets the configuration of sequence detection.
  * @param ulBase: the base address of g_at_lpuart module
  * @param ucValidBits: the valid bits of ucPatternfor sequence detection.
  * @param ucPattern: a given pattern for UART sequence detection.
  *   This parameter can be one of the following values:
  *     @arg UART_SQUENCE_DETECT_VALID_5BITS
  *     @arg UART_SQUENCE_DETECT_VALID_6BITS
  *     @arg UART_SQUENCE_DETECT_VALID_7BITS
  *     @arg UART_SQUENCE_DETECT_VALID_8BITS
  * @retval None
  */
void UARTSequenceDetectModeSet(unsigned long ulBase, unsigned char ucValidBits, unsigned char ucPattern)
{
	HWREGB(ulBase + UART_SEQ_DETECT_CFG1) = ucPattern;
    HWREGB(ulBase + UART_SEQ_DETECT_CFG2) = (HWREGB(UART_SEQ_DETECT_CFG2) & ~(UART_SQUENCE_DETECT_VALID_Msk)) | ucValidBits;
}

/**
  * @brief Enables Sequence Detect.
  * @param ulBase: the base address of g_at_lpuart module
  * @retval None
  */
void UARTSequenceDetectEnable(unsigned long ulBase)
{
    HWREGB(ulBase + UART_SEQ_DETECT_CFG0) |= UART_SQUENCE_DETECT_Msk ;
}

/**
  * @brief Disables Sequence Detect.
  * @param ulBase: the base address of g_at_lpuart module
  * @retval None
  */
void UARTSequenceDetectDisable(unsigned long ulBase)
{
    HWREGB(ulBase + UART_SEQ_DETECT_CFG0) &= ~UART_SQUENCE_DETECT_Msk ;
}

/**
  * @brief Sets the configuration of time out.
  * @param ulBase: the base address of g_at_lpuart module
  * @param ucValue: the timeout value.
      This parameter is the time = ucValue * (1 / baudrate).
  * @param ucStartCondition: is the time start condition.
  *   This parameter can be one of the following values:
  *     @arg UART_RX_TIMEOUT_START_NO_MATTER
  *     @arg UART_RX_TIMEOUT_START_FIFO_NOT_EMPTY
  * @retval None
  */
void UARTTimeOutConfig(unsigned long ulBase, unsigned char ucStartCondition, unsigned char ucValue)
{
	HWREGH(ulBase + UART_RX_TIMEOUT_CONFIG) = (ucValue << 8 )| ucStartCondition;
}


void UARTTimeOutCondition_Set(unsigned long ulBase, unsigned char ucStartCondition)
{
	HWREGH(ulBase + UART_RX_TIMEOUT_CONFIG) = (HWREGH(ulBase + UART_RX_TIMEOUT_CONFIG) & ~UART_RX_TIMEOUT_START_Msk) | ucStartCondition;
}

/**
 * @brief 获取UART超时类型
 * @param ulBase 基地址
 * @return 0：超时倒计时触发条件为RXD处于IDLE
 *         1：超时倒计时触发条件为RXD处于IDLE且RXFIFO非空
 */
unsigned char UART_TimeoutCondition_Get(unsigned long ulBase)
{
    return ((HWREGB(ulBase + UART_RX_TIMEOUT_CONFIG) & UART_RX_TIMEOUT_START_Msk) ? 1 : 0);
}


/**
  * @brief Enables the time.
  * @param ulBase: the base address of g_at_lpuart module
  * @retval None
  */
void UARTTimeOutEnable(unsigned long ulBase)
{
	HWREGB(ulBase + UART_RX_TIMEOUT_CONFIG) |= UART_RX_TIMEOUT_EN_Msk;
}

/**
  * @brief Disables the time.
  * @param ulBase: the base address of g_at_lpuart module
  * @retval None
  */
void UARTTimeOutDisable(unsigned long ulBase)
{
	HWREGB(ulBase + UART_RX_TIMEOUT_CONFIG) &= ~UART_RX_TIMEOUT_EN_Msk;
}

/**
  * @brief  Determines whether the UART receiver is enable by hardware.
  * @param  ulBase: the base address of g_at_lpuart module
  * @retval The receiver status.
  */
unsigned char UARTRXEnaStatus(unsigned long ulBase)
{
    return ((HWREGB(ulBase + UART_CTL) & UART_CTL_RX_ENA_Msk) ? 1: 0);
}

/**
  * @brief Enables flow control.
  * @param ulBase: the base address of g_at_lpuart module
  * @retval None
  */
void UARTFlowCtrlEnable(unsigned long ulBase)
{

	HWREGB(ulBase + UART_FLOW_CTL0) |= UART_FLOW_CTL_EN_Msk ;
}

/**
  * @brief Disbale flow control.
  * @param ulBase: the base address of g_at_lpuart module
  * @retval None
  */
void UARTFlowCtrlDisable(unsigned long ulBase)
{

	HWREGB(ulBase + UART_FLOW_CTL0) &= ~UART_FLOW_CTL_EN_Msk ;
}

/**
  * @brief Configure flow control.
  * @param ulBase: the base address of g_at_lpuart module.
  * @param ucConfig: the configuration for flow control.
  *   This parameter can be any combination of the following values:
  *     @arg UART_FLOW_CTL_AUTO
  *     @arg UART_FLOW_CTL_SW_EN
  *     @arg UART_FLOW_CTL_VALID_LEVEL_LOW
  *     @arg UART_FLOW_CTL_VALID_LEVEL_HIGH
  *     @arg UART_FLOW_CTL_RTS_TRIGGER_LEVEL_1_4
  *     @arg UART_FLOW_CTL_RTS_TRIGGER_LEVEL_2_4
  *     @arg UART_FLOW_CTL_RTS_TRIGGER_LEVEL_3_4
  *     @arg UART_FLOW_CTL_RTS_TRIGGER_DIS
  * @retval None
  */
void UARTFlowCtrlConfig(unsigned long ulBase, unsigned char ucConfig)
{

	HWREGB(ulBase + UART_FLOW_CTL0) = (HWREGB(ulBase + UART_FLOW_CTL0) & ~(UART_FLOW_CTL_SW_EN_Msk | UART_FLOW_CTL_VALID_LEVEL_Msk | UART_FLOW_CTL_RTS_TRIGGER_Msk))| ucConfig ;
}

/**
  * @brief Get the RTS pin value.
  * @param ulBase: the base address of g_at_lpuart module
  * @retval the RTS pin value
  */
unsigned char UARTFlowCtrlRtsGet(unsigned long ulBase)
{
    return ((HWREGB(ulBase + UART_FLOW_CTL1) & UART_FLOW_CTL_RTS_VALUE_Msk) ? 1 : 0);
}

/**
  * @brief Set the RTS pin.
  * @param ulBase: the base address of g_at_lpuart module
  * @retval None
  */
void UARTFlowCtrlRtsSet(unsigned long ulBase)
{
	HWREGB(ulBase + UART_FLOW_CTL1) |= UART_FLOW_CTL_RTS_VALUE_Msk;
}

/**
  * @brief Clear the RTS pin.
  * @param ulBase: the base address of g_at_lpuart module
  * @retval None
  */
void UARTFlowCtrlRtsClear(unsigned long ulBase)
{
	HWREGB(ulBase + UART_FLOW_CTL1) &= ~UART_FLOW_CTL_RTS_VALUE_Msk;
}

/**
  * @brief Get the CTS pin value.
  * @param ulBase: the base address of g_at_lpuart module
  * @retval the CTS pin value
  */
unsigned char UARTFlowCtrlCtsGet(unsigned long ulBase)
{
	return ((HWREGB(ulBase + UART_FLOW_CTL1) & UART_FLOW_CTL_CTS_VALUE_Msk) ? 1 : 0);
}

/**
  * @brief Set the CTS pin.
  * @param ulBase: the base address of g_at_lpuart module
  * @retval None
  */
void UARTFlowCtrlCtsSet(unsigned long ulBase)
{
	HWREGB(ulBase + UART_FLOW_CTL1) |= UART_FLOW_CTL_CTS_VALUE_Msk;
}

/**
  * @brief Clear the CTS pin.
  * @param ulBase: the base address of g_at_lpuart module
  * @retval None
  */
void UARTFlowCtrlCtsClear(unsigned long ulBase)
{
	HWREGB(ulBase + UART_FLOW_CTL1) &= ~UART_FLOW_CTL_CTS_VALUE_Msk;
}

/**
  * @brief Configure RX start bit offset
  * @param ulBase: the base address of g_at_lpuart module.
  * @param ucValue: the start bit offset value.
  * @retval None.
  */
void UARTStartOffsetConfig(unsigned long ulBase, unsigned char ucValue)
{
	HWREGH(ulBase + UART_START_OFFSET_CONFIG) = (HWREGH(ulBase + UART_START_OFFSET_CONFIG)& ~(UART_START_OFFSET_VALUE_Msk)) | (ucValue << UART_START_OFFSET_VALUE_Pos);
}

/**
  * @brief Get the RX start bit offset flag
  * @param ulBase: the base address of g_at_lpuart module.
  * @note After enable StartOffset, do not change the value
  *       when StartOffsetFlag is 1.
  * @retval None.
  */
void UARTStartOffsetEnable(unsigned long ulBase)
{
	HWREGB(ulBase + UART_START_OFFSET_CONFIG) |= UART_START_OFFSET_EN_Msk;
}

/**
  * @brief Get the RX start bit offset flag
  * @param ulBase: the base address of g_at_lpuart module.
  * @retval The flag.
  */
unsigned char UARTStartOffsetFlag(unsigned long ulBase)
{
	return ((HWREGB(ulBase + UART_FLAG_STATUS) & UART_FLAG_START_OFFSET_VALID_Msk)? 1 : 0);
}

/**
  * @brief Get a character.
  * @note This function gets a character from the receive FIFO of UART.
          If there no char available, this function will be waitting.
  * @param ulBase: the base address of g_at_lpuart module.
  * @retval the character read from g_at_lpuart.
  */
unsigned char UARTCharGet(unsigned long ulBase)
{
    /* Wait until a char is available.*/
    while((HWREGB(ulBase + UART_STAT_RXFIFO) & UART_STAT_RXFIFO_EMPTY_Msk));

    return HWREGB(ulBase + UART_FIFO_READ);
}


unsigned char UARTReadData(unsigned long ulBase)
{
    return HWREGB(ulBase + UART_FIFO_READ);
}


/**
  * @brief Get a character with non-blocking.
  * @param ulBase: the base address of g_at_lpuart module.
  * @retval Returns the character read from the specified port, cast as a
  *         unsigned char. -1 is returned if there are no characters present
  *         in the receive FIFO.
  */
unsigned long UARTCharGetNonBlocking(unsigned long ulBase)
{
    /* Check if the fifo is not empty */
    if(!(HWREGB(ulBase + UART_STAT_RXFIFO) & UART_STAT_RXFIFO_EMPTY_Msk))
    {
        return(HWREGB(ulBase + UART_FIFO_READ));
    }
    else
    {
        /* There are no characters, so return a failure */
        return(-1);
    }
}

/**
  * @brief Send a character.
  * @note This function sends a character to the transmit FIFO for
  *       g_at_lpuart. If there is no space available in the transmit FIFO,
  *       this function waits until there is space available.
  * @param ulBase: the base address of g_at_lpuart module.
  * @param ucData: the character to be transmitted.
  * @retval None
  */
void UARTCharPut(unsigned long ulBase, unsigned char ucData)
{
    /* Wait until space is available */
    while(HWREGB(ulBase + UART_STAT_TXFIFO) & UART_STAT_TXFIFO_FULL_Msk);

    HWREGB(ulBase + UART_FIFO_WRITE) = ucData;
}


void UARTWriteData(unsigned long ulBase, unsigned char ucData)
{
    HWREGB(ulBase + UART_FIFO_WRITE) = ucData;
}


/**
  * @brief Send a character with non-blocking.
  * @note This function sends the a character to the transmit FIFO for
  *       g_at_lpuart. If there is no space available in the transmit FIFO,
  *       this function will return a failure.
  * @param ulBase: the base address of g_at_lpuart module.
  * @param ucData: the character to be transmitted.
  * @retval Returns true if the character was successfully placed in the
  *         transmit FIFO or false if there was no space available in the
  *         transmit FIFO.
  */
unsigned char UARTCharPutNonBlocking(unsigned long ulBase, unsigned char ucData)
{
    /* Check if there is space in the transmit FIFO */
    if(!(HWREGB(ulBase + UART_STAT_TXFIFO) & UART_STAT_TXFIFO_FULL_Msk))
    {

        HWREGB(ulBase + UART_FIFO_WRITE) = ucData;

        return(true);
    }
    else
    {
        /* There is no space in the transmit FIFO, so return a failure */
        return(false);
    }
}

// ========================== For printf ===============================
static void printch(unsigned long ulBase, char ch)
{
    if(ulBase == UART1_BASE || ulBase == UART2_BASE)
	{
        UARTCharPut(ulBase, ch);
    }
    else if(ulBase == CSP1_BASE || ulBase == CSP2_BASE || ulBase == CSP3_BASE || ulBase == CSP4_BASE)
    {
        CSP_CharPut((CSP_TypeDef*)ulBase, ch);
    }
}

static void printstr(unsigned long ulBase, char* str)
{
    while(*str)
    {
        printch(ulBase, *str++);
    }
}

void UARTPrintf(unsigned long ulBase, char* fmt, ...)
{
    char s[512]={0};
    va_list ap;

    va_start(ap, fmt);
    vsprintf(s, fmt, ap);
    va_end(ap);

    printstr(ulBase, s);
}


