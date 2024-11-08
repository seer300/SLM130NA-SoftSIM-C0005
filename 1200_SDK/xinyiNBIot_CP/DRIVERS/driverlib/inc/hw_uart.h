#include "hw_memmap.h"

#ifndef __HW_UART_H__
#define __HW_UART_H__

//*****************************************************************************
//
// The following are defines for the UART register offsets.
//
//*****************************************************************************
#define UART_CTL                        0x00000000  //8 bits
#define UART_ABD_ENA                    0x00000001  //8 bits
#define UART_FIFO_TRIGGER               0x00000002  //8 bits
#define UART_TX_WAIT                    0x00000003  //8 bits
#define UART_BAUD_DIV                   0x00000004  //32 bits  //baudrate_quotient
#define UART_INT_MASK                   0x00000008  //16 bits
#define UART_INT_STAT                   0x0000000A  //16 bits
#define UART_FLAG_STATUS                0x0000000C  //8 bits
#define UART_FIFO_FLUSH                 0x0000000D  //8 bits
#define UART_STAT_RXFIFO                0x0000000E  //8 bits
#define UART_STAT_TXFIFO                0x0000000F  //8 bits
#define UART_TRANS_CFG                  0x00000014  //8 bits
#define UART_WAKEUP_MODE                0x00000018  //8 bits; wakeup config 32 bits
#define UART_WAKEUP_PATTERN1            0x00000019  
#define UART_WAKEUP_PATTERN2            0x0000001A  
#define UART_SEQ_DETECT_CFG0            0x0000001C  //8 bits   //sequence_detect_ena
#define UART_SEQ_DETECT_CFG1            0x0000001D  //8 bits   
#define UART_SEQ_DETECT_CFG2            0x0000001E  //8 bits   
#define UART_FLOW_CTL0                  0x00000020  //8 bits  
#define UART_FLOW_CTL1                  0x00000021  //8 bits  
#define UART_RXFIFO_BYTE_LEVEL          0x00000024  //
#define UART_TXFIFO_BYTE_LEVEL          0x00000026  //
#define UART_RX_TIMEOUT_CONFIG          0x00000028
#define UART_START_OFFSET_CONFIG        0x0000002C
#define UART_FIFO_READ                  0x00000040  // 10 , 40
#define UART_FIFO_WRITE                 0x00000080  // 14 , 80

//*****************************************************************************
//
// The following are defines for the bit fields in the UART_CTL register.
//
//*****************************************************************************
#define UART_CTL_ENA_Pos               0
#define UART_CTL_ENA_Msk               (1UL << UART_CTL_ENA_Pos)

#define UART_CTL_RX_ENA_Pos            1
#define UART_CTL_RX_ENA_Msk            (1UL << UART_CTL_RX_ENA_Pos)
#define UART_CTL_RX_ENA                (1UL << UART_CTL_RX_ENA_Pos)

#define UART_CTL_TX_ENA_Pos            2
#define UART_CTL_TX_ENA_Msk            (1UL << UART_CTL_TX_ENA_Pos)
#define UART_CTL_TX_ENA                (1UL << UART_CTL_TX_ENA_Pos)

#define UART_CTL_PARITY_Pos            3
#define UART_CTL_PARITY_Msk            (3UL << UART_CTL_PARITY_Pos)
#define UART_CTL_PARITY_NONE           (0UL << UART_CTL_PARITY_Pos)  // No Parity 
#define UART_CTL_PARITY_EVEN           (1UL << UART_CTL_PARITY_Pos)  // Even Parity 
#define UART_CTL_PARITY_ODD            (2UL << UART_CTL_PARITY_Pos)  // Odd Parity

#define UART_CTL_ENDIAN_Pos            5
#define UART_CTL_ENDIAN_Msk            (1UL << UART_CTL_ENDIAN_Pos)
#define UART_CTL_ENDIAN_LITTLE         (0UL << UART_CTL_ENDIAN_Pos)
#define UART_CTL_ENDIAN_BIG            (1UL << UART_CTL_ENDIAN_Pos)

#define UART_CTL_CHAR_LEN_Pos          6
#define UART_CTL_CHAR_LEN_Msk          (3UL << UART_CTL_CHAR_LEN_Pos)
#define UART_CTL_CHAR_LEN_8            (0UL << UART_CTL_CHAR_LEN_Pos) 
#define UART_CTL_CHAR_LEN_7            (2UL << UART_CTL_CHAR_LEN_Pos)
#define UART_CTL_CHAR_LEN_6            (3UL << UART_CTL_CHAR_LEN_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the ABD_ENA register.
//
//*****************************************************************************
#define UART_ABD_ENA_Pos                0
#define UART_ABD_ENA_Msk                (1UL << UART_ABD_ENA_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the FIFO_TRIGGER register.
//
//*****************************************************************************
#define UART_FIFO_DMA_EN_Pos			0
#define UART_FIFO_DMA_EN_MSK			(1UL << UART_FIFO_DMA_EN_Pos)

#define UART_FIFO_LEVEL_TX_Pos			1
#define UART_FIFO_LEVEL_TX_Msk			(3UL << UART_FIFO_LEVEL_TX_Pos)
#define UART_FIFO_LEVEL_TX1_4			(0UL << UART_FIFO_LEVEL_TX_Pos) 
#define UART_FIFO_LEVEL_TX2_4			(1UL << UART_FIFO_LEVEL_TX_Pos)
#define UART_FIFO_LEVEL_TX3_4			(2UL << UART_FIFO_LEVEL_TX_Pos)
#define UART_FIFO_LEVEL_TX4_4			(3UL << UART_FIFO_LEVEL_TX_Pos)

#define UART_FIFO_LEVEL_RX_Pos			3
#define UART_FIFO_LEVEL_RX_Msk			(3UL << UART_FIFO_LEVEL_RX_Pos)
#define UART_FIFO_LEVEL_RX4_4			(0UL << UART_FIFO_LEVEL_RX_Pos)
#define UART_FIFO_LEVEL_RX3_4			(1UL << UART_FIFO_LEVEL_RX_Pos)
#define UART_FIFO_LEVEL_RX2_4			(2UL << UART_FIFO_LEVEL_RX_Pos)
#define UART_FIFO_LEVEL_RX1_4			(3UL << UART_FIFO_LEVEL_RX_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the INTERRUPT_MASK register.
//
//*****************************************************************************
#define UART_INT_RX_FRAME_ERR				0x00000001  //(1<<0)
#define UART_INT_RX_PAR_ERR	                0x00000002  //(1<<1)
#define UART_INT_RXFIFO_EMPTY				0x00000004  //(1<<2)
#define UART_INT_RXFIFO_FULL				0x00000008  //(1<<3)
#define UART_INT_RXFIFO_OVFLW				0x00000010  //(1<<4)
#define UART_INT_TXFIFO_EMPTY				0x00000020  //(1<<5)
#define UART_INT_TXFIFO_FULL				0x00000040  //(1<<6)
#define UART_INT_TXFIFO_OVFLW				0x00000080  //(1<<7)
#define UART_INT_WAKEUP		                0x00000100  //(1<<8)
#define UART_INT_RXFIFO_TRIGGER	            0x00000200  //(1<<9)
#define UART_INT_TXFIFO_TRIGGER	            0x00000400  //(1<<10)
#define UART_INT_FLOW_CTL	                0x00000800  //(1<<11)
#define UART_INT_RXFIFO_ALMOST_FULL         0x00001000  //(1<<12)
#define UART_INT_TIMEOUT                    0x00002000  //(1<<13)
#define UART_INT_ALL						0x00003FFF 

//*****************************************************************************
//
// The following are defines for the bit fields in the FLAG_STATUS register.
//
//*****************************************************************************
#define UART_FLAG_STATUS_ABD_END_Pos		0
#define UART_FLAG_STATUS_ABD_END_Msk		(1UL << UART_FLAG_STATUS_ABD_END_Pos)

#define UART_FLAG_STATUS_RX_IDLE_Pos		1
#define UART_FLAG_STATUS_RX_IDLE_Msk		(1UL << UART_FLAG_STATUS_RX_IDLE_Pos)

#define UART_FLAG_START_OFFSET_VALID_Pos	2
#define UART_FLAG_START_OFFSET_VALID_Msk    (1UL << UART_FLAG_START_OFFSET_VALID_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the FIFO_FLUSH register.
//
//*****************************************************************************
#define UART_FIFO_FLUSH_RX_Pos              0
#define UART_FIFO_FLUSH_RX_Msk              (1UL << UART_FIFO_FLUSH_RX_Pos)

#define UART_FIFO_FLUSH_TX_Pos              1
#define UART_FIFO_FLUSH_TX_Msk              (1UL << UART_FIFO_FLUSH_TX_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the FIFO_STATUS register.
//
//*****************************************************************************
#define UART_STAT_RXFIFO_EMPTY_Pos			0
#define UART_STAT_RXFIFO_EMPTY_Msk	 		(1UL << UART_STAT_RXFIFO_EMPTY_Pos)
#define UART_STAT_RXFIFO_EMPTY              (1UL << UART_STAT_RXFIFO_EMPTY_Pos)

#define UART_STAT_RXFIFO_FULL_Pos			1
#define UART_STAT_RXFIFO_FULL_Msk			(1UL << UART_STAT_RXFIFO_FULL_Pos)
#define UART_STAT_RXFIFO_FULL  	            (1UL << UART_STAT_RXFIFO_FULL_Pos)

#define UART_STAT_RXFIFO_LEVEL_Pos			2
#define UART_STAT_RXFIFO_LEVEL_Msk    		(3UL << UART_STAT_RXFIFO_LEVEL_Pos)
#define UART_STAT_RXFIFO_LEVEL_4_4    		(0UL << UART_STAT_RXFIFO_LEVEL_Pos)
#define UART_STAT_RXFIFO_LEVEL_3_4    		(1UL << UART_STAT_RXFIFO_LEVEL_Pos)
#define UART_STAT_RXFIFO_LEVEL_2_4    		(2UL << UART_STAT_RXFIFO_LEVEL_Pos)
#define UART_STAT_RXFIFO_LEVEL_1_4    		(3UL << UART_STAT_RXFIFO_LEVEL_Pos)

#define UART_STAT_TXFIFO_EMPTY_Pos			0
#define UART_STAT_TXFIFO_EMPTY_Msk  		(1UL << UART_STAT_TXFIFO_EMPTY_Pos)
#define UART_STAT_TXFIFO_EMPTY  			(1UL << UART_STAT_TXFIFO_EMPTY_Pos)

#define UART_STAT_TXFIFO_FULL_Pos			1
#define UART_STAT_TXFIFO_FULL_Msk           (1UL << UART_STAT_TXFIFO_FULL_Pos)
#define UART_STAT_TXFIFO_FULL               (1UL << UART_STAT_TXFIFO_FULL_Pos)

#define UART_STAT_TXFIFO_LEVEL_Pos			2
#define UART_STAT_TXFIFO_LEVEL_Msk          (3UL << UART_STAT_TXFIFO_LEVEL_Pos)
#define UART_STAT_TXFIFO_LEVEL_1_4    		(0UL << UART_STAT_TXFIFO_LEVEL_Pos)
#define UART_STAT_TXFIFO_LEVEL_2_4    		(1UL << UART_STAT_TXFIFO_LEVEL_Pos)
#define UART_STAT_TXFIFO_LEVEL_3_4    		(2UL << UART_STAT_TXFIFO_LEVEL_Pos)
#define UART_STAT_TXFIFO_LEVEL_4_4    		(3UL << UART_STAT_TXFIFO_LEVEL_Pos)

#define UART_STAT_TXFSM_FLAG_Pos	        4
#define UART_STAT_TXFSM_FLAG_Msk	        (1UL << UART_STAT_TXFSM_FLAG_Pos)
#define UART_STAT_TXFSM_FLAG                (1UL << UART_STAT_TXFSM_FLAG_Pos)

#define FRAME_PARITY_ERR_TO_WFIFO_EN_Pos    1
#define FRAME_PARITY_ERR_TO_WFIFO_EN_Msk    (1UL << FRAME_PARITY_ERR_TO_WFIFO_EN_Pos)
#define FRAME_PARITY_ERR_TO_WFIFO_EN        FRAME_PARITY_ERR_TO_WFIFO_EN_Msk
//*****************************************************************************
//
// The following are defines for the bit fields in the WAKEUP_MODE register.
//
//*****************************************************************************
#define UART_WAKEUP_ENABLE_Pos				0  
#define UART_WAKEUP_ENABLE_Msk              (1UL << UART_WAKEUP_ENABLE_Pos)    

#define UART_WAKEUP_MODE_Pos				1  
#define UART_WAKEUP_PATTERN_BYTE_Msk	    (3UL << UART_WAKEUP_MODE_Pos) 
#define UART_WAKEUP_PATTERN_ANY_BYTE        (0UL << UART_WAKEUP_MODE_Pos)  
#define UART_WAKEUP_PATTERN_ONE_BYTE        (1UL << UART_WAKEUP_MODE_Pos)  
#define UART_WAKEUP_PATTERN_TWO_BYTE        (2UL << UART_WAKEUP_MODE_Pos) 

#define UART_WAKEUP_DATA_CLEAR_Pos			3
#define UART_WAKEUP_DATA_CLEAR_Msk			(1UL << UART_WAKEUP_DATA_CLEAR_Pos)
#define UART_WAKEUP_DATA_CLEAR_DIS			(0UL << UART_WAKEUP_DATA_CLEAR_Pos)
#define UART_WAKEUP_DATA_CLEAR_ENA			(1UL << UART_WAKEUP_DATA_CLEAR_Pos)

#define UART_WAKEUP_FIFO_CHECK_Pos			4
#define UART_WAKEUP_FIFO_CHECK_Msk          (3UL << UART_WAKEUP_FIFO_CHECK_Pos)
#define UART_WAKEUP_CHECK_LEVEL_3_4         (0UL << UART_WAKEUP_FIFO_CHECK_Pos)
#define UART_WAKEUP_CHECK_LEVEL_2_4         (1UL << UART_WAKEUP_FIFO_CHECK_Pos)
#define UART_WAKEUP_CHECK_LEVEL_1_4         (2UL << UART_WAKEUP_FIFO_CHECK_Pos)
#define UART_WAKEUP_CHECK_DIS               (3UL << UART_WAKEUP_FIFO_CHECK_Pos)

#define UART_WAKEUP_PATTERN_Pos             8           
#define UART_WAKEUP_PATTERN_Msk             (0xFFFF << UART_WAKEUP_PATTERN_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the SQUENCE_DETECT register.
//
//*****************************************************************************
#define UART_SQUENCE_DETECT_Pos				    0  
#define UART_SQUENCE_DETECT_Msk				    (1UL << UART_SQUENCE_DETECT_Pos)  

#define UART_SQUENCE_DETECT_VALID_Pos			0  
#define UART_SQUENCE_DETECT_VALID_Msk			(3UL << UART_SQUENCE_DETECT_VALID_Pos)
#define UART_SQUENCE_DETECT_VALID_5BITS		    (0UL << UART_SQUENCE_DETECT_VALID_Pos)  
#define UART_SQUENCE_DETECT_VALID_6BITS		    (1UL << UART_SQUENCE_DETECT_VALID_Pos) 
#define UART_SQUENCE_DETECT_VALID_7BITS		    (2UL << UART_SQUENCE_DETECT_VALID_Pos) 
#define UART_SQUENCE_DETECT_VALID_8BITS		    (3UL << UART_SQUENCE_DETECT_VALID_Pos) 

//*****************************************************************************
//
// The following are defines for the bit fields in the FLOW_CTRL register.
//
//*****************************************************************************
#define UART_FLOW_CTL_EN_Pos				    0  
#define UART_FLOW_CTL_EN_Msk				    (1UL << UART_FLOW_CTL_EN_Pos)  

#define UART_FLOW_CTL_SW_EN_Pos                 1
#define UART_FLOW_CTL_SW_EN_Msk				    (1UL << UART_FLOW_CTL_SW_EN_Pos)
#define UART_FLOW_CTL_AUTO				        (0UL << UART_FLOW_CTL_SW_EN_Pos) 
#define UART_FLOW_CTL_SW_CTL			        (1UL << UART_FLOW_CTL_SW_EN_Pos) 

#define UART_FLOW_CTL_VALID_LEVEL_Pos           2
#define UART_FLOW_CTL_VALID_LEVEL_Msk			(1UL << UART_FLOW_CTL_VALID_LEVEL_Pos)
#define UART_FLOW_CTL_VALID_LEVEL_LOW			(0UL << UART_FLOW_CTL_VALID_LEVEL_Pos) 
#define UART_FLOW_CTL_VALID_LEVEL_HIGH			(1UL << UART_FLOW_CTL_VALID_LEVEL_Pos) 

#define UART_FLOW_CTL_RTS_TRIGGER_Pos           3
#define UART_FLOW_CTL_RTS_TRIGGER_Msk	        (3UL << UART_FLOW_CTL_RTS_TRIGGER_Pos) 
#define UART_FLOW_CTL_RTS_TRIGGER_LEVEL_3_4	    (0UL << UART_FLOW_CTL_RTS_TRIGGER_Pos)  
#define UART_FLOW_CTL_RTS_TRIGGER_LEVEL_2_4	    (1UL << UART_FLOW_CTL_RTS_TRIGGER_Pos)  
#define UART_FLOW_CTL_RTS_TRIGGER_LEVEL_1_4	    (2UL << UART_FLOW_CTL_RTS_TRIGGER_Pos)  
#define UART_FLOW_CTL_RTS_TRIGGER_DIS    	    (3UL << UART_FLOW_CTL_RTS_TRIGGER_Pos)

#define UART_FLOW_CTL_CTS_VALUE_Pos             0
#define UART_FLOW_CTL_CTS_VALUE_Msk				(1UL << UART_FLOW_CTL_CTS_VALUE_Pos)

#define UART_FLOW_CTL_RTS_VALUE_Pos             1
#define UART_FLOW_CTL_RTS_VALUE_Msk				(1UL << UART_FLOW_CTL_RTS_VALUE_Pos) 

//*****************************************************************************
//
// The following are defines for the bit fields in the RX_TIMEOUT_CONFIG register.
//
//*****************************************************************************
#define UART_RX_TIMEOUT_EN_Pos                  0
#define UART_RX_TIMEOUT_EN_Msk                  (1UL << UART_RX_TIMEOUT_EN_Pos) 

#define UART_RX_TIMEOUT_START_Pos               1
#define UART_RX_TIMEOUT_START_Msk               (1UL << UART_RX_TIMEOUT_START_Pos)
#define UART_RX_TIMEOUT_START_NO_MATTER         (0UL << UART_RX_TIMEOUT_START_Pos) 
#define UART_RX_TIMEOUT_START_FIFO_NEMPTY       (1UL << UART_RX_TIMEOUT_START_Pos) 

#define UART_RX_TIMEOUT_VALUE_Pos               8
#define UART_RX_TIMEOUT_VALUE_Msk               (0x1F << UART_RX_TIMEOUT_VALUE_Pos) 

//*****************************************************************************
//
// The following are defines for the bit fields in the RX_START_OFFSET_CONFIG register.
//
//*****************************************************************************
#define UART_START_OFFSET_EN_Pos                0
#define UART_START_OFFSET_EN_Msk                (1UL << UART_START_OFFSET_EN_Pos)

#define UART_START_OFFSET_VALUE_Pos             8
#define UART_START_OFFSET_VALUE_Msk             (0x1F << UART_START_OFFSET_VALUE_Pos) 



#endif // __HW_UART_H__
