#ifndef __QSPI_FLASH_H__
#define __QSPI_FLASH_H__

#include "hw_qspi_flash.h"
#include "hw_ints.h"
#include "xinyi2100.h"

#include "hw_types.h"
#include "debug.h"
#include "interrupt.h"

#define FLASH_INDIRECT_MAX_BYTE     0x8000

#define BITS_PER_LONG       32
#define BIT(nr)             (1UL << (nr))
#define BIT_MASK(nr)        (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)        ((nr) / BITS_PER_LONG)

#define readw(addr)         (*(volatile uint16_t *)  (addr))
#define readl(addr)         (*(volatile uint32_t *)  (addr))
#define readb(addr)         (*(volatile uint8_t *)   (addr))
#define writew(b,addr)      ((*(volatile uint16_t *) (addr)) = (b))
#define writel(b,addr)      ((*(volatile uint32_t *) (addr)) = (b))
#define writeb(b,addr)      ((*(volatile uint8_t  *) (addr)) = (b))

#define DIV_ROUND_UP(n, d)	(((n) + (d) - 1) / (d))

/* SPI mode flags */
#define SPI_CPHA            BIT(0)			/* clock phase */
#define SPI_CPOL            BIT(1)			/* clock polarity */
#define SPI_MODE_0          (0|0)			/* (original MicroWire) */
#define SPI_MODE_1          (0|SPI_CPHA)
#define SPI_MODE_2          (SPI_CPOL|0)
#define SPI_MODE_3          (SPI_CPOL|SPI_CPHA)
#define SPI_CS_HIGH         BIT(2)			/* CS active high */
#define SPI_LSB_FIRST       BIT(3)			/* per-word bits-on-wire */
#define SPI_3WIRE           BIT(4)			/* SI/SO signals shared */
#define SPI_LOOP            BIT(5)			/* loopback mode */
#define SPI_SLAVE           BIT(6)			/* slave mode */
#define SPI_PREAMBLE        BIT(7)			/* Skip preamble bytes */
#define SPI_TX_BYTE         BIT(8)			/* transmit with 1 wire byte */
#define SPI_TX_DUAL         BIT(9)			/* transmit with 2 wires */
#define SPI_TX_QUAD         BIT(10)			/* transmit with 4 wires */
#define SPI_RX_SLOW         BIT(11)			/* receive with 1 wire slow */
#define SPI_RX_DUAL         BIT(12)			/* receive with 2 wires */
#define SPI_RX_QUAD         BIT(13)			/* receive with 4 wires */


#define QSPI_IS_ADDR(cmd_len)               (cmd_len > 1 ? 1 : 0)

#define QSPI_NO_DECODER_MAX_CS              4
#define QSPI_DECODER_MAX_CS                 16
#define QSPI_READ_CAPTURE_MAX_DELAY	        16

typedef struct {
	unsigned char	*regbase;
	unsigned char	*ahbbase;

	unsigned int    page_size;
	unsigned int    block_size;
	unsigned int    tshsl_ns;
	unsigned int    tsd2d_ns;
	unsigned int    tchsh_ns;
	unsigned int    tslch_ns;
	unsigned int    flash_type;
	unsigned int    addr_bytes;
    unsigned int    otp_base;
} QSPI_FLASH_Def;

/**
  * @brief  flash定义结构体
  */
extern QSPI_FLASH_Def xinyi_flash;

/* Functions call declaration */
extern void qspi_apb_controller_init(QSPI_FLASH_Def *plat);
extern void qspi_apb_controller_enable(void);
extern void qspi_apb_controller_disable(void);
extern void qspi_wait_indirect_complete(void);
extern int qspi_apb_command_read(unsigned int cmdlen, const uint8_t *cmdbuf, unsigned int rxlen, uint8_t *rxbuf);
extern int qspi_apb_command_write(unsigned int cmdlen, const uint8_t *cmdbuf, unsigned int txlen,  const uint8_t *txbuf);
extern void qspi_apb_config_baudrate_div(unsigned int ref_clk_hz, unsigned int sclk_hz);
extern void qspi_apb_delay(unsigned int ref_clk, unsigned int sclk_hz, unsigned int tshsl_ns, unsigned int tsd2d_ns, unsigned int tchsh_ns, unsigned int tslch_ns);
extern void qspi_wait_idle(void);
extern void qspi_rbuf_wait_idle(void);
extern void qspi_wbuf_wait_idle(void);
extern void qspi_wait_writeburst_done(void);
extern void qspi_apb_indirect_enable(void);
extern void qspi_apb_indirect_disable(void);
extern void qspi_apb_indirect_write(QSPI_FLASH_Def *plat, unsigned int dst, unsigned int n_bytes);
extern void qspi_apb_indirect_read(QSPI_FLASH_Def *plat, unsigned int src, unsigned int n_bytes);
extern void qspi_apb_set_read_instruction(unsigned char devId, unsigned int config);
extern void qspi_apb_set_write_instruction(unsigned char devId, unsigned int config);
extern void QSPIIntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void));
extern void QSPIIntUnregister(unsigned long *g_pRAMVectors);
extern void qspi_dma_Config(unsigned char ucConfig);
extern void qspi_dma_bstsize_set(unsigned char ucBstsize);
extern unsigned char qspi_dma_bstsize_get(void);

extern void qspi_sus_cmd_check_enable(void);
extern void qspi_sus_cmd_set(unsigned int sus_cmd);
extern void qspi_sus_cs_set(unsigned char sus_cs);
extern void qspi_block_alldevice_set(unsigned char flag);
extern unsigned char qspi_block_alldevice_get(void);
extern void qspi_tsus_cnt_set(unsigned char tresume_cnt, unsigned int tsus_cnt);
extern void qspi_sus_opcode_set(unsigned char sus_opcode);
extern void qspi_resume_opcode_set(unsigned char resume_opcode);
extern void qspi_tsus_timeout_cnt_set(unsigned char tsus_timeout_cnt);
extern void qspi_clear_sus_cmd_hit(void);

extern void qspi_ap_erase_write_req(void);
extern void qspi_ap_dma_erase_write_req(void);
extern void qspi_ap_wait_erase_write_req_ack(void);
extern void qspi_cp_send_erase_write_req_ack(void);
extern void qspi_cp_wait_erase_write_done(void);
extern void qspi_ap_send_erase_write_done(void);
extern void qspi_cp_erase_write_req(void);
extern void qspi_ap_send_erase_write_req_ack(void);
extern void qspi_ap_wait_erase_write_done(void);
extern void qspi_cp_wait_erase_write_req_ack(void);
extern void qspi_cp_send_erase_write_done(void);

#endif /* __QSPI_FLASH_H__ */



