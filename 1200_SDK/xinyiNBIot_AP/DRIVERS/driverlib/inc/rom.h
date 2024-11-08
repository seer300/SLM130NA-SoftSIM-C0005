#ifndef __ROM_H__
#define __ROM_H__

#define ROM_VERSION_B1

#include "abdhwd.h"
#include "adc.h"
#include "cache.h"
#include "common.h"
#include "crc.h"
#include "crypto.h"
#include "csp.h"
#include "debug.h"
#include "dma.h"
#include "flash_vendor.h"
#include "gpio.h"
#include "hw_abdhwd.h"
#include "hw_adc.h"
#include "hw_cache.h"
#include "hw_crc.h"
#include "hw_crypto.h"
#include "hw_csp.h"
#include "hw_dfe.h"
#include "hw_dma.h"
#include "hw_gpio.h"
#include "hw_i2c.h"
#include "hw_ints.h"
#include "hw_iso7816.h"
#include "hw_keyscan.h"
#include "hw_lcdc.h"
#include "hw_lptimer.h"
#include "hw_mcnt.h"
//#include "hw_memmap.h"
#include "hw_nvic.h"
#include "hw_phytimer.h"
#include "hw_prcm.h"
#include "hw_qspi_flash.h"
#include "hw_sema.h"
#include "hw_spi.h"
#include "hw_tick.h"
#include "hw_timer.h"
#include "hw_trng.h"
#include "hw_types.h"
#include "hw_uart.h"
#include "hw_utc.h"
#include "hw_watchdog.h"
#include "i2c.h"
#include "interrupt.h"
#include "iso7816.h"
#include "keyscan.h"
#include "lcdc.h"
#include "lptimer.h"
#include "mcnt.h"
#include "phytimer.h"
#include "prcm.h"
#include "qspi_flash.h"
#include "sema.h"
#include "spi.h"
#include "systick.h"
#include "tick.h"
#include "timer.h"
#include "trng.h"
#include "uart.h"
#include "utc.h"
#include "watchdog.h"


#ifdef ROM_VERSION_B1

#define func_SHA1ProcessMessageBlock                            0x000046c7
#define func_SHA1PadMessage                                     0x000048a9
#define func_SHA1Finalize                                       0x00004923
#define func_rsa_check_context                                  0x00004a7d
#define func_rsa_prepare_blinding                               0x00005089
#define func_all_or_nothing_int                                 0x00005505
#define func_size_greater_than                                  0x0000550d
#define func_if_int                                             0x00005513
#define func_mem_move_to_left                                   0x00005529
#define func_myrand                                             0x000057ed
#define func_mbedtls_mpi_zeroize                                0x00005899
#define func_mpi_safe_cond_assign                               0x000059a1
#define func_mbedtls_clz                                        0x00005a61
#define func_mpi_get_digit                                      0x00005ab3
#define func_mpi_uint_bigendian_to_host_c                       0x00005ba1
#define func_mpi_bigendian_to_host                              0x00005bbb
#define func_mpi_sub_hlp                                        0x00006047
#define func_mpi_mul_hlp                                        0x00006249
#define func_mbedtls_int_div_int                                0x00006d7f
#define func_mpi_montg_init                                     0x000071a1
#define func_mpi_montmul                                        0x000071cb
#define func_mpi_montred                                        0x00007279
#define func_qspi_apb_cmd2addr                                  0x00007ac7
#define func_qspi_apb_exec_flash_cmd                            0x00007c27
#define func_printch                                            0x00008347
#define func_printstr                                           0x00008379
#define func_IntDefaultHandler                                  0x000083d9
#define func__fp_digits                                         0x0000e92d
#define func__printf_core                                       0x0000eab1
#define func__printf_post_padding                               0x0000f18d
#define func__printf_pre_padding                                0x0000f1b1
#define func__sputc                                             0x0000f1df
#define func___main                                             0x000000c1
#define func__main_stk                                          0x000000c1
#define func__main_scatterload                                  0x000000c5
#define func___main_after_scatterload                           0x000000c9
#define func__main_clock                                        0x000000c9
#define func__main_cpp_init                                     0x000000c9
#define func__main_init                                         0x000000c9
#define func___rt_final_cpp                                     0x000000d1
#define func___rt_final_exit                                    0x000000d1
#define func___get_PSP                                          0x000000d5
#define func___set_PSP                                          0x000000db
#define func___get_MSP                                          0x000000e1
#define func___set_MSP                                          0x000000e7
#define func___REV16                                            0x000000ed
#define func___REVSH                                            0x000000f1
#define func_Reset_Handler                                      0x000000f5
#define func_NMI_Handler                                        0x000000fd
#define func_HardFault_Handler                                  0x000000ff
#define func_MemManage_Handler                                  0x00000101
#define func_BusFault_Handler                                   0x00000103
#define func_UsageFault_Handler                                 0x00000105
#define func_SVC_Handler                                        0x00000107
#define func_DebugMon_Handler                                   0x00000109
#define func_PendSV_Handler                                     0x0000010b
#define func_SysTick_Handler                                    0x0000010d
#define func_IRQ0_Handler                                       0x0000010f
#define func_IRQ10_Handler                                      0x0000010f
#define func_IRQ11_Handler                                      0x0000010f
#define func_IRQ12_Handler                                      0x0000010f
#define func_IRQ13_Handler                                      0x0000010f
#define func_IRQ14_Handler                                      0x0000010f
#define func_IRQ15_Handler                                      0x0000010f
#define func_IRQ16_Handler                                      0x0000010f
#define func_IRQ17_Handler                                      0x0000010f
#define func_IRQ18_Handler                                      0x0000010f
#define func_IRQ19_Handler                                      0x0000010f
#define func_IRQ1_Handler                                       0x0000010f
#define func_IRQ20_Handler                                      0x0000010f
#define func_IRQ21_Handler                                      0x0000010f
#define func_IRQ22_Handler                                      0x0000010f
#define func_IRQ23_Handler                                      0x0000010f
#define func_IRQ24_Handler                                      0x0000010f
#define func_IRQ25_Handler                                      0x0000010f
#define func_IRQ26_Handler                                      0x0000010f
#define func_IRQ27_Handler                                      0x0000010f
#define func_IRQ28_Handler                                      0x0000010f
#define func_IRQ29_Handler                                      0x0000010f
#define func_IRQ2_Handler                                       0x0000010f
#define func_IRQ30_Handler                                      0x0000010f
#define func_IRQ31_Handler                                      0x0000010f
#define func_IRQ3_Handler                                       0x0000010f
#define func_IRQ4_Handler                                       0x0000010f
#define func_IRQ5_Handler                                       0x0000010f
#define func_IRQ6_Handler                                       0x0000010f
#define func_IRQ7_Handler                                       0x0000010f
#define func_IRQ8_Handler                                       0x0000010f
#define func_IRQ9_Handler                                       0x0000010f
#define func_SystemInit                                         0x00000119
#define func_bl_delay                                           0x00000199
#define func_FLASH_CheckReadMode                                0x000001af
#define func_Flash_RDID_Check                                   0x000001bf
#define func_flash_init_flag_check_and_set                      0x000003a5
#define func_DmaAddrCheck                                       0x000003e7
#define func_FlashHeaderCheck                                   0x00000417
#define func_SecondaryBootHeaderCheck                           0x000004bb
#define func_DMAChannelMemcpy                                   0x000004f5
#define func_CMD_GET_CHIP_ID_HANDLER                            0x000005d1
#define func_CMD_GET_VERSION_HANDLER                            0x00000611
#define func_CMD_GET_TESTVERSION_HANDLER                        0x00000637
#define func_CMD_GET_COMMANDS_HANDLER                           0x0000065d
#define func_CMD_ERASE_MEMORY_HANDLER                           0x00000763
#define func_CMD_WRITE_MEMORY_HANDLER                           0x00000851
#define func_CMD_READ_MEMORY_HANDLER                            0x00000b9d
#define func_CMD_COPY_MEMORY_HANDLER                            0x00000dbd
#define func_CMD_COMPARE_MEMORY_HANDLER                         0x00000e93
#define func_CMD_FAST_WRITE_MEMORY_HANDLER                      0x00000fcd
#define func_CMD_RUN_PROGRAM_HANDLER                            0x0000139d
#define func_CMD_ENABLE_CP_CORE_HANDLER                         0x0000146f
#define func_CMD_FLASH_QE_SET_HANDLER                           0x00001517
#define func_CMD_FLASH_XIP_EXIT_HANDLER                         0x0000154b
#define func_CMD_FLASH_XIP_ENTER_HANDLER                        0x0000156b
#define func_CMD_FLASH_OTP_READ_HANDLER                         0x0000158d
#define func_CMD_FLASH_OTP_WRITE_HANDLER                        0x000015b5
#define func_CMD_FLASH_UNIQUE_ID_READ_HANDLER                   0x000015d7
#define func_CMD_FLASH_RDID_READ_HANDLER                        0x00001623
#define func_CMD_FLASH_OTP_ERASE_HANDLER                        0x00001699
#define func_CMD_FLASH_STATUS_REG_READ_HANDLER                  0x00001717
#define func_CMD_FLASH_STATUS_REG_WRITE_HANDLER                 0x00001781
#define func_CMD_FLASH_FAST_ERASE_HANDLER                       0x00001809
#define func_CMD_FLASH_BLOCK_ERASE_HANDLER                      0x000019c1
#define func_CMD_REG_BITS_WRITE_HANDLER                         0x00001b2f
#define func_CMD_UPDATE_UART_BAUDRATE_HANDLER                   0x00001be1
#define func_FLASH_OTP_Header_Read_and_Check                    0x00001c4d
#define func_OTP_Reg_Set                                        0x00001d1b
#define func_SPISlave_Send_Response                             0x00001d65
#define func_SPISlave_Send_Response_WithTimeout                 0x00001d91
#define func_SPISlave_Recv_Response                             0x00001dbd
#define func_SPISlave_Send_Data                                 0x00001de5
#define func_SPI_CMD_GET_CHIP_ID_HANDLER                        0x00001e21
#define func_SPI_CMD_GET_VERSION_HANDLER                        0x00001e6d
#define func_SPI_CMD_ERASE_MEMORY_HANDLER                       0x00001ea3
#define func_SPI_CMD_ERASE_MEMORY_EXT_HANDLER                   0x00001fcd
#define func_SPI_CMD_WRITE_MEMORY_HANDLER                       0x000020eb
#define func_SPI_CMD_READ_MEMORY_HANDLER                        0x00002477
#define func_SPI_CMD_COPY_MEMORY_HANDLER                        0x000026af
#define func_SPI_CMD_COMPARE_MEMORY_HANDLER                     0x000027c5
#define func_SPI_CMD_FAST_WRITE_MEMORY_HANDLER                  0x00002921
#define func_SPI_CMD_RUN_PROGRAM_HANDLER                        0x00002d47
#define func_SPI_CMD_ENABLE_CP_CORE_HANDLER                     0x00002e2d
#define func_SPI_CMD_FLASH_QE_SET_HANDLER                       0x00002ed3
#define func_SPI_CMD_FLASH_XIP_EXIT_HANDLER                     0x00002f41
#define func_SPI_CMD_FLASH_XIP_ENTER_HANDLER                    0x00002fa7
#define func_SPI_CMD_FLASH_OTP_READ_HANDLER                     0x00002fe5
#define func_SPI_CMD_FLASH_OTP_WRITE_HANDLER                    0x00003011
#define func_SPI_CMD_FLASH_UNIQUE_ID_READ_HANDLER               0x00003037
#define func_SPI_CMD_FLASH_RDID_READ_HANDLER                    0x000030a3
#define func_SPI_CMD_FLASH_OTP_ERASE_HANDLER                    0x00003101
#define func_SPI_CMD_FLASH_STATUS_REG_READ_HANDLER              0x0000319d
#define func_SPI_CMD_FLASH_STATUS_REG_WRITE_HANDLER             0x00003221
#define func_SPI_CMD_FLASH_FAST_ERASE_HANDLER                   0x000032bf
#define func_SPI_CMD_FLASH_BLOCK_ERASE_HANDLER                  0x000034c3
#define func_SPI_CMD_REG_BITS_WRITE_HANDLER                     0x0000365b
#define func_spi_boot_process                                   0x0000372b
#define func_rsa_public_decryption                              0x00003a21
#define func_b1_flash_poweroff                                  0x00003af7
#define func_b1_flash_poweron                                   0x00003b53
#define func_bl_flash_cold_reset                                0x00003b9f
#define func_bl_flash_warm_reset                                0x00003baf
#define func_main                                               0x00003bd3
#define func_SHA1Reset                                          0x00004699
#define func_SHA1Input                                          0x00004835
#define func_SHA1FinalBits                                      0x00004943
#define func_SHA1Result                                         0x000049a7
#define func_mbedtls_rsa_import                                 0x00004a01
#define func_mbedtls_rsa_deduce_crt                             0x00004b2d
#define func_mbedtls_rsa_deduce_private_exponent                0x00004b9b
#define func_mbedtls_rsa_deduce_primes                          0x00004c51
#define func_mbedtls_rsa_complete                               0x00004dfb
#define func_mbedtls_rsa_set_padding                            0x00004fd3
#define func_mbedtls_rsa_init                                   0x00004fe3
#define func_mbedtls_rsa_public                                 0x00005005
#define func_mbedtls_rsa_private                                0x00005173
#define func_mbedtls_rsa_rsaes_pkcs1_v15_encrypt                0x000053ed
#define func_mbedtls_rsa_pkcs1_encrypt                          0x000054d1
#define func_mbedtls_rsa_rsaes_pkcs1_v15_decrypt                0x0000557b
#define func_mbedtls_rsa_pkcs1_decrypt                          0x0000572d
#define func_mbedtls_rsa_free                                   0x0000577d
#define func_mbedtls_rsa_public_decryption                      0x00005807
#define func_mbedtls_platform_zeroize                           0x0000588d
#define func_mbedtls_mpi_init                                   0x000058a3
#define func_mbedtls_mpi_free                                   0x000058b3
#define func_mbedtls_mpi_grow                                   0x000058d5
#define func_mbedtls_mpi_copy                                   0x0000592d
#define func_mbedtls_mpi_lset                                   0x000059c5
#define func_mbedtls_mpi_get_bit                                0x00005a07
#define func_mbedtls_mpi_lsb                                    0x00005a2f
#define func_mbedtls_mpi_bitlen                                 0x00005a77
#define func_mbedtls_mpi_size                                   0x00005aa7
#define func_mbedtls_mpi_read_string                            0x00005ae9
#define func_mbedtls_mpi_read_hex                               0x00005b9b
#define func_mbedtls_mpi_read_binary                            0x00005be5
#define func_mbedtls_mpi_read_binary_small                      0x00005c59
#define func_mbedtls_mpi_write_binary                           0x00005cb5
#define func_mbedtls_mpi_shift_l                                0x00005d29
#define func_mbedtls_mpi_shift_r                                0x00005dd3
#define func_mbedtls_mpi_cmp_abs                                0x00005e55
#define func_mbedtls_mpi_cmp_mpi                                0x00005ec1
#define func_mbedtls_mpi_cmp_int                                0x00005f53
#define func_mbedtls_mpi_add_abs                                0x00005f89
#define func_mbedtls_mpi_sub_abs                                0x0000607d
#define func_mbedtls_mpi_add_mpi                                0x00006123
#define func_mbedtls_mpi_sub_mpi                                0x0000617d
#define func_mbedtls_mpi_add_int                                0x000061d9
#define func_mbedtls_mpi_sub_int                                0x00006211
#define func_mbedtls_mpi_mul_mpi                                0x00006c8b
#define func_mbedtls_mpi_mul_int                                0x00006d5b
#define func_mbedtls_mpi_div_mpi                                0x00006e31
#define func_mbedtls_mpi_mod_mpi                                0x00007131
#define func_mbedtls_mpi_exp_mod                                0x0000729b
#define func_mbedtls_mpi_gcd                                    0x000076bd
#define func_mbedtls_mpi_fill_random                            0x000077bf
#define func_mbedtls_mpi_inv_mod                                0x00007831
#define func_qspi_memcpy                                        0x00007aa1
#define func_qspi_memset                                        0x00007ab7
#define func_qspi_apb_controller_enable                         0x00007ae3
#define func_qspi_apb_controller_disable                        0x00007aef
#define func_qspi_wait_indirect_complete                        0x00007afb
#define func_qspi_wait_idle                                     0x00007b0d
#define func_qspi_rbuf_wait_idle                                0x00007b17
#define func_qspi_wbuf_wait_idle                                0x00007b23
#define func_qspi_wait_writeburst_done                          0x00007b2f
#define func_qspi_apb_config_baudrate_div                       0x00007b45
#define func_qspi_apb_delay                                     0x00007b77
#define func_qspi_apb_controller_init                           0x00007bef
#define func_qspi_apb_command_read                              0x00007c47
#define func_qspi_apb_command_write                             0x00007cab
#define func_qspi_apb_indirect_enable                           0x00007d47
#define func_qspi_apb_indirect_disable                          0x00007d4f
#define func_qspi_apb_indirect_write                            0x00007d57
#define func_qspi_apb_indirect_read                             0x00007d6d
#define func_qspi_apb_set_read_instruction                      0x00007d83
#define func_qspi_apb_set_write_instruction                     0x00007d8d
#define func_QSPIIntRegister                                    0x00007d97
#define func_QSPIIntUnregister                                  0x00007dab
#define func_qspi_dma_config                                    0x00007dbf
#define func_qspi_dma_bstsize_set                               0x00007dd5
#define func_qspi_dma_bstsize_get                               0x00007ddd
#define func_qspi_sus_cmd_check_enable                          0x00007de5
#define func_qspi_sus_cmd_set                                   0x00007def
#define func_qspi_sus_cs_set                                    0x00007df7
#define func_qspi_block_alldevice_set                           0x00007e09
#define func_qspi_block_alldevice_get                           0x00007e1b
#define func_qspi_tsus_cnt_set                                  0x00007e27
#define func_qspi_sus_opcode_set                                0x00007e33
#define func_qspi_resume_opcode_set                             0x00007e3b
#define func_qspi_tsus_timeout_cnt_set                          0x00007e43
#define func_qspi_clear_sus_cmd_hit                             0x00007e4b
#define func_qspi_ap_erase_write_req                            0x00007e63
#define func_qspi_ap_dma_erase_write_req                        0x00007e71
#define func_qspi_ap_wait_erase_write_req_ack                   0x00007e7f
#define func_qspi_cp_send_erase_write_req_ack                   0x00007e8b
#define func_qspi_cp_wait_erase_write_done                      0x00007e99
#define func_qspi_ap_send_erase_write_done                      0x00007ea5
#define func_UARTEnable                                         0x00007ec1
#define func_UARTDisable                                        0x00007ecb
#define func_UARTFIFOEnable                                     0x00007ed5
#define func_UARTFIFODisable                                    0x00007ef5
#define func_UARTDmaTransferEnable                              0x00007f15
#define func_UARTDmaTransferDisable                             0x00007f1f
#define func_UARTFIFOLevelSet                                   0x00007f29
#define func_UARTTxWaitSet                                      0x00007f37
#define func_UARTWaitTxDone                                     0x00007f3b
#define func_UARTConfigSetExpClk                                0x00007f43
#define func_UARTConfigGetExpClk                                0x00007fb7
#define func_UARTAutoBaudrate                                   0x00007fe3
#define func_UARTABDEndStatus                                   0x0000800f
#define func_UARTIntRegister                                    0x00008017
#define func_UARTIntUnregister                                  0x00008027
#define func_UARTIntEnable                                      0x0000803b
#define func_UARTIntDisable                                     0x00008043
#define func_UARTIntReadAndClear                                0x0000804b
#define func_UARTIntMasked                                      0x00008055
#define func_UARTRxIdle                                         0x00008059
#define func_UARTFIFOFlush                                      0x00008061
#define func_UARTRxFifoStatusGet                                0x00008081
#define func_UARTTxFifoStatusGet                                0x000080ab
#define func_UARTFIFOByteLevel                                  0x000080d5
#define func_UARTWakeUpModeConfig                               0x000080e5
#define func_UARTWakeUpModeEnable                               0x000080f1
#define func_UARTWakeUpModeDisable                              0x000080fb
#define func_UARTSequenceDetectModeSet                          0x00008105
#define func_UARTSequenceDetectEnable                           0x00008115
#define func_UARTSequenceDetectDisable                          0x0000811f
#define func_UARTTimeOutConfig                                  0x00008129
#define func_UARTTimeOutEnable                                  0x00008131
#define func_UARTTimeOutDisable                                 0x0000813d
#define func_UARTRXEnaStatus                                    0x00008149
#define func_UARTFlowCtrlEnable                                 0x00008151
#define func_UARTFlowCtrlDisable                                0x0000815d
#define func_UARTFlowCtrlConfig                                 0x00008169
#define func_UARTFlowCtrlRtsGet                                 0x00008177
#define func_UARTFlowCtrlRtsSet                                 0x00008181
#define func_UARTFlowCtrlRtsClear                               0x0000818d
#define func_UARTFlowCtrlCtsGet                                 0x00008199
#define func_UARTFlowCtrlCtsSet                                 0x000081a3
#define func_UARTFlowCtrlCtsClear                               0x000081af
#define func_UARTStartOffsetConfig                              0x000081bb
#define func_UARTStartOffsetEnable                              0x000081c9
#define func_UARTStartOffsetFlag                                0x000081d5
#define func_UARTCharGet                                        0x000081dd
#define func_UARTCharGetWithTimeout                             0x000081e9
#define func_UARTHalfWordGet                                    0x00008215
#define func_UARTWordGet                                        0x00008229
#define func_UARTCharGetNonBlocking                             0x0000823b
#define func_UARTHalfWordGetNonBlocking                         0x0000824b
#define func_UARTWordGetNonBlocking                             0x00008265
#define func_UARTCharPut                                        0x0000827d
#define func_UARTHalfWordPut                                    0x00008289
#define func_UARTWordPut                                        0x000082ab
#define func_UARTCharPutNonBlocking                             0x000082cd
#define func_UARTHalfWordPutNonBlocking                         0x000082df
#define func_UARTWordPutNonBlocking                             0x00008307
#define func_UARTStringPut                                      0x0000832f
#define func_UARTPrintf                                         0x00008391
#define func_IntRegister                                        0x000083db
#define func_IntUnregister                                      0x000083ff
#define func_IntPriorityGroupingSet                             0x00008409
#define func_IntPriorityGroupingGet                             0x0000841d
#define func_IntPrioritySet                                     0x0000843f
#define func_IntPriorityGet                                     0x00008461
#define func_IntEnable                                          0x00008479
#define func_IntDisable                                         0x000084d5
#define func_IntPendSet                                         0x00008531
#define func_IntPendClear                                       0x0000857f
#define func_TimerEnable                                        0x000085d1
#define func_TimerDisable                                       0x000085db
#define func_TimerConfigure                                     0x000085e5
#define func_TimerInitCountValueSet                             0x000085fb
#define func_TimerReloadValueSet                                0x000085ff
#define func_TimerPWMValueSet                                   0x00008603
#define func_TimerPolaritySet                                   0x00008607
#define func_TimerPrescaleSet                                   0x0000861b
#define func_TimerPWMDelaySet                                   0x00008627
#define func_TimerCountOffset                                   0x00008633
#define func_TimerCountValueGet                                 0x00008639
#define func_TimerReloadValueGet                                0x0000863d
#define func_TimerCaptureValueGet                               0x00008641
#define func_TimerPolarityGet                                   0x00008645
#define func_TimerIntRegister                                   0x0000864d
#define func_TimerIntUnregister                                 0x0000865d
#define func_TimerIntEnable                                     0x00008671
#define func_TimerIntEventGet                                   0x0000867d
#define func_SysTickEnable                                      0x00008685
#define func_SysTickDisable                                     0x00008693
#define func_SysTickIntRegister                                 0x000086a1
#define func_SysTickIntUnregister                               0x000086c1
#define func_SysTickIntEnable                                   0x000086e1
#define func_SysTickIntDisable                                  0x000086ef
#define func_SysTickPeriodSet                                   0x000086fd
#define func_SysTickPeriodGet                                   0x00008707
#define func_SysTickValueGet                                    0x00008711
#define func_DMAChannelConfigure                                0x00008719
#define func_DMAPeriphReqEn                                     0x0000872f
#define func_DMAPeriphReqDis                                    0x00008749
#define func_DMAChannelTransferStart                            0x00008763
#define func_DMAChannelTransferStop                             0x0000878d
#define func_DMAChannelPeriphReq                                0x000087bf
#define func_DMAIntClear                                        0x000087df
#define func_DMAChannelWaitIdle                                 0x000087f9
#define func_DMAChannelTransferSet                              0x0000881d
#define func_DMAChannelTransfer                                 0x000088f5
#define func_DMAChannelArbitrateEnable                          0x00008919
#define func_DMAChannelArbitrateDisable                         0x0000892b
#define func_DMAChannelNextPointerSet                           0x0000893d
#define func_DMAErrorStatusClear                                0x00008953
#define func_DMAErrorStatusGet                                  0x0000897d
#define func_DMAReqSrcPendingGet                                0x00008997
#define func_DMAChannelTransferRemainCNT                        0x0000899f
#define func_DMACMemset                                         0x000089b5
#define func_DMAIntRegister                                     0x00008a0f
#define func_DMAIntUnregister                                   0x00008a1f
#define func_DMAIntStatus                                       0x00008a33
#define func_DMAIntAllStatus                                    0x00008a49
#define func_SPIConfigSetExpClk                                 0x00008a6d
#define func_SPIChipSelect                                      0x00008a83
#define func_SPIEnable                                          0x00008a91
#define func_SPIDisable                                         0x00008a99
#define func_SPISlaveBurstEnable                                0x00008aa1
#define func_SPISlaveBurstDisable                               0x00008aad
#define func_SPISetTxFifoThreshold                              0x00008ab9
#define func_SPISetRxFifoThreshold                              0x00008abf
#define func_SPITxFifoReset                                     0x00008ac5
#define func_SPITxFifoEnable                                    0x00008ad9
#define func_SPITxFifoDisable                                   0x00008ae5
#define func_SPIRxFifoReset                                     0x00008af1
#define func_SPIRxFifoEnable                                    0x00008b05
#define func_SPIRxFifoDisable                                   0x00008b11
#define func_SPISetDelay                                        0x00008b1d
#define func_SPIManualCSIdle                                    0x00008b2f
#define func_SPIManualCSActive                                  0x00008b3b
#define func_SPIEnManualTransmit                                0x00008b47
#define func_SPIDisManualTransmit                               0x00008b53
#define func_SPIStartManualTransmit                             0x00008b5f
#define func_SPIModeFailEnable                                  0x00008b6b
#define func_SPIModeFailDisable                                 0x00008b77
#define func_SPIIdleCountSet                                    0x00008b83
#define func_SPIModeGet                                         0x00008b8b
#define func_SPIClockPhaseGet                                   0x00008b95
#define func_SPIClockPolarityGet                                0x00008b9f
#define func_SPIClockDivGet                                     0x00008ba9
#define func_SPIDataWidthGet                                    0x00008bb3
#define func_SPIRxFifoStatusGet                                 0x00008bbd
#define func_SPITxFifoStatusGet                                 0x00008bdb
#define func_SPIIsEnable                                        0x00008bf9
#define func_SPIIntRegister                                     0x00008c01
#define func_SPIIntUnregister                                   0x00008c15
#define func_SPIIntEnable                                       0x00008c29
#define func_SPIIntDisable                                      0x00008c33
#define func_SPIIntClear                                        0x00008c3d
#define func_SPIIntStatus                                       0x00008c43
#define func_SPIIntMasked                                       0x00008c49
#define func_SPIBytePut                                         0x00008c4f
#define func_SPINBytePut                                        0x00008c5b
#define func_SPIBytePutNonBlocking                              0x00008c77
#define func_SPIWordPut                                         0x00008c89
#define func_SPINWordPut                                        0x00008c9d
#define func_SPIByteGet                                         0x00008cbf
#define func_SPIByteGetNonBlocking                              0x00008ccf
#define func_SPIWordGet                                         0x00008cd7
#define func_SPIByteGetWithTimeout                              0x00008ced
#define func_SPIByteClearWithTimeout                            0x00008d19
#define func_WatchdogRunning                                    0x00008d4d
#define func_WatchdogEnable                                     0x00008d55
#define func_WatchdogDisable                                    0x00008d5f
#define func_WatchdogResetEnable                                0x00008d69
#define func_WatchdogResetDisable                               0x00008d73
#define func_WatchdogTimerRepeatEnable                          0x00008d7d
#define func_WatchdogTimerRepeatDisable                         0x00008d87
#define func_WatchdogReloadSet                                  0x00008d91
#define func_WatchdogValueGet                                   0x00008d95
#define func_WatchdogIntRegister                                0x00008d99
#define func_WatchdogIntUnregister                              0x00008dad
#define func_WatchdogIntEnable                                  0x00008dc1
#define func_WatchdogIntMaskedGet                               0x00008dcb
#define func_WatchdogReadClearInt                               0x00008dd3
#define func_flash_vendor_delay                                 0x00008dd9
#define func_FLASH_Init                                         0x00008de9
#define func_FLASH_ReadStatusRegIdx                             0x00008e31
#define func_FLASH_WriteEnable                                  0x00008e61
#define func_FLASH_WriteStatusReg                               0x00008e97
#define func_FLASH_WriteStatusRegIdx                            0x00008ebd
#define func_FLASH_StigSendCmd                                  0x00008eef
#define func_FLASH_ChipErase                                    0x00008f03
#define func_FLASH_SectorErase                                  0x00008f1d
#define func_FLASH_BlockErase32K                                0x00008f49
#define func_FLASH_BlockErase64K                                0x00008f75
#define func_FLASH_WriteData                                    0x00008fa1
#define func_FLASH_ReadData                                     0x0000904f
#define func_FLASH_WaitIdle                                     0x000090eb
#define func_FLASH_UpdateData                                   0x0000910f
#define func_FLASH_FAST_WriteData                               0x00009195
#define func_FLASH_WaitIdleNoSus                                0x000091fd
#define func_FLASH_EnableQEFlag                                 0x00009251
#define func_FLASH_EnableQEFlagExt                              0x0000928d
#define func_FLASH_SetReadMode                                  0x000092c5
#define func_FLASH_EnterXIPMode                                 0x000092cd
#define func_FLASH_ExitXIPMode                                  0x00009307
#define func_FLASH_SetWriteMode                                 0x00009339
#define func_FLASH_SecurityErase                                0x00009341
#define func_FLASH_GetUniqueID128                               0x0000936d
#define func_FLASH_GetDeviceID                                  0x00009395
#define func_FLASH_isXIPMode                                    0x000093cd
#define func_I2CInit                                            0x000093e5
#define func_I2CReset                                           0x0000948d
#define func_I2CDeInit                                          0x0000949f
#define func_I2CSetFiFoThreshold                                0x000094b1
#define func_I2CSetAddr                                         0x000094b7
#define func_I2CSetMasterClockStretch                           0x000094bb
#define func_I2CSetDelayBetweenByte                             0x000094cf
#define func_I2CIntEnable                                       0x000094e7
#define func_I2CIntDisable                                      0x000094ef
#define func_I2CIntClear                                        0x000094f7
#define func_I2CIntStatus                                       0x000094ff
#define func_I2CIntRegister                                     0x00009503
#define func_I2CIntUnregister                                   0x0000951f
#define func_I2CStatus                                          0x0000953f
#define func_I2CTxFiFoLevel                                     0x00009543
#define func_I2CRxFiFoLevel                                     0x00009547
#define func_I2CPutData                                         0x0000954b
#define func_I2CGetData                                         0x0000955f
#define func_I2CMasterWriteData                                 0x00009571
#define func_I2CSlaveWriteData                                  0x000095af
#define func_I2CMasterReadData                                  0x000095cf
#define func_I2CSlaveReadData                                   0x00009615
#define func_UTCTimerStop                                       0x0000964d
#define func_UTCTimerRun                                        0x00009659
#define func_UTCCalStop                                         0x00009665
#define func_UTCCalRun                                          0x00009671
#define func_UTCDivStop                                         0x0000967d
#define func_UTCDivEn                                           0x00009689
#define func_UTCHourModeSet                                     0x00009695
#define func_UTCHourModeGet                                     0x0000969b
#define func_UTCTimerSet                                        0x000096a1
#define func_UTCTimerChangeGet                                  0x00009717
#define func_UTCTimerGet                                        0x0000971f
#define func_UTCTimerGetBy10ms                                  0x0000977b
#define func_UTCTimerAlarmSet                                   0x000097e1
#define func_UTCTimerAlarmGet                                   0x0000986d
#define func_UTCTimerAlarmSetBy10ms                             0x000098fb
#define func_UTCCalSet                                          0x000099ab
#define func_UTCCalChangeGet                                    0x00009a25
#define func_UTCCalGet                                          0x00009a35
#define func_UTCCalAlarmSet                                     0x00009a91
#define func_UTCCalAlarmGet                                     0x00009ac5
#define func_UTCAlarmEnable                                     0x00009af1
#define func_UTCAlarmDisable                                    0x00009afb
#define func_UTCIntEnable                                       0x00009b05
#define func_UTCIntDisable                                      0x00009b0f
#define func_UTCIntMaskSet                                      0x00009b19
#define func_UTCIntMaskGet                                      0x00009b23
#define func_UTCIntStatusSet                                    0x00009b29
#define func_UTCIntStatusGet                                    0x00009b33
#define func_UTCValidStatusGet                                  0x00009b39
#define func_UTCKeepRun                                         0x00009b3f
#define func_UTCClkCntSet                                       0x00009b45
#define func_UTCClkCntGet                                       0x00009b57
#define func_UTCClkCntConvert                                   0x00009b77
#define func_UTCIntRegister                                     0x00009b89
#define func_UTCIntUnregister                                   0x00009b9d
#define func_UTC32768Sel                                        0x00009bb1
#define func_UTC32768Get                                        0x00009bb7
#define func_UTCAlarmCntCheckEnable                             0x00009bc1
#define func_UTCAlarmCntCheckDisable                            0x00009bcd
#define func_UTCAlarmCntCFG                                     0x00009bd9
#define func_UTCAlarmCntGet                                     0x00009bfb
#define func_UTCWDTCtrlConfig                                   0x00009c15
#define func_UTCWDTEnable                                       0x00009c1d
#define func_UTCWDTDisable                                      0x00009c2b
#define func_UTCWDTLongTimerDataValid                           0x00009c39
#define func_UTCWDTLongTimerDataInvalid                         0x00009c47
#define func_UTCWDTTickConfig                                   0x00009c55
#define func_UTCWDTLongTimerDataSet                             0x00009c63
#define func_UTCWDTShortTimerDataSet                            0x00009c69
#define func_UTCWDTStartAfterWakeupSet                          0x00009c77
#define func_UTCWDTCalendarDataSet                              0x00009c87
#define func_UTCWDTIntStatusGet                                 0x00009c8d
#define func_UTCWDTClearInt                                     0x00009c93
#define func_GPIOPinSet                                         0x00009ca5
#define func_GPIOPinClear                                       0x00009cc5
#define func_GPIOPinRead                                        0x00009ce5
#define func_GPIOModeSet                                        0x00009d05
#define func_GPIODirectionSet                                   0x00009d3f
#define func_GPIOPeripheralPad                                  0x00009d83
#define func_GPIOInputPeriSelect                                0x00009d8f
#define func_GPIOInputPeriSelectEn                              0x00009d9b
#define func_GPIOInputPeriInvertEn                              0x00009dc1
#define func_GPIOConflictStatusGet                              0x00009de7
#define func_GPIOAllocationStatusGet                            0x00009e07
#define func_GPIOConflictStatusClear                            0x00009e2b
#define func_GPIOConflictClearCheck                             0x00009e43
#define func_GPIOPullupEn                                       0x00009e79
#define func_GPIOPullupDis                                      0x00009e99
#define func_GPIOPulldownEn                                     0x00009eb9
#define func_GPIOPulldownDis                                    0x00009ed9
#define func_GPIOOutputODEn                                     0x00009ef9
#define func_GPIOOutputODDis                                    0x00009f19
#define func_GPIOAnalogEn                                       0x00009f39
#define func_GPIOAnalogDis                                      0x00009f57
#define func_GPIODrvStrengthSet                                 0x00009f75
#define func_GPIOIntRegister                                    0x00009fa1
#define func_GPIOIntUnregister                                  0x00009fb5
#define func_GPIOIntEnable                                      0x00009fc9
#define func_GPIOIntDisable                                     0x00009fe9
#define func_GPIOIntTypeSet                                     0x0000a009
#define func_GPIOIntEdgeSet                                     0x0000a03b
#define func_GPIOIntSingleEdgeSet                               0x0000a06d
#define func_GPIOIntMaskSet                                     0x0000a0a5
#define func_GPIOInt0ReadAndClear                               0x0000a0d9
#define func_GPIOInt1ReadAndClear                               0x0000a0df
#define func_prcm_delay                                         0x0000a0e9
#define func_AON_CP_Load_Flag_Get                               0x0000a0f9
#define func_AON_AP_Load_Flag_Get                               0x0000a11d
#define func_AON_SECONDARY_Load_Flag_Get                        0x0000a141
#define func_AON_CP_Load_Flag_Set                               0x0000a165
#define func_AON_AP_Load_Flag_Set                               0x0000a17f
#define func_AON_SECONDARY_Load_Flag_Set                        0x0000a199
#define func_AON_Flash_Delay_Get                                0x0000a1b3
#define func_AON_Flash_tPWD_Delay_Get                           0x0000a233
#define func_AON_Flash_tRST_Delay_Get                           0x0000a285
#define func_AON_Flash_Delay_Set                                0x0000a2d7
#define func_AON_Flash_tPWD_Delay_Set                           0x0000a2ef
#define func_AON_BOOTMODE_GET                                   0x0000a305
#define func_AON_CP_Memory_Remap_Enable                         0x0000a313
#define func_AON_CP_Memory_Remap_Disable                        0x0000a323
#define func_AON_CP_Core_Release                                0x0000a331
#define func_AON_CP_Core_Hold                                   0x0000a33f
#define func_AON_UP_STATUS_Get                                  0x0000a34d
#define func_AON_System_Clock_Select                            0x0000a363
#define func_Core_PRCM_Clock_Enable0                            0x0000a375
#define func_Core_PRCM_Clock_Disable0                           0x0000a381
#define func_Core_PRCM_Clock_Enable1                            0x0000a38d
#define func_Core_PRCM_Clock_Disable1                           0x0000a399
#define func_Core_PRCM_CHIP_VER_GET                             0x0000a3a5
#define func_CSPRxFrameSet                                      0x0000a3ad
#define func_CSPTxFrameSet                                      0x0000a3d9
#define func_CSPClockSet                                        0x0000a411
#define func_CSPEndianModeSet                                   0x0000a4df
#define func_CSPOperationModeSet                                0x0000a4eb
#define func_CSPUARTModeSet                                     0x0000a4f7
#define func_CSPSPIMode                                         0x0000a5a7
#define func_CSPFIFOLevelSet                                    0x0000a5ed
#define func_CSPCharGet                                         0x0000a5f7
#define func_CSPCharPut                                         0x0000a605
#define func_CSPRxFifoStatusGet                                 0x0000a613
#define func_CSPTxFifoStatusGet                                 0x0000a635
#define func_CSPCharGetNonBlocking                              0x0000a657
#define func_CSPCharPutNonBlocking                              0x0000a66b
#define func_CSPClockModeSet                                    0x0000a67f
#define func_CSPIrdaEnable                                      0x0000a68b
#define func_CSPEnable                                          0x0000a697
#define func_CSPDisable                                         0x0000a6a1
#define func_CSPDrivenEdgeSet                                   0x0000a6ab
#define func_CSPSyncValidLevelSet                               0x0000a6b9
#define func_CSPClockIdleModeSet                                0x0000a6c7
#define func_CSPClockIdleLevelSet                               0x0000a6d3
#define func_CSPPinModeSet                                      0x0000a6df
#define func_CSPIrdaPulseWidthSet                               0x0000a711
#define func_CSPIrdaIdleLevelSet                                0x0000a71d
#define func_CSPHwDetectClockSet                                0x0000a729
#define func_CSPRxEnable                                        0x0000a7a1
#define func_CSPRxDisable                                       0x0000a7ab
#define func_CSPRxStatusGet                                     0x0000a7b5
#define func_CSPTxEnable                                        0x0000a7bd
#define func_CSPTxDisable                                       0x0000a7c7
#define func_CSPIntRegister                                     0x0000a7d1
#define func_CSPIntUnregister                                   0x0000a7e1
#define func_CSPIntEnable                                       0x0000a7f5
#define func_CSPIntDisable                                      0x0000a7fd
#define func_CSPIntStatus                                       0x0000a805
#define func_CSPIntClear                                        0x0000a809
#define func_CSPSPIConfigSetExpClk                              0x0000a80d
#define func_CSPSetTxFifoThreshold                              0x0000a997
#define func_CSPSetRxFifoThreshold                              0x0000a9a5
#define func_CSPFifoReset                                       0x0000a9b3
#define func_CSPFifoStart                                       0x0000a9bf
#define func_CSPDMAConfigRX                                     0x0000a9cb
#define func_CSPDMAConfigTX                                     0x0000a9ff
#define func_CSPUARTRxTimeoutConfig                             0x0000aa33
#define func_SEMAResetAll                                       0x0000aa55
#define func_SEMARequest                                        0x0000aa65
#define func_SEMARequestNonBlocking                             0x0000aa87
#define func_SEMARelease                                        0x0000aa99
#define func_SEMAReqQueueState                                  0x0000aaab
#define func_SEMAMasterGet                                      0x0000aab7
#define func_SEMASlaveGet                                       0x0000aac3
#define func_SEMAIntRegister                                    0x0000aad5
#define func_SEMAIntUnregister                                  0x0000aae9
#define func_SEMAMasterIntEnable                                0x0000aafd
#define func_SEMAMasterIntClear                                 0x0000ab0b
#define func_MCNTStart                                          0x0000ab1d
#define func_MCNTStop                                           0x0000ab25
#define func_MCNTSetCNT32K                                      0x0000ab2d
#define func_MCNTGetCNT32K                                      0x0000ab33
#define func_MCNTGetMCNT                                        0x0000ab39
#define func_MCNTGetIntStatus                                   0x0000ab3f
#define func_MCNTSetClkSrc                                      0x0000ab49
#define func_MCNTIntRegister                                    0x0000ab4f
#define func_MCNTIntUnregister                                  0x0000ab63
#define func_CacheEN                                            0x0000ab7d
#define func_CacheDis                                           0x0000ab87
#define func_CacheBypassEN                                      0x0000ab91
#define func_CacheBypassDis                                     0x0000ab9b
#define func_CacheEnabled                                       0x0000aba5
#define func_CacheStatisticEN                                   0x0000abb3
#define func_CacheStatisticDis                                  0x0000abbd
#define func_CacheWbPolicySet                                   0x0000abc7
#define func_CacheEntryCacheEN                                  0x0000abd3
#define func_CacheEntryCacheDis                                 0x0000abdd
#define func_CacheFlushEn                                       0x0000abe7
#define func_CacheCheckEn                                       0x0000abf1
#define func_CacheWRCleanEn                                     0x0000abfb
#define func_CacheEnableStatusGet                               0x0000ac05
#define func_CacheFlushStatusGet                                0x0000ac0d
#define func_CacheOperationStatusGet                            0x0000ac15
#define func_CacheLineValidGet                                  0x0000ac19
#define func_CacheLineDirtyGet                                  0x0000ac21
#define func_CacheRDHitCntGet                                   0x0000ac27
#define func_CacheRDMissCntGet                                  0x0000ac2b
#define func_CacheRDHitCntSet                                   0x0000ac2f
#define func_CacheRDMissCntSet                                  0x0000ac33
#define func_CacheWaysIdxSet                                    0x0000ac37
#define func_CacheWaysIdxGet                                    0x0000ac47
#define func_CacheSetIdxSet                                     0x0000ac4f
#define func_CacheSetIdxGet                                     0x0000ac59
#define func_CacheElrAddrGet                                    0x0000ac5f
#define func_CacheCRESet                                        0x0000ac63
#define func_CacheCREGet                                        0x0000ac67
#define func_CacheWRHitCntGet                                   0x0000ac6d
#define func_CacheWRMissCntGet                                  0x0000ac71
#define func_CacheParamsGet                                     0x0000ac75
#define func_CacheBAddrSet                                      0x0000ac79
#define func_CacheBAddrGet                                      0x0000ac7d
#define func_CacheTAddrSet                                      0x0000ac81
#define func_CacheTAddrGet                                      0x0000ac85
#define func_CacheIntRegister                                   0x0000ac89
#define func_CacheIntUnregister                                 0x0000ac9d
#define func_CacheIntMask                                       0x0000acb1
#define func_CacheReadAndClearInt                               0x0000acb5
#define func_CacheSetRange                                      0x0000acb9
#define func_CacheEventClear                                    0x0000acbf
#define func_CacheCleanAll                                      0x0000acd3
#define func_CacheCleanInvalidAll                               0x0000acef
#define func_CacheFlush                                         0x0000ad0f
#define func_CacheInit                                          0x0000ad2b
#define func_AESHardwareCtrlClock                               0x0000ad81
#define func_AESClockAlwaysOn                                   0x0000ad8d
#define func_AESKeyLenSet                                       0x0000ad99
#define func_AESKeySet                                          0x0000ada7
#define func_AESIVSet                                           0x0000adaf
#define func_AESAADLen0Set                                      0x0000adb7
#define func_AESAADLen1Set                                      0x0000adbd
#define func_AESPayloadLenSet                                   0x0000adc3
#define func_AESModeSet                                         0x0000adc9
#define func_AESEncDecSet                                       0x0000add7
#define func_SetCipherMode                                      0x0000ade5
#define func_AESDMAEn                                           0x0000adfd
#define func_AESDMADis                                          0x0000ae09
#define func_AESBlockDataInput                                  0x0000ae15
#define func_AESBlockDataOutput                                 0x0000ae1b
#define func_AESBlockStart                                      0x0000ae23
#define func_AESKeyLoadWaitDone                                 0x0000ae2f
#define func_AESBlockTransWaitDone                              0x0000ae41
#define func_S3G_ZUC_TransWaitDone                              0x0000ae4b
#define func_AESClockDiv2En                                     0x0000ae55
#define func_AESClockDiv2Dis                                    0x0000ae61
#define func_ZUCIntegrityEn                                     0x0000ae6d
#define func_ZUCEncrypteEn                                      0x0000ae81
#define func_Snow3GIntegrityEn                                  0x0000ae95
#define func_Snow3GEncrypteEn                                   0x0000aea9
#define func_AESClearUiaAndEia                                  0x0000aebd
#define func_AES_Endian_Clear                                   0x0000aec9
#define func_AES_Endian_In_Small                                0x0000aed5
#define func_AES_Endian_In_Big                                  0x0000aee1
#define func_SZ1_Endian_Out_Small                               0x0000aeed
#define func_SZ1_Endian_Out_Big                                 0x0000aef9
#define func_SZ2_Endian_Out_Small                               0x0000af05
#define func_SZ2_Endian_Out_Big                                 0x0000af11
#define func_AESCCMAuthLenSet                                   0x0000af1d
#define func_AESCCMLengthLenSet                                 0x0000af2d
#define func_AESTagGet                                          0x0000af3d
#define func_AESBlock                                           0x0000af45
#define func_AESECB                                             0x0000b149
#define func_AESCBC                                             0x0000b369
#define func_AESCFB                                             0x0000b5cd
#define func_AESOFB                                             0x0000b835
#define func_AESCTR                                             0x0000ba8f
#define func_AESCCM                                             0x0000bcf9
#define func_AES_128_EEA2                                       0x0000c249
#define func_AES_CMAC_XOR_128                                   0x0000c52d
#define func_AES_CMAC_Leftshift_Onebit                          0x0000c543
#define func_AES_CMAC_Generate_Subkey                           0x0000c55f
#define func_AES_128_EIA2                                       0x0000c5e5
#define func_snow_3g_basic                                      0x0000c8d7
#define func_zuc_basic                                          0x0000cb05
#define func_f_128_EEA1                                         0x0000cd1f
#define func_f_128_EIA1                                         0x0000cd6d
#define func_f_128_EEA3                                         0x0000cf41
#define func_f_128_EIA3                                         0x0000cf97
#define func_sha1_process                                       0x0000d14b
#define func_REG_Bus_Field_Set                                  0x0000d2a1
#define func_REG_Bus_Field_Get                                  0x0000d37f
#define func_CRC_Reset                                          0x0000d431
#define func_CRC_ParameterSet                                   0x0000d43d
#define func_CRC_PolyCoefSet                                    0x0000d44f
#define func_CRC_InitSet                                        0x0000d455
#define func_CRC_WaitForDone                                    0x0000d45b
#define func_CRC_XORSet                                         0x0000d465
#define func_CRC_DataWordInput                                  0x0000d46b
#define func_CRC_DataByteInput                                  0x0000d471
#define func_CRC_ResultGet                                      0x0000d479
#define func_CRC_ProcessStart                                   0x0000d47f
#define func_CRC_ProcessResult                                  0x0000d515
#define func_ABDEnable                                          0x0000d539
#define func_ABDDisable                                         0x0000d545
#define func_ABDEndStatusGet                                    0x0000d551
#define func_ABDCountValueGet                                   0x0000d55b
#define func_HwDetectConfig                                     0x0000d567
#define func_HwDetectEnable                                     0x0000d57d
#define func_HwDetectDisable                                    0x0000d589
#define func_ISO7816_ColdReset                                  0x0000d599
#define func_ISO7816_WarmReset                                  0x0000d5a5
#define func_ISO7816_Deactivation                               0x0000d5b1
#define func_ISO7816_IDLEETUSet                                 0x0000d5bd
#define func_ISO7816_ClockStopEn                                0x0000d5c7
#define func_ISO7816_ClockStopDis                               0x0000d5e5
#define func_ISO7816_TRxRetrySet                                0x0000d5f1
#define func_ISO7816_ClockDiVSet                                0x0000d601
#define func_ISO7816_ETUCycleSet                                0x0000d623
#define func_ISO7816_IdleETUSet                                 0x0000d62f
#define func_ISO7816_GetFifoByteNum                             0x0000d639
#define func_ISO7816_GetFifoFullFlag                            0x0000d63f
#define func_ISO7816_GetFifoEmptyFlag                           0x0000d649
#define func_ISO7816_ByteGet                                    0x0000d651
#define func_ISO7816_BytePut                                    0x0000d657
#define func_ISO7816_SwitchToTxFromRx                           0x0000d65d
#define func_ISO7816_SwitchToRxFromTx                           0x0000d66f
#define func_ISO7816_IntStatGet                                 0x0000d687
#define func_ISO7816_IntStatClr                                 0x0000d68f
#define func_ISO7816_IntEnable                                  0x0000d695
#define func_ISO7816_IntDisable                                 0x0000d69f
#define func_KeyScanStart                                       0x0000d6ad
#define func_KeyScanStop                                        0x0000d6b9
#define func_KeyScanPadEn                                       0x0000d6c5
#define func_KeyScanPadDis                                      0x0000d6d1
#define func_KeyColValidEn                                      0x0000d6dd
#define func_KeyColValidDis                                     0x0000d6e7
#define func_KeyRowAndColNumSet                                 0x0000d6f1
#define func_KeyScanConfig                                      0x0000d717
#define func_KeyScanLongPressEnable                             0x0000d737
#define func_KeyScanLongPressDisable                            0x0000d74f
#define func_KeyScanValidStatus                                 0x0000d75b
#define func_KeyScanValidCountGet                               0x0000d765
#define func_KeyScanValidKeyCoordinate                          0x0000d76d
#define func_KeyCoordinateConvert                               0x0000d773
#define func_KeyScanIntRegister                                 0x0000d799
#define func_KeyScanIntUnregister                               0x0000d7ad
#define func_KeyScanIntEnable                                   0x0000d7c1
#define func_KeyScanIntDisable                                  0x0000d7cd
#define func_LPTimerEnable                                      0x0000d7dd
#define func_LPTimerDisable                                     0x0000d7e7
#define func_LPTimerDualEnable                                  0x0000d7f1
#define func_LPTimerConfigure                                   0x0000d805
#define func_LPTimerInitCountValueSet                           0x0000d819
#define func_LPTimerReloadValueSet                              0x0000d81d
#define func_LPTimerPWMValueSet                                 0x0000d821
#define func_LPTimerPolaritySet                                 0x0000d825
#define func_LPTimerPrescaleSet                                 0x0000d839
#define func_LPTimerPWMDelaySet                                 0x0000d845
#define func_LPTimerCountValueGet                               0x0000d851
#define func_LPTimerReloadValueGet                              0x0000d857
#define func_LPTimerCaptureValueGet                             0x0000d85d
#define func_LPTimerPolarityGet                                 0x0000d863
#define func_LPTimerClockSrcMux                                 0x0000d86b
#define func_LPTimerClockSwitchEnable                           0x0000d877
#define func_LPTimerClockSwitchDisable                          0x0000d881
#define func_LPTimerClockSwitch                                 0x0000d88b
#define func_LPTimerClockPolarity                               0x0000d897
#define func_LPTimerEXTClockFilterDelay                         0x0000d8a3
#define func_LPTimerClockGateFilterDelay                        0x0000d8af
#define func_LPTimerClockStateGet                               0x0000d8bb
#define func_LPTimerExtPhaseDetectEnable                        0x0000d8c5
#define func_LPTimerExtPhaseDetectDisable                       0x0000d8cf
#define func_LPTimerExtPhaseDetectMode                          0x0000d8d9
#define func_LPTimerIntRegister                                 0x0000d8e5
#define func_LPTimerIntUnregister                               0x0000d8f5
#define func_LPTimerIntEnable                                   0x0000d909
#define func_LPTimerIntDisable                                  0x0000d937
#define func_LPTimerIntStatus                                   0x0000d95b
#define func_LPTimerIntEventGet                                 0x0000d965
#define func_TickTimerEnable                                    0x0000d971
#define func_TickTimerDisable                                   0x0000d979
#define func_TickPrescaleSet                                    0x0000d981
#define func_TickAPReloadSet                                    0x0000d987
#define func_TickCPReloadSet                                    0x0000d98d
#define func_TickAPReloadGet                                    0x0000d993
#define func_TickCPReloadGet                                    0x0000d999
#define func_TickCounterSet                                     0x0000d99f
#define func_TickCounterGet                                     0x0000d9ab
#define func_TickAPCompareSet                                   0x0000d9b1
#define func_TickCPCompareSet                                   0x0000d9b7
#define func_TickAPIntEnable                                    0x0000d9bd
#define func_TickCPIntEnable                                    0x0000d9c7
#define func_TickAPIntDisable                                   0x0000d9d1
#define func_TickCPIntDisable                                   0x0000d9db
#define func_TickIntRegister                                    0x0000d9e5
#define func_TickIntUnregister                                  0x0000d9f9
#define func_TickAPReadAndClearInt                              0x0000da0d
#define func_TickCPReadAndClearInt                              0x0000da13
#define func_ADC_SWITCH_TIME_SET                                0x0000da1d
#define func_ADC_CHANNEL_CLK_SET                                0x0000da23
#define func_ADC_CHANNEL_MODE_SET                               0x0000da59
#define func_ADC_AUX_RDY_INV                                    0x0000dacf
#define func_ADC_AUX_RDY_INV_SET                                0x0000dadb
#define func_ADC_AUX_SIGN_SWAP                                  0x0000dae7
#define func_ADC_DMA_UP_THR_SET                                 0x0000daf3
#define func_ADC_INT_ENABLE                                     0x0000daff
#define func_ADC_DMA_MODE_ENABLE                                0x0000db09
#define func_ADC_DMA_MODE_DISABLE                               0x0000db15
#define func_ADC_SCAN_MODE_ENABLE                               0x0000db21
#define func_ADC_SCAN_MODE_DISABLE                              0x0000db2d
#define func_ADC_CHANNEL_ID_SET                                 0x0000db39
#define func_ADC_AUX_CTRL_ENABLE                                0x0000db45
#define func_ADC_AUX_CTRL_DISABLE                               0x0000db51
#define func_ADC_DUTY_ENABLE                                    0x0000db5d
#define func_ADC_DUTY_DISABLE                                   0x0000db69
#define func_ADC_AUX_POWER_ON                                   0x0000db75
#define func_ADC_AUX_POWER_OFF                                  0x0000db81
#define func_ADC_SAMPLE_START                                   0x0000db8d
#define func_ADC_SAMPLE_STOP                                    0x0000db99
#define func_ADC_CLK_DIV_ENABLE                                 0x0000dba5
#define func_ADC_CLK_DIV_DISABLE                                0x0000dbb1
#define func_ADC_DIRECT_DATA_RDY                                0x0000dbbd
#define func_ADC_SCAN_SUSPEND_SET                               0x0000dbc7
#define func_ADC_SCAN_SUSPEND_CLEAR                             0x0000dbd3
#define func_ADC_SCAN_CHANNEL_SET                               0x0000dbdf
#define func_ADC_SCAN_CHANNEL_CLEAR                             0x0000dc33
#define func_ADC_SCAN_CHANNEL_CLEAR_ALL                         0x0000dc41
#define func_ADC_SCAN_TURN_SET                                  0x0000dc55
#define func_ADC_FIFO_FLUSH                                     0x0000dc65
#define func_adcDtatExtend                                      0x0000dc71
#define func_ADC_FIFO_EMPTY                                     0x0000dc7d
#define func_ADC_DATA_READ                                      0x0000dc85
#define func_ADC_DIRECT_DATA_READ                               0x0000dce1
#define func_ADC_SCAN_DATA_READ                                 0x0000dcf3
#define func_ADC_SingleMode_Init                                0x0000dd0f
#define func_ADC_ScanMode_Init                                  0x0000dd3f
#define func_ADC_Start                                          0x0000dd55
#define func_ADC_Stop                                           0x0000dd71
#define func_TRNG_Resetblock                                    0x0000dd8d
#define func_TRNG_EnableHwRngClock                              0x0000dd97
#define func_TRNG_SetSamlpeCountValue                           0x0000dda1
#define func_TRNG_ReadSamlpeCountValue                          0x0000dda9
#define func_TRNG_SetRngRoscLength                              0x0000ddb1
#define func_TRNG_FastModeBypass                                0x0000ddb9
#define func_TRNG_FeModeBypass                                  0x0000ddc3
#define func_TRNG_80090bModeBypass                              0x0000ddcd
#define func_TRNG_EnableRndSource                               0x0000ddd7
#define func_TRNG_DisableRndSource                              0x0000dde1
#define func_TRNG_ReadValidReg                                  0x0000ddeb
#define func_TRNG_ReadValidISRReg                               0x0000ddf3
#define func_TRNG_ReadEHR_Data                                  0x0000ddfb
#define func_TRNG_CleanUpInterruptStatus                        0x0000de05
#define func_TRNG_collectU8Array                                0x0000de11
#define func_TRNG_checkInput                                    0x0000de39
#define func_TRNG_initializingHardware                          0x0000de59
#define func_TRNG_stopHardware                                  0x0000de9f
#define func_TRNG_rndWriteHeader                                0x0000dea7
#define func_TRNG_loadEHR                                       0x0000defb
#define func_TRNG_rndWriteFooter                                0x0000df57
#define func_PhyTimerOffsetTMRL                                 0x0000dfa1
#define func_PhyTimerOffsetTMRM                                 0x0000dfab
#define func_PhyTimerOffsetTMRH                                 0x0000dfb5
#define func_PhyTimerIntStatGet                                 0x0000dfbf
#define func_PhyTimerIntStatClear                               0x0000dfc5
#define func_PhyTimerIntEnable                                  0x0000dfcf
#define func_PhyTimerPOLSet                                     0x0000dfe1
#define func_PhyTimerUtcAlarmCfg                                0x0000dff1
#define func_PhyTimerUtcAlarmRstEn                              0x0000e001
#define func_PhyTimerUtcAlarmRstDis                             0x0000e00d
#define func_PhyTimerUtcAlarmStartEnable                        0x0000e019
#define func_PhyTimerUtcAlarmStartDisable                       0x0000e025
#define func_PhyTimerEnable                                     0x0000e031
#define func_PhyTimerDisable                                    0x0000e03d
#define func_FRC_GetLocalFRC                                    0x0000e049
#define func_FRC_GetSnapshot                                    0x0000e07b
#define func_FRC_AddFRC                                         0x0000e0ab
#define func_FRC_DecFRC                                         0x0000e121
#define func_TRX_CfgCnt_Mode                                    0x0000e1a3
#define func_TRX_CfgCnt_Set                                     0x0000e1ad
#define func_SFW_CfgCnt_Set                                     0x0000e1e1
#define func_LCDC_Enable                                        0x0000e209
#define func_LCDC_Disable                                       0x0000e215
#define func_LCDC_DutySet                                       0x0000e221
#define func_LCDC_BiasSet                                       0x0000e231
#define func_LCDC_MDSet                                         0x0000e241
#define func_LCDC_LwaveSet                                      0x0000e251
#define func_LCDC_LwaveGet                                      0x0000e25f
#define func_LCDC_ForceclkEnable                                0x0000e269
#define func_LCDC_ForceclkDisable                               0x0000e275
#define func_LCDC_BlinkSet                                      0x0000e281
#define func_LCDC_DeadSet                                       0x0000e291
#define func_LCDC_UvolrefSet                                    0x0000e2a1
#define func_LCDC_Uvolnowaitstable                              0x0000e2af
#define func_LCDC_Uvolwaitstable                                0x0000e2bb
#define func_LCDC_FCRpsSet                                      0x0000e2c7
#define func_LCDC_FCRdivSet                                     0x0000e2d5
#define func_LCDC_ComsegmuxSet                                  0x0000e2e5
#define func_LCDC_ComsegmuxGet                                  0x0000e2ed
#define func_rand                                               0x0000e2f9
#define func_srand                                              0x0000e30d
#define func___aeabi_memcpy                                     0x0000e31d
#define func___aeabi_memcpy4                                    0x0000e31d
#define func___aeabi_memcpy8                                    0x0000e31d
#define func___aeabi_memset                                     0x0000e341
#define func___aeabi_memset4                                    0x0000e341
#define func___aeabi_memset8                                    0x0000e341
#define func___aeabi_memclr                                     0x0000e34f
#define func___aeabi_memclr4                                    0x0000e34f
#define func___aeabi_memclr8                                    0x0000e34f
#define func__memset$wrapper                                    0x0000e353
#define func_strlen                                             0x0000e365
#define func_memcmp                                             0x0000e373
#define func_calloc                                             0x0000e38d
#define func___aeabi_uidiv                                      0x0000e3a9
#define func___aeabi_uidivmod                                   0x0000e3a9
#define func___aeabi_uldivmod                                   0x0000e3d5
#define func___I$use$fp                                         0x0000e437
#define func___aeabi_dadd                                       0x0000e437
#define func___aeabi_dsub                                       0x0000e579
#define func___aeabi_drsub                                      0x0000e57f
#define func___aeabi_dmul                                       0x0000e585
#define func___aeabi_ddiv                                       0x0000e669
#define func___aeabi_d2ulz                                      0x0000e747
#define func___aeabi_cdrcmple                                   0x0000e779
#define func___scatterload                                      0x0000e7a9
#define func___scatterload_rt2                                  0x0000e7a9
#define func___aeabi_llsl                                       0x0000e7cd
#define func__ll_shift_l                                        0x0000e7cd
#define func___aeabi_llsr                                       0x0000e7eb
#define func__ll_ushift_r                                       0x0000e7eb
#define func___aeabi_lasr                                       0x0000e80b
#define func__ll_sshift_r                                       0x0000e80b
#define func__double_round                                      0x0000e82f
#define func__double_epilogue                                   0x0000e84d
#define func___0vsprintf                                        0x0000e8e9
#define func___1vsprintf                                        0x0000e8e9
#define func___2vsprintf                                        0x0000e8e9
#define func___c89vsprintf                                      0x0000e8e9
#define func_vsprintf                                           0x0000e8e9
#define func___scatterload_copy                                 0x0000e90d
#define func___scatterload_null                                 0x0000e91b
#define func___scatterload_zeroinit                             0x0000e91d
#define func_free                                               0x0000f1e9
#define func_malloc                                             0x0000f239



#define ROM_ABDEnable                                           ((void (*)(void))func_ABDEnable)
#define ROM_ABDDisable                                          ((void (*)(void))func_ABDDisable)
#define ROM_ABDEndStatusGet                                     ((unsigned char (*)(void))func_ABDEndStatusGet)
#define ROM_ABDCountValueGet                                    ((unsigned long (*)(void))func_ABDCountValueGet)
#define ROM_HwDetectConfig                                      ((void (*)(unsigned char ucCSP, unsigned char ucValidBits,unsigned char ucPattern))func_HwDetectConfig)
#define ROM_HwDetectEnable                                      ((void (*)(void))func_HwDetectEnable)
#define ROM_HwDetectDisable                                     ((void (*)(void))func_HwDetectDisable)
#define ROM_ADC_SCAN_CHANNEL_SET                                ((int (*)(uint8_t scan_index, uint8_t channelID, uint16_t divN, uint8_t ave_mode, uint8_t ave_num, uint8_t scan_num))func_ADC_SCAN_CHANNEL_SET)
#define ROM_ADC_SCAN_CHANNEL_CLEAR_ALL                          ((void (*)(void))func_ADC_SCAN_CHANNEL_CLEAR_ALL)
#define ROM_ADC_DMA_UP_THR_SET                                  ((void (*)(uint8_t up_thr))func_ADC_DMA_UP_THR_SET)
#define ROM_ADC_DMA_MODE_ENABLE                                 ((void (*)(void))func_ADC_DMA_MODE_ENABLE)
#define ROM_ADC_DMA_MODE_DISABLE                                ((void (*)(void))func_ADC_DMA_MODE_DISABLE)
#define ROM_ADC_SCAN_MODE_ENABLE                                ((void (*)(void))func_ADC_SCAN_MODE_ENABLE)
#define ROM_ADC_SCAN_MODE_DISABLE                               ((void (*)(void))func_ADC_SCAN_MODE_DISABLE)
#define ROM_ADC_DUTY_ENABLE                                     ((void (*)(void))func_ADC_DUTY_ENABLE)
#define ROM_ADC_DIRECT_DATA_RDY                                 ((uint8_t (*)(void))func_ADC_DIRECT_DATA_RDY)
#define ROM_ADC_SCAN_SUSPEND_SET                                ((void (*)(void))func_ADC_SCAN_SUSPEND_SET)
#define ROM_ADC_INT_ENABLE                                      ((void (*)(uint8_t adcIntFlag))func_ADC_INT_ENABLE)
#define ROM_ADC_CHANNEL_ID_SET                                  ((void (*)(uint8_t channelID))func_ADC_CHANNEL_ID_SET)
#define ROM_ADC_DIRECT_DATA_READ                                ((int16_t (*)(uint8_t channelID))func_ADC_DIRECT_DATA_READ)
#define ROM_ADC_SCAN_DATA_READ                                  ((int32_t (*)(void))func_ADC_SCAN_DATA_READ)
#define ROM_ADC_DATA_READ                                       ((int16_t (*)(uint8_t channelID))func_ADC_DATA_READ)
#define ROM_ADC_SingleMode_Init                                 ((void (*)(uint8_t channelID, uint16_t divN, uint8_t ave_mode, uint8_t ave_num))func_ADC_SingleMode_Init)
#define ROM_ADC_ScanMode_Init                                   ((void (*)(uint8_t turnCnt))func_ADC_ScanMode_Init)
#define ROM_ADC_Start                                           ((void (*)(void))func_ADC_Start)
#define ROM_ADC_Stop                                            ((void (*)(void))func_ADC_Stop)
#define ROM_CacheEN                                             ((void (*)(unsigned long ulBase))func_CacheEN)
#define ROM_CacheDis                                            ((void (*)(unsigned long ulBase))func_CacheDis)
#define ROM_CacheBypassEN                                       ((void (*)(unsigned long ulBase))func_CacheBypassEN)
#define ROM_CacheBypassDis                                      ((void (*)(unsigned long ulBase))func_CacheBypassDis)
#define ROM_CacheEnabled                                        ((unsigned char (*)(unsigned long ulBase))func_CacheEnabled)
#define ROM_CacheStatisticEN                                    ((void (*)(unsigned long ulBase))func_CacheStatisticEN)
#define ROM_CacheStatisticDis                                   ((void (*)(unsigned long ulBase))func_CacheStatisticDis)
#define ROM_CacheWbPolicySet                                    ((void (*)(unsigned long ulBase, unsigned char ucValue))func_CacheWbPolicySet)
#define ROM_CacheEntryCacheEN                                   ((void (*)(unsigned long ulBase))func_CacheEntryCacheEN)
#define ROM_CacheEntryCacheDis                                  ((void (*)(unsigned long ulBase))func_CacheEntryCacheDis)
#define ROM_CacheFlushEn                                        ((void (*)(unsigned long ulBase))func_CacheFlushEn)
#define ROM_CacheCheckEn                                        ((void (*)(unsigned long ulBase))func_CacheCheckEn)
#define ROM_CacheWRCleanEn                                      ((void (*)(unsigned long ulBase))func_CacheWRCleanEn)
#define ROM_CacheEnableStatusGet                                ((unsigned char (*)(unsigned long ulBase))func_CacheEnableStatusGet)
#define ROM_CacheFlushStatusGet                                 ((unsigned char (*)(unsigned long ulBase))func_CacheFlushStatusGet)
#define ROM_CacheOperationStatusGet                             ((unsigned char (*)(unsigned long ulBase))func_CacheOperationStatusGet)
#define ROM_CacheLineValidGet                                   ((unsigned char (*)(unsigned long ulBase))func_CacheLineValidGet)
#define ROM_CacheLineDirtyGet                                   ((unsigned char (*)(unsigned long ulBase))func_CacheLineDirtyGet)
#define ROM_CacheRDHitCntGet                                    ((unsigned long (*)(unsigned long ulBase))func_CacheRDHitCntGet)
#define ROM_CacheRDMissCntGet                                   ((unsigned long (*)(unsigned long ulBase))func_CacheRDMissCntGet)
#define ROM_CacheRDHitCntSet                                    ((void (*)(unsigned long ulBase, unsigned long value))func_CacheRDHitCntSet)
#define ROM_CacheRDMissCntSet                                   ((void (*)(unsigned long ulBase, unsigned long value))func_CacheRDMissCntSet)
#define ROM_CacheWaysIdxSet                                     ((void (*)(unsigned long ulBase, unsigned char ucValue))func_CacheWaysIdxSet)
#define ROM_CacheWaysIdxGet                                     ((unsigned char (*)(unsigned long ulBase))func_CacheWaysIdxGet)
#define ROM_CacheSetIdxSet                                      ((void (*)(unsigned long ulBase, unsigned long ulValue))func_CacheSetIdxSet)
#define ROM_CacheSetIdxGet                                      ((unsigned long (*)(unsigned long ulBase))func_CacheSetIdxGet)
#define ROM_CacheElrAddrGet                                     ((unsigned long (*)(unsigned long ulBase))func_CacheElrAddrGet)
#define ROM_CacheCRESet                                         ((void (*)(unsigned long ulBase, unsigned long ulValue))func_CacheCRESet)
#define ROM_CacheCREGet                                         ((unsigned long (*)(unsigned long ulBase))func_CacheCREGet)
#define ROM_CacheWRHitCntGet                                    ((unsigned long (*)(unsigned long ulBase))func_CacheWRHitCntGet)
#define ROM_CacheWRMissCntGet                                   ((unsigned long (*)(unsigned long ulBase))func_CacheWRMissCntGet)
#define ROM_CacheParamsGet                                      ((unsigned short (*)(unsigned long ulBase))func_CacheParamsGet)
#define ROM_CacheBAddrSet                                       ((void (*)(unsigned long ulBase, unsigned long ulValue))func_CacheBAddrSet)
#define ROM_CacheBAddrGet                                       ((unsigned long (*)(unsigned long ulBase))func_CacheBAddrGet)
#define ROM_CacheTAddrSet                                       ((void (*)(unsigned long ulBase, unsigned long ulValue))func_CacheTAddrSet)
#define ROM_CacheTAddrGet                                       ((unsigned long (*)(unsigned long ulBase))func_CacheTAddrGet)
#define ROM_CacheIntRegister                                    ((void (*)(unsigned long *g_pRAMVectors, void (*pfnHandler)(void)))func_CacheIntRegister)
#define ROM_CacheIntUnregister                                  ((void (*)(unsigned long *g_pRAMVectors))func_CacheIntUnregister)
#define ROM_CacheIntMask                                        ((void (*)(unsigned long ulBase, unsigned char ucConfig))func_CacheIntMask)
#define ROM_CacheReadAndClearInt                                ((unsigned char (*)(unsigned long ulBase))func_CacheReadAndClearInt)
#define ROM_CacheSetRange                                       ((void (*)(unsigned long ulBase, unsigned long ulBAddr, unsigned long ulTAddr))func_CacheSetRange)
#define ROM_CacheEventClear                                     ((void (*)(unsigned long ulBase))func_CacheEventClear)
#define ROM_CacheCleanAll                                       ((void (*)(unsigned long ulBase))func_CacheCleanAll)
#define ROM_CacheCleanInvalidAll                                ((void (*)(unsigned long ulBase))func_CacheCleanInvalidAll)
#define ROM_CacheFlush                                          ((void (*)(unsigned long ulBase))func_CacheFlush)
#define ROM_CacheInit                                           ((void (*)(unsigned long ulBase, unsigned char ucWbPolicy, unsigned long ulBAddr, unsigned long ulTAddr))func_CacheInit)
#define ROM_REG_Bus_Field_Set                                   ((uint8_t (*)(unsigned long ulRegBase, uint8_t ucBit_high, uint8_t ucBit_low, unsigned long ulValue))func_REG_Bus_Field_Set)
#define ROM_REG_Bus_Field_Get                                   ((uint8_t (*)(unsigned long ulRegBase, uint8_t ucBit_high, uint8_t ucBit_low, unsigned long *ulValue))func_REG_Bus_Field_Get)
#define ROM_CRC_Reset                                           ((void (*)(void))func_CRC_Reset)
#define ROM_CRC_ParameterSet                                    ((void (*)(unsigned char ucPolySize, unsigned char ucInRevMode, unsigned char ucOutRevMode, unsigned char ucXorEn))func_CRC_ParameterSet)
#define ROM_CRC_PolyCoefSet                                     ((void (*)(unsigned long ulPoly))func_CRC_PolyCoefSet)
#define ROM_CRC_InitSet                                         ((void (*)(unsigned long ulPolyInit))func_CRC_InitSet)
#define ROM_CRC_WaitForDone                                     ((void (*)(void))func_CRC_WaitForDone)
#define ROM_CRC_XORSet                                          ((void (*)(unsigned long ulXor))func_CRC_XORSet)
#define ROM_CRC_DataWordInput                                   ((void (*)(unsigned long ulDataIn))func_CRC_DataWordInput)
#define ROM_CRC_DataByteInput                                   ((void (*)(unsigned char ucDataIn))func_CRC_DataByteInput)
#define ROM_CRC_ResultGet                                       ((unsigned long (*)(void))func_CRC_ResultGet)
#define ROM_CRC_ProcessStart                                    ((void (*)(unsigned char *pMessageIn, unsigned long ulMessageInLenBytes, unsigned char ucDmaFlag, unsigned char ucDmaChannel, unsigned char ucMemType))func_CRC_ProcessStart)
#define ROM_CRC_ProcessResult                                   ((unsigned long (*)(unsigned char ucDmaFlag, unsigned char ucDmaChannel))func_CRC_ProcessResult)
#define ROM_AESHardwareCtrlClock                                ((void (*)(void))func_AESHardwareCtrlClock)
#define ROM_AESClockAlwaysOn                                    ((void (*)(void))func_AESClockAlwaysOn)
#define ROM_AESKeyLenSet                                        ((void (*)(unsigned long ulKeyLenMode))func_AESKeyLenSet)
#define ROM_AESKeySet                                           ((void (*)(unsigned char ucOffset, unsigned long ulKeyValue))func_AESKeySet)
#define ROM_AESIVSet                                            ((void (*)(unsigned char ucOffset, unsigned long ucIV))func_AESIVSet)
#define ROM_AESAADLen0Set                                       ((void (*)(unsigned long ulAadLen0))func_AESAADLen0Set)
#define ROM_AESAADLen1Set                                       ((void (*)(unsigned long ulAadLen1))func_AESAADLen1Set)
#define ROM_AESPayloadLenSet                                    ((void (*)(unsigned long ulPayloadLenByte))func_AESPayloadLenSet)
#define ROM_AESModeSet                                          ((void (*)(unsigned long ucAESMode))func_AESModeSet)
#define ROM_AESEncDecSet                                        ((void (*)(unsigned long ucAESEncDec))func_AESEncDecSet)
#define ROM_SetCipherMode                                       ((void (*)(unsigned char ucMode))func_SetCipherMode)
#define ROM_AESDMAEn                                            ((void (*)(void))func_AESDMAEn)
#define ROM_AESDMADis                                           ((void (*)(void))func_AESDMADis)
#define ROM_AESBlockDataInput                                   ((void (*)(unsigned char ucOffset, unsigned long ulDataIn))func_AESBlockDataInput)
#define ROM_AESBlockDataOutput                                  ((unsigned long (*)(unsigned char ucOffset))func_AESBlockDataOutput)
#define ROM_AESBlockStart                                       ((void (*)(void))func_AESBlockStart)
#define ROM_AESKeyLoadWaitDone                                  ((void (*)(void))func_AESKeyLoadWaitDone)
#define ROM_AESBlockTransWaitDone                               ((void (*)(void))func_AESBlockTransWaitDone)
#define ROM_S3G_ZUC_TransWaitDone                               ((void (*)(void))func_S3G_ZUC_TransWaitDone)
#define ROM_AESClockDiv2En                                      ((void (*)(void))func_AESClockDiv2En)
#define ROM_AESClockDiv2Dis                                     ((void (*)(void))func_AESClockDiv2Dis)
#define ROM_ZUCIntegrityEn                                      ((void (*)(void))func_ZUCIntegrityEn)
#define ROM_ZUCEncrypteEn                                       ((void (*)(void))func_ZUCEncrypteEn)
#define ROM_Snow3GIntegrityEn                                   ((void (*)(void))func_Snow3GIntegrityEn)
#define ROM_Snow3GEncrypteEn                                    ((void (*)(void))func_Snow3GEncrypteEn)
#define ROM_AESClearUiaAndEia                                   ((void (*)(void))func_AESClearUiaAndEia)
#define ROM_AES_Endian_Clear                                    ((void (*)(void))func_AES_Endian_Clear)
#define ROM_AES_Endian_In_Small                                 ((void (*)(void))func_AES_Endian_In_Small)
#define ROM_AES_Endian_In_Big                                   ((void (*)(void))func_AES_Endian_In_Big)
#define ROM_SZ1_Endian_Out_Small                                ((void (*)(void))func_SZ1_Endian_Out_Small)
#define ROM_SZ1_Endian_Out_Big                                  ((void (*)(void))func_SZ1_Endian_Out_Big)
#define ROM_SZ2_Endian_Out_Small                                ((void (*)(void))func_SZ2_Endian_Out_Small)
#define ROM_SZ2_Endian_Out_Big                                  ((void (*)(void))func_SZ2_Endian_Out_Big)
#define ROM_AESCCMAuthLenSet                                    ((void (*)(unsigned long ucAuthLenByte))func_AESCCMAuthLenSet)
#define ROM_AESCCMLengthLenSet                                  ((void (*)(unsigned char ucLengthLenByte))func_AESCCMLengthLenSet)
#define ROM_AESTagGet                                           ((unsigned long (*)(unsigned char ucOffset))func_AESTagGet)
#define ROM_AESBlock                                            ((void (*)(unsigned char *pucInput, unsigned char *pucKey,unsigned long ulKeyLen, unsigned char *pucOutput,unsigned char ucOperateMode, unsigned char ucCoreTypeOrIO, unsigned long ulDataInChannel, unsigned long ulDataOutChannel))func_AESBlock)
#define ROM_AESECB                                              ((void (*)(unsigned char *pucInput, unsigned long ulInputLenByte,unsigned char *pucKey, unsigned long ulKeyBitsLen,unsigned char *pucOutput, unsigned long ucOperateMode, unsigned char ucCoreTypeOrIO, unsigned long ulDataInChannel, unsigned long ulDataOutChannel))func_AESECB)
#define ROM_AESCBC                                              ((void (*)(unsigned char *pucInput, unsigned long ulInputLenByte,unsigned char *pucKey, unsigned long ulKeyBitsLen,unsigned char *pucIV, unsigned char *pucOutput,unsigned long ucOperateMode, unsigned char ucCoreTypeOrIO, unsigned long ulDataInChannel, unsigned long ulDataOutChannel))func_AESCBC)
#define ROM_AESCFB                                              ((void (*)(unsigned char *pucInput, unsigned long ulInputLenByte,unsigned char *pucKey, unsigned long ulKeyBitsLen,unsigned char *pucIV, unsigned char *pucOutput,unsigned long ulOperateMode, unsigned char ucCoreTypeOrIO, unsigned long ulDataInChannel, unsigned long ulDataOutChannel))func_AESCFB)
#define ROM_AESOFB                                              ((void (*)(unsigned char *pucInput, unsigned long ulInputLenByte,unsigned char *pucKey, unsigned long ulKeyBitsLen,unsigned char *pucIV, unsigned char *pucOutput,unsigned long ulOperateMode, unsigned char ucCoreTypeOrIO, unsigned long ulDataInChannel, unsigned long ulDataOutChannel))func_AESOFB)
#define ROM_AESCTR                                              ((void (*)(unsigned char *pucInput, unsigned long ulInputLenByte,unsigned char *pucKey, unsigned long ulKeyBitsLen,unsigned char *pucIV, unsigned char *pucOutput,unsigned long ulOperateMode, unsigned char ucCoreTypeOrIO, unsigned long ulDataInChannel, unsigned long ulDataOutChannel))func_AESCTR)
#define ROM_AESCCM                                              ((void (*)(unsigned char *pucAdata, unsigned long ulAdataByteLen,unsigned char *pucPayload, unsigned long ulPayloadByteLen,unsigned long *pulKey, unsigned long ulKeyBitsLen,unsigned char *pucNonce, unsigned char ulNonceByteLen,unsigned char ulTagByteLen, unsigned char *pucCipher,unsigned char *pucDigest, unsigned long ulOperateMode, unsigned char ucCoreTypeOrIO, unsigned long ulDataInChannel, unsigned long ulDataOutChannel))func_AESCCM)
#define ROM_AES_128_EEA2                                        ((void (*)(unsigned char *pucPlain, unsigned char *pucKey, unsigned char *pucCount,unsigned char ucBearer, unsigned char ucDir, unsigned long ulLengthBit,unsigned char *pucCipher, unsigned char ucCoreTypeOrIO, unsigned long ulDataInChannel, unsigned long ulDataOutChannel))func_AES_128_EEA2)
#define ROM_AES_128_EIA2                                        ((void (*)(unsigned char *pucMessage, unsigned char *pucKey, unsigned char *pucCount,unsigned char ucBearer, unsigned char ucDir, unsigned long ulLengthBit,unsigned char *ucM, unsigned char *pucMACT, unsigned char ucCoreTypeOrIO, unsigned long ulDataInChannel, unsigned long ulDataOutChannel))func_AES_128_EIA2)
#define ROM_AES_CMAC_XOR_128                                    ((void (*)(unsigned char *pucA, const unsigned char *pucB, unsigned char *pucOut))func_AES_CMAC_XOR_128)
#define ROM_AES_CMAC_Leftshift_Onebit                           ((void (*)(unsigned char *pucInput, unsigned char *pucOutput))func_AES_CMAC_Leftshift_Onebit)
#define ROM_AES_CMAC_Generate_Subkey                            ((void (*)(unsigned char *pucKey, unsigned char *pucK1, unsigned char *pucK2))func_AES_CMAC_Generate_Subkey)
#define ROM_snow_3g_basic                                       ((void (*)(unsigned char *pucInput, unsigned char *pucKey, unsigned char *pucIV,unsigned char *pucOutput, unsigned long ulLengthByte,unsigned char ucEndianOut, unsigned char ucCoreTypeOrIO, unsigned long ulDataInChannel, unsigned long ulDataOutChannel))func_snow_3g_basic)
#define ROM_zuc_basic                                           ((void (*)(unsigned char *pucInput, unsigned char *pucKey,unsigned char *pucIV, unsigned char *pucOutput,unsigned long ulLengthByte, unsigned char ucEndianOut, unsigned char ucCoreTypeOrIO, unsigned long ulDataInChannel, unsigned long ulDataOutChannel))func_zuc_basic)
#define ROM_f_128_EEA1                                          ((void (*)(unsigned char *pucInput, unsigned char *pucKey,unsigned char *pucCount, unsigned char ucBearer,unsigned char ucDir, unsigned long ulLengthBit, unsigned char ucCoreTypeOrIO, unsigned long ulDataInChannel, unsigned long ulDataOutChannel))func_f_128_EEA1)
#define ROM_f_128_EIA1                                          ((void (*)(unsigned char *pucMessage, unsigned char *pucKey,unsigned char *pucCountI, unsigned char ucBearer,unsigned char ucDir, unsigned long ulLengthBit, unsigned char *pucMacT, unsigned char ucCoreTypeOrIO, unsigned long ulChannelNum))func_f_128_EIA1)
#define ROM_f_128_EEA3                                          ((void (*)(unsigned char *pucKey, unsigned char *pucCount,unsigned char ucBearer, unsigned char ucDir, unsigned long ulLengthBit,unsigned char *pucPlain, unsigned char *pucCipher, unsigned char ucCoreTypeOrIO, unsigned long ulDataInChannel, unsigned long ulDataOutChannel))func_f_128_EEA3)
#define ROM_f_128_EIA3                                          ((void (*)(unsigned char *pucKey, unsigned char *pucCount,unsigned char ucBearer, unsigned char ucDir, unsigned long ulLengthBit,unsigned char *pucMessage, unsigned char *pucMacI, unsigned char ucCoreTypeOrIO, unsigned long ulChannelNum))func_f_128_EIA3)
#define ROM_sha1_process                                        ((void (*)(unsigned char *pucData, unsigned long lenBits,unsigned char *pucResult, unsigned char dmaFlag, unsigned char ucMemType, unsigned long ulChannelNum))func_sha1_process)
#define ROM_CSPUARTModeSet                                      ((void (*)(unsigned long ulBase, unsigned long ulPclk, unsigned long ulBaudrate, unsigned char ucDatabits, unsigned long ulParityCheck, unsigned char ucStopbits))func_CSPUARTModeSet)
#define ROM_CSPSPIMode                                          ((void (*)(unsigned long ulBase, unsigned long ulMode1, unsigned long ulMode2, unsigned long ulTxFrameCtl, unsigned long ulRxFrameCtl))func_CSPSPIMode)
#define ROM_CSPFIFOLevelSet                                     ((void (*)(unsigned long ulBase, unsigned long ulTxLevel, unsigned long ulRxLevel))func_CSPFIFOLevelSet)
#define ROM_CSPCharGet                                          ((unsigned int (*)(unsigned long ulBase))func_CSPCharGet)
#define ROM_CSPCharPut                                          ((void (*)(unsigned long ulBase, unsigned int ucData))func_CSPCharPut)
#define ROM_CSPRxFifoStatusGet                                  ((unsigned char (*)(unsigned long ulBase, unsigned char ucFlagType))func_CSPRxFifoStatusGet)
#define ROM_CSPTxFifoStatusGet                                  ((unsigned char (*)(unsigned long ulBase, unsigned char ucFlagType))func_CSPTxFifoStatusGet)
#define ROM_CSPCharGetNonBlocking                               ((long (*)(unsigned long ulBase))func_CSPCharGetNonBlocking)
#define ROM_CSPCharPutNonBlocking                               ((long (*)(unsigned long ulBase, unsigned char ucData))func_CSPCharPutNonBlocking)
#define ROM_CSPOperationModeSet                                 ((void (*)(unsigned long ulBase, unsigned long ulOperationMode))func_CSPOperationModeSet)
#define ROM_CSPClockModeSet                                     ((void (*)(unsigned long ulBase, unsigned long ulClockMode))func_CSPClockModeSet)
#define ROM_CSPHwDetectClockSet                                 ((void (*)(unsigned long ulBase, unsigned long ulPclk, unsigned ulClock))func_CSPHwDetectClockSet)
#define ROM_CSPIrdaEnable                                       ((void (*)(unsigned long ulBase, unsigned long ulIrdaEn))func_CSPIrdaEnable)
#define ROM_CSPEndianModeSet                                    ((void (*)(unsigned long ulBase, unsigned long ulEndianMode))func_CSPEndianModeSet)
#define ROM_CSPEnable                                           ((void (*)(unsigned long ulBase))func_CSPEnable)
#define ROM_CSPDisable                                          ((void (*)(unsigned long ulBase))func_CSPDisable)
#define ROM_CSPDrivenEdgeSet                                    ((void (*)(unsigned long ulBase, unsigned long ulRxEdge, unsigned long ulTxEdge))func_CSPDrivenEdgeSet)
#define ROM_CSPSyncValidLevelSet                                ((void (*)(unsigned long ulBase, unsigned long ulRFS, unsigned long ulTFS))func_CSPSyncValidLevelSet)
#define ROM_CSPClockIdleModeSet                                 ((void (*)(unsigned long ulBase, unsigned long ulClkIdleMode))func_CSPClockIdleModeSet)
#define ROM_CSPClockIdleLevelSet                                ((void (*)(unsigned long ulBase, unsigned long ulClkIdleLevel))func_CSPClockIdleLevelSet)
#define ROM_CSPPinModeSet                                       ((void (*)(unsigned long ulBase, unsigned long ulCSPPin, unsigned long ulPinMode, unsigned long ulPinDirection))func_CSPPinModeSet)
#define ROM_CSPIrdaPulseWidthSet                                ((void (*)(unsigned long ulBase, unsigned long ulWidth))func_CSPIrdaPulseWidthSet)
#define ROM_CSPIrdaIdleLevelSet                                 ((void (*)(unsigned long ulBase, unsigned long ulIrdaIdleLevel))func_CSPIrdaIdleLevelSet)
#define ROM_CSPClockSet                                         ((void (*)(unsigned long ulBase, unsigned long ulCSPMode, unsigned long ulPclk, unsigned ulClock))func_CSPClockSet)
#define ROM_CSPTxFrameSet                                       ((void (*)(unsigned long ulBase, unsigned long ulDelay, unsigned long ulDataLen, unsigned long ulSyncLen, unsigned long ulFrameLen))func_CSPTxFrameSet)
#define ROM_CSPRxFrameSet                                       ((void (*)(unsigned long ulBase, unsigned long ulDelay, unsigned long ulDataLen, unsigned long ulFrameLen))func_CSPRxFrameSet)
#define ROM_CSPRxEnable                                         ((void (*)(unsigned long ulBase))func_CSPRxEnable)
#define ROM_CSPRxDisable                                        ((void (*)(unsigned long ulBase))func_CSPRxDisable)
#define ROM_CSPRxStatusGet                                      ((unsigned char (*)(unsigned long ulBase))func_CSPRxStatusGet)
#define ROM_CSPTxEnable                                         ((void (*)(unsigned long ulBase))func_CSPTxEnable)
#define ROM_CSPTxDisable                                        ((void (*)(unsigned long ulBase))func_CSPTxDisable)
#define ROM_CSPIntRegister                                      ((void (*)(unsigned long ulBase, unsigned long *g_pRAMVectors, void(*pfnHandler)(void)))func_CSPIntRegister)
#define ROM_CSPIntUnregister                                    ((void (*)(unsigned long ulBase, unsigned long *g_pRAMVectors))func_CSPIntUnregister)
#define ROM_CSPIntEnable                                        ((void (*)(unsigned long ulBase, unsigned long ulIntFlags))func_CSPIntEnable)
#define ROM_CSPIntDisable                                       ((void (*)(unsigned long ulBase, unsigned long ulIntFlags))func_CSPIntDisable)
#define ROM_CSPIntStatus                                        ((unsigned long (*)(unsigned long ulBase))func_CSPIntStatus)
#define ROM_CSPIntClear                                         ((void (*)(unsigned long ulBase, unsigned long ulIntFlags))func_CSPIntClear)
#define ROM_CSPDMAConfigRX                                      ((void (*)(unsigned long ulBase, unsigned char ucLevelCheck, unsigned long ulDataLen))func_CSPDMAConfigRX)
#define ROM_CSPDMAConfigTX                                      ((void (*)(unsigned long ulBase, unsigned char ucLevelCheck, unsigned long ulDataLen))func_CSPDMAConfigTX)
#define ROM_CSPUARTRxTimeoutConfig                              ((void (*)(unsigned long ulBase, unsigned char relevantFifoEmpty, unsigned short timeout_num))func_CSPUARTRxTimeoutConfig)
#define ROM_CSPSPIConfigSetExpClk                               ((void (*)(unsigned long ulBase, unsigned char ulMode, unsigned long ulPclk, unsigned ulClock, unsigned char CPOL, unsigned char CPHA))func_CSPSPIConfigSetExpClk)
#define ROM_CSPSetRxFifoThreshold                               ((void (*)(unsigned long ulBase, unsigned long ulThreshold))func_CSPSetRxFifoThreshold)
#define ROM_CSPSetTxFifoThreshold                               ((void (*)(unsigned long ulBase, unsigned long ulThreshold))func_CSPSetTxFifoThreshold)
#define ROM_CSPFifoReset                                        ((void (*)(unsigned long ulBase))func_CSPFifoReset)
#define ROM_CSPFifoStart                                        ((void (*)(unsigned long ulBase))func_CSPFifoStart)
#define ROM_DMAChannelConfigure                                 ((void (*)(unsigned long ulChannelNum, unsigned long ulConfig))func_DMAChannelConfigure)
#define ROM_DMAPeriphReqEn                                      ((void (*)(unsigned long ulChannelNum))func_DMAPeriphReqEn)
#define ROM_DMAPeriphReqDis                                     ((void (*)(unsigned long ulChannelNum))func_DMAPeriphReqDis)
#define ROM_DMAChannelTransferStart                             ((void (*)(unsigned long ulChannelNum))func_DMAChannelTransferStart)
#define ROM_DMAChannelTransferStop                              ((void (*)(unsigned long ulChannelNum))func_DMAChannelTransferStop)
#define ROM_DMAChannelPeriphReq                                 ((void (*)(unsigned long ulChannelNum, unsigned char ucPeriSrc))func_DMAChannelPeriphReq)
#define ROM_DMAChannelTransfer                                  ((void (*)(unsigned long ulChannelNum, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned char ucMemType))func_DMAChannelTransfer)
#define ROM_DMAChannelTransferSet                               ((void (*)(unsigned long ulChannelNum, void *pvSrcAddr, void *pvDstAddr, unsigned long ulTransferSize, unsigned char ucMemType))func_DMAChannelTransferSet)
#define ROM_DMAChannelArbitrateEnable                           ((void (*)(unsigned char group))func_DMAChannelArbitrateEnable)
#define ROM_DMAChannelArbitrateDisable                          ((void (*)(unsigned char group))func_DMAChannelArbitrateDisable)
#define ROM_DMAChannelNextPointerSet                            ((void (*)(unsigned long ulChannelNum, void *pvNextPointer))func_DMAChannelNextPointerSet)
#define ROM_DMAErrorStatusClear                                 ((void (*)(unsigned long ulChannelNum))func_DMAErrorStatusClear)
#define ROM_DMAErrorStatusGet                                   ((unsigned char (*)(unsigned long ulChannelNum))func_DMAErrorStatusGet)
#define ROM_DMAReqSrcPendingGet                                 ((unsigned long (*)(void))func_DMAReqSrcPendingGet)
#define ROM_DMAChannelTransferRemainCNT                         ((unsigned long (*)(unsigned long ulChannelNum))func_DMAChannelTransferRemainCNT)
#define ROM_DMACMemset                                          ((void (*)(unsigned long ulChannelNum, void *pvDstAddr, unsigned char ucValue,unsigned long ulSize, unsigned char ucMemType))func_DMACMemset)
#define ROM_DMAIntRegister                                      ((void (*)(unsigned long ulIntChannel, unsigned long *g_pRAMVectors, void (*pfnHandler)(void)))func_DMAIntRegister)
#define ROM_DMAIntUnregister                                    ((void (*)(unsigned long ulIntChannel, unsigned long *g_pRAMVectors))func_DMAIntUnregister)
#define ROM_DMAIntClear                                         ((void (*)(unsigned long ulChannelNum))func_DMAIntClear)
#define ROM_DMAChannelWaitIdle                                  ((void (*)(unsigned long ulChannelNum))func_DMAChannelWaitIdle)
#define ROM_DMAIntStatus                                        ((unsigned char (*)(unsigned long ulChannelNum))func_DMAIntStatus)
#define ROM_DMAIntAllStatus                                     ((unsigned char (*)(void))func_DMAIntAllStatus)
#define ROM_FLASH_Init                                          ((void (*)(QSPI_FLASH_Def *flash_vendor, unsigned int ref_clk_hz, unsigned int sclk_hz))func_FLASH_Init)
#define ROM_FLASH_ReadStatusRegIdx                              ((unsigned char (*)(unsigned char ucStatusRegIdx))func_FLASH_ReadStatusRegIdx)
#define ROM_FLASH_WriteEnable                                   ((void (*)(void))func_FLASH_WriteEnable)
#define ROM_FLASH_WriteStatusReg                                ((void (*)(unsigned char ucStatusRegVal0, unsigned char ucStatusRegVal1))func_FLASH_WriteStatusReg)
#define ROM_FLASH_WriteStatusRegIdx                             ((void (*)(unsigned char ucStatusRegIdx, unsigned char ucStatusRegVal))func_FLASH_WriteStatusRegIdx)
#define ROM_FLASH_StigSendCmd                                   ((void (*)(unsigned char ucCmd))func_FLASH_StigSendCmd)
#define ROM_FLASH_ChipErase                                     ((void (*)(void))func_FLASH_ChipErase)
#define ROM_FLASH_SectorErase                                   ((void (*)(unsigned int address))func_FLASH_SectorErase)
#define ROM_FLASH_BlockErase32K                                 ((void (*)(unsigned int address))func_FLASH_BlockErase32K)
#define ROM_FLASH_BlockErase64K                                 ((void (*)(unsigned int address))func_FLASH_BlockErase64K)
#define ROM_FLASH_WriteData                                     ((void (*)(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum, unsigned char ucMemType))func_FLASH_WriteData)
#define ROM_FLASH_ReadData                                      ((void (*)(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum, unsigned char ucMemType))func_FLASH_ReadData)
#define ROM_FLASH_UpdateData                                    ((void (*)(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum, unsigned char ucMemType))func_FLASH_UpdateData)
#define ROM_FLASH_FAST_WriteData                                ((void (*)(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum, unsigned char ucMemType))func_FLASH_FAST_WriteData)
#define ROM_FLASH_WaitIdle                                      ((void (*)(void))func_FLASH_WaitIdle)
#define ROM_FLASH_WaitIdleNoSus                                 ((void (*)(void))func_FLASH_WaitIdleNoSus)
#define ROM_FLASH_EnableQEFlag                                  ((void (*)(QSPI_FLASH_Def *flash_vendor))func_FLASH_EnableQEFlag)
#define ROM_FLASH_EnableQEFlagExt                               ((void (*)(unsigned long flashMode))func_FLASH_EnableQEFlagExt)
#define ROM_FLASH_EnterXIPMode                                  ((void (*)(QSPI_FLASH_Def *flash_vendor, char xip_dummy))func_FLASH_EnterXIPMode)
#define ROM_FLASH_ExitXIPMode                                   ((void (*)(QSPI_FLASH_Def *flash_vendor))func_FLASH_ExitXIPMode)
#define ROM_FLASH_SetReadMode                                   ((void (*)(unsigned char devId, unsigned int mode))func_FLASH_SetReadMode)
#define ROM_FLASH_SetWriteMode                                  ((void (*)(unsigned char devId, unsigned int mode))func_FLASH_SetWriteMode)
#define ROM_FLASH_SecurityErase                                 ((void (*)(unsigned long ulEraseAddr))func_FLASH_SecurityErase)
#define ROM_FLASH_GetUniqueID128                                ((void (*)(QSPI_FLASH_Def *flash_vendor, unsigned char *pucUniqueId, unsigned char ucMemType))func_FLASH_GetUniqueID128)
#define ROM_FLASH_GetDeviceID                                   ((unsigned long (*)(void))func_FLASH_GetDeviceID)
#define ROM_FLASH_isXIPMode                                     ((unsigned char (*)(void))func_FLASH_isXIPMode)
#define ROM_GPIOPinSet                                          ((void (*)(unsigned char ucPadNum))func_GPIOPinSet)
#define ROM_GPIOPinClear                                        ((void (*)(unsigned char ucPadNum))func_GPIOPinClear)
#define ROM_GPIOPinRead                                         ((unsigned char (*)(unsigned char ucPadNum))func_GPIOPinRead)
#define ROM_GPIOModeSet                                         ((void (*)(unsigned char ucPadNum, unsigned char ucMode, unsigned char ucConfig))func_GPIOModeSet)
#define ROM_GPIODirectionSet                                    ((void (*)(unsigned char ucPins, unsigned long ulPinIO))func_GPIODirectionSet)
#define ROM_GPIOPeripheralPad                                   ((void (*)(unsigned char ucPeriNum, unsigned char ucPadNum))func_GPIOPeripheralPad)
#define ROM_GPIOInputPeriSelect                                 ((void (*)(unsigned char ucPeriNum, unsigned char ucPadNum))func_GPIOInputPeriSelect)
#define ROM_GPIOInputPeriSelectEn                               ((void (*)(unsigned char ucPeriNum, unsigned char ucEnable))func_GPIOInputPeriSelectEn)
#define ROM_GPIOInputPeriInvertEn                               ((void (*)(unsigned char ucPeriNum, unsigned char ucInvert))func_GPIOInputPeriInvertEn)
#define ROM_GPIOConflictStatusGet                               ((unsigned char (*)(unsigned char ucPins))func_GPIOConflictStatusGet)
#define ROM_GPIOAllocationStatusGet                             ((unsigned char (*)(unsigned char ucPins))func_GPIOAllocationStatusGet)
#define ROM_GPIOConflictClearCheck                              ((void (*)(unsigned char ucPins, unsigned char ucPeriNum))func_GPIOConflictClearCheck)
#define ROM_GPIOConflictStatusClear                             ((void (*)(unsigned char ucPins))func_GPIOConflictStatusClear)
#define ROM_GPIOPullupEn                                        ((void (*)(unsigned char ucPins))func_GPIOPullupEn)
#define ROM_GPIOPullupDis                                       ((void (*)(unsigned char ucPins))func_GPIOPullupDis)
#define ROM_GPIOPulldownEn                                      ((void (*)(unsigned char ucPins))func_GPIOPulldownEn)
#define ROM_GPIOPulldownDis                                     ((void (*)(unsigned char ucPins))func_GPIOPulldownDis)
#define ROM_GPIOOutputODEn                                      ((void (*)(unsigned char ucPins))func_GPIOOutputODEn)
#define ROM_GPIOOutputODDis                                     ((void (*)(unsigned char ucPins))func_GPIOOutputODDis)
#define ROM_GPIOAnalogEn                                        ((void (*)(unsigned char ucPins))func_GPIOAnalogEn)
#define ROM_GPIOAnalogDis                                       ((void (*)(unsigned char ucPins))func_GPIOAnalogDis)
#define ROM_GPIODrvStrengthSet                                  ((void (*)(unsigned char ucPins, unsigned char ucDrvStrength))func_GPIODrvStrengthSet)
#define ROM_GPIOIntRegister                                     ((void (*)(unsigned long *g_pRAMVectors, void (*pfnHandler)(void)))func_GPIOIntRegister)
#define ROM_GPIOIntUnregister                                   ((void (*)(unsigned long *g_pRAMVectors))func_GPIOIntUnregister)
#define ROM_GPIOIntEnable                                       ((void (*)(unsigned char ucPins))func_GPIOIntEnable)
#define ROM_GPIOIntDisable                                      ((void (*)(unsigned char ucPins))func_GPIOIntDisable)
#define ROM_GPIOIntTypeSet                                      ((void (*)(unsigned char ucPins, unsigned char ucConfig))func_GPIOIntTypeSet)
#define ROM_GPIOIntEdgeSet                                      ((void (*)(unsigned char ucPins, unsigned char ucConfig))func_GPIOIntEdgeSet)
#define ROM_GPIOIntSingleEdgeSet                                ((void (*)(unsigned char ucPins, unsigned char ucConfig))func_GPIOIntSingleEdgeSet)
#define ROM_GPIOIntMaskSet                                      ((void (*)(unsigned char ucPins,unsigned char ucEnable))func_GPIOIntMaskSet)
#define ROM_GPIOInt0ReadAndClear                                ((unsigned long (*)(void))func_GPIOInt0ReadAndClear)
#define ROM_GPIOInt1ReadAndClear                                ((unsigned long (*)(void))func_GPIOInt1ReadAndClear)
#define ROM_I2CInit                                             ((void (*)(unsigned long ulBase,I2C_workmode workmode,I2C_clockmode clockmode,I2C_addrbits addrbits,unsigned long pclk))func_I2CInit)
#define ROM_I2CReset                                            ((void (*)(unsigned long ulBase))func_I2CReset)
#define ROM_I2CDeInit                                           ((void (*)(unsigned long ulBase))func_I2CDeInit)
#define ROM_I2CSetFiFoThreshold                                 ((void (*)(unsigned long ulBase, unsigned char TxFiFoThreshold, unsigned char RxFiFoThreshold))func_I2CSetFiFoThreshold)
#define ROM_I2CSetAddr                                          ((void (*)(unsigned long ulBase, unsigned short addr))func_I2CSetAddr)
#define ROM_I2CSetMasterClockStretch                            ((void (*)(unsigned long ulBase, unsigned char enable))func_I2CSetMasterClockStretch)
#define ROM_I2CSetDelayBetweenByte                              ((void (*)(unsigned long ulBase, unsigned char enable, unsigned char delayBetweenByte))func_I2CSetDelayBetweenByte)
#define ROM_I2CIntEnable                                        ((void (*)(unsigned long ulBase, unsigned long ulIntFlags))func_I2CIntEnable)
#define ROM_I2CIntDisable                                       ((void (*)(unsigned long ulBase, unsigned long ulIntFlags))func_I2CIntDisable)
#define ROM_I2CIntClear                                         ((void (*)(unsigned long ulBase, unsigned long ulIntFlags))func_I2CIntClear)
#define ROM_I2CIntStatus                                        ((unsigned long (*)(unsigned long ulBase))func_I2CIntStatus)
#define ROM_I2CIntRegister                                      ((void (*)(unsigned long ulBase, unsigned long *g_pRAMVectors, void (*pfnHandler)(void)))func_I2CIntRegister)
#define ROM_I2CIntUnregister                                    ((void (*)(unsigned long ulBase, unsigned long *g_pRAMVectors))func_I2CIntUnregister)
#define ROM_I2CStatus                                           ((unsigned short (*)(unsigned long ulBase))func_I2CStatus)
#define ROM_I2CTxFiFoLevel                                      ((unsigned char (*)(unsigned long ulBase))func_I2CTxFiFoLevel)
#define ROM_I2CRxFiFoLevel                                      ((unsigned char (*)(unsigned long ulBase))func_I2CRxFiFoLevel)
#define ROM_I2CPutData                                          ((void (*)(unsigned long ulBase, unsigned short data))func_I2CPutData)
#define ROM_I2CGetData                                          ((unsigned char (*)(unsigned long ulBase))func_I2CGetData)
#define ROM_I2CMasterWriteData                                  ((unsigned long (*)(unsigned long ulBase, unsigned char *pdata, unsigned long len))func_I2CMasterWriteData)
#define ROM_I2CSlaveWriteData                                   ((unsigned long (*)(unsigned long ulBase, unsigned char *pdata, unsigned long len))func_I2CSlaveWriteData)
#define ROM_I2CMasterReadData                                   ((unsigned long (*)(unsigned long ulBase, unsigned char *pdata, unsigned long len))func_I2CMasterReadData)
#define ROM_I2CSlaveReadData                                    ((unsigned long (*)(unsigned long ulBase, unsigned char *pdata, unsigned long len))func_I2CSlaveReadData)
#define ROM_IntRegister                                         ((void (*)(unsigned long ulInterrupt, unsigned long *g_pRAMVectors, void (*pfnHandler)(void)))func_IntRegister)
#define ROM_IntUnregister                                       ((void (*)(unsigned long ulInterrupt, unsigned long *g_pRAMVectors))func_IntUnregister)
#define ROM_IntPriorityGroupingSet                              ((void (*)(unsigned long ulBits))func_IntPriorityGroupingSet)
#define ROM_IntPriorityGroupingGet                              ((unsigned long (*)(void))func_IntPriorityGroupingGet)
#define ROM_IntPrioritySet                                      ((void (*)(unsigned long ulInterrupt,unsigned char ucPriority))func_IntPrioritySet)
#define ROM_IntPriorityGet                                      ((long (*)(unsigned long ulInterrupt))func_IntPriorityGet)
#define ROM_IntEnable                                           ((void (*)(unsigned long ulInterrupt))func_IntEnable)
#define ROM_IntDisable                                          ((void (*)(unsigned long ulInterrupt))func_IntDisable)
#define ROM_IntPendSet                                          ((void (*)(unsigned long ulInterrupt))func_IntPendSet)
#define ROM_IntPendClear                                        ((void (*)(unsigned long ulInterrupt))func_IntPendClear)
#define ROM_ISO7816_ColdReset                                   ((void (*)(void))func_ISO7816_ColdReset)
#define ROM_ISO7816_WarmReset                                   ((void (*)(void))func_ISO7816_WarmReset)
#define ROM_ISO7816_Deactivation                                ((void (*)(void))func_ISO7816_Deactivation)
#define ROM_ISO7816_ClockStopEn                                 ((void (*)(unsigned char ucClkLevel))func_ISO7816_ClockStopEn)
#define ROM_ISO7816_ClockStopDis                                ((void (*)(void))func_ISO7816_ClockStopDis)
#define ROM_ISO7816_TRxRetrySet                                 ((void (*)(unsigned char ucRetryNum))func_ISO7816_TRxRetrySet)
#define ROM_ISO7816_ClockDiVSet                                 ((void (*)(unsigned long ulRefClock, unsigned long ulExpectClock))func_ISO7816_ClockDiVSet)
#define ROM_ISO7816_ETUCycleSet                                 ((void (*)(unsigned short int usiCyclesPerETU))func_ISO7816_ETUCycleSet)
#define ROM_ISO7816_IdleETUSet                                  ((void (*)(unsigned short int usiIdleCnt))func_ISO7816_IdleETUSet)
#define ROM_ISO7816_GetFifoByteNum                              ((unsigned char (*)(void))func_ISO7816_GetFifoByteNum)
#define ROM_ISO7816_GetFifoFullFlag                             ((unsigned char (*)(void))func_ISO7816_GetFifoFullFlag)
#define ROM_ISO7816_GetFifoEmptyFlag                            ((unsigned char (*)(void))func_ISO7816_GetFifoEmptyFlag)
#define ROM_ISO7816_ByteGet                                     ((unsigned char (*)(void))func_ISO7816_ByteGet)
#define ROM_ISO7816_BytePut                                     ((void (*)(unsigned char ucByte))func_ISO7816_BytePut)
#define ROM_ISO7816_SwitchToTxFromRx                            ((void (*)(void))func_ISO7816_SwitchToTxFromRx)
#define ROM_ISO7816_SwitchToRxFromTx                            ((void (*)(void))func_ISO7816_SwitchToRxFromTx)
#define ROM_ISO7816_IntStatGet                                  ((unsigned char (*)(unsigned long ulIntFlags))func_ISO7816_IntStatGet)
#define ROM_ISO7816_IntStatClr                                  ((void (*)(unsigned long ulIntFlags))func_ISO7816_IntStatClr)
#define ROM_ISO7816_IntEnable                                   ((void (*)(unsigned long ulIntFlags))func_ISO7816_IntEnable)
#define ROM_ISO7816_IntDisable                                  ((void (*)(unsigned long ulIntFlags))func_ISO7816_IntDisable)
#define ROM_KeyScanStart                                        ((void (*)(void))func_KeyScanStart)
#define ROM_KeyScanStop                                         ((void (*)(void))func_KeyScanStop)
#define ROM_KeyScanPadEn                                        ((void (*)(unsigned long ulPadMsk))func_KeyScanPadEn)
#define ROM_KeyScanPadDis                                       ((void (*)(unsigned long ulPadMsk))func_KeyScanPadDis)
#define ROM_KeyColValidEn                                       ((void (*)(unsigned long ulKeyColValid))func_KeyColValidEn)
#define ROM_KeyColValidDis                                      ((void (*)(unsigned long ulKeyColValid))func_KeyColValidDis)
#define ROM_KeyRowAndColNumSet                                  ((unsigned char (*)(unsigned char ucRowNum, unsigned char ucColNum))func_KeyRowAndColNumSet)
#define ROM_KeyScanConfig                                       ((void (*)(unsigned char ucHoldCount, unsigned char ucScanRowCount, unsigned char ucScanCount, unsigned char ucVolState, unsigned char ucClkConfig))func_KeyScanConfig)
#define ROM_KeyScanLongPressEnable                              ((void (*)(unsigned char ucLongPressCount))func_KeyScanLongPressEnable)
#define ROM_KeyScanLongPressDisable                             ((void (*)(void))func_KeyScanLongPressDisable)
#define ROM_KeyScanValidStatus                                  ((unsigned char (*)(void))func_KeyScanValidStatus)
#define ROM_KeyScanValidCountGet                                ((unsigned char (*)(void))func_KeyScanValidCountGet)
#define ROM_KeyScanValidKeyCoordinate                           ((unsigned long (*)(void))func_KeyScanValidKeyCoordinate)
#define ROM_KeyCoordinateConvert                                ((void (*)(const unsigned long ulKeyCoordinate, unsigned char* const ucKeyValue))func_KeyCoordinateConvert)
#define ROM_KeyScanIntRegister                                  ((void (*)(unsigned long *g_pRAMVectors, void (*pfnHandler)(void)))func_KeyScanIntRegister)
#define ROM_KeyScanIntUnregister                                ((void (*)(unsigned long *g_pRAMVectors))func_KeyScanIntUnregister)
#define ROM_KeyScanIntEnable                                    ((void (*)(void))func_KeyScanIntEnable)
#define ROM_KeyScanIntDisable                                   ((void (*)(void))func_KeyScanIntDisable)
#define ROM_LCDC_Enable                                         ((void (*)(void))func_LCDC_Enable)
#define ROM_LCDC_Disable                                        ((void (*)(void))func_LCDC_Disable)
#define ROM_LCDC_DutySet                                        ((void (*)(unsigned char ucValue))func_LCDC_DutySet)
#define ROM_LCDC_BiasSet                                        ((void (*)(unsigned char ucValue))func_LCDC_BiasSet)
#define ROM_LCDC_MDSet                                          ((void (*)(unsigned char ucValue))func_LCDC_MDSet)
#define ROM_LCDC_LwaveSet                                       ((void (*)(unsigned char ucValue))func_LCDC_LwaveSet)
#define ROM_LCDC_LwaveGet                                       ((unsigned char (*)(void))func_LCDC_LwaveGet)
#define ROM_LCDC_ForceclkEnable                                 ((void (*)(void))func_LCDC_ForceclkEnable)
#define ROM_LCDC_ForceclkDisable                                ((void (*)(void))func_LCDC_ForceclkDisable)
#define ROM_LCDC_BlinkSet                                       ((void (*)(unsigned char ucValue))func_LCDC_BlinkSet)
#define ROM_LCDC_DeadSet                                        ((void (*)(unsigned char ucValue))func_LCDC_DeadSet)
#define ROM_LCDC_UvolrefSet                                     ((void (*)(unsigned char ucValue))func_LCDC_UvolrefSet)
#define ROM_LCDC_Uvolnowaitstable                               ((void (*)(void))func_LCDC_Uvolnowaitstable)
#define ROM_LCDC_Uvolwaitstable                                 ((void (*)(void))func_LCDC_Uvolwaitstable)
#define ROM_LCDC_FCRpsSet                                       ((void (*)(unsigned char ucValue))func_LCDC_FCRpsSet)
#define ROM_LCDC_FCRdivSet                                      ((void (*)(unsigned char ucValue))func_LCDC_FCRdivSet)
#define ROM_LCDC_ComsegmuxSet                                   ((void (*)(unsigned char ucValue))func_LCDC_ComsegmuxSet)
#define ROM_LCDC_ComsegmuxGet                                   ((unsigned char (*)(void))func_LCDC_ComsegmuxGet)
#define ROM_LPTimerEnable                                       ((void (*)(unsigned long ulBase))func_LPTimerEnable)
#define ROM_LPTimerDisable                                      ((void (*)(unsigned long ulBase))func_LPTimerDisable)
#define ROM_LPTimerDualEnable                                   ((void (*)(void))func_LPTimerDualEnable)
#define ROM_LPTimerConfigure                                    ((void (*)(unsigned long ulBase, unsigned long ulConfig))func_LPTimerConfigure)
#define ROM_LPTimerInitCountValueSet                            ((void (*)(unsigned long ulBase, unsigned short ulValue))func_LPTimerInitCountValueSet)
#define ROM_LPTimerReloadValueSet                               ((void (*)(unsigned long ulBase, unsigned short ulValue))func_LPTimerReloadValueSet)
#define ROM_LPTimerPWMValueSet                                  ((void (*)(unsigned long ulBase, unsigned short ulValue))func_LPTimerPWMValueSet)
#define ROM_LPTimerPolaritySet                                  ((void (*)(unsigned long ulBase, unsigned short ulInvert))func_LPTimerPolaritySet)
#define ROM_LPTimerPrescaleSet                                  ((void (*)(unsigned long ulBase, unsigned short usValue))func_LPTimerPrescaleSet)
#define ROM_LPTimerPWMDelaySet                                  ((void (*)(unsigned long ulBase, unsigned short usValue))func_LPTimerPWMDelaySet)
#define ROM_LPTimerCountValueGet                                ((unsigned short (*)(unsigned long ulBase))func_LPTimerCountValueGet)
#define ROM_LPTimerReloadValueGet                               ((unsigned short (*)(unsigned long ulBase))func_LPTimerReloadValueGet)
#define ROM_LPTimerCaptureValueGet                              ((unsigned short (*)(unsigned long ulBase))func_LPTimerCaptureValueGet)
#define ROM_LPTimerPolarityGet                                  ((unsigned char (*)(unsigned long ulBase))func_LPTimerPolarityGet)
#define ROM_LPTimerClockSrcMux                                  ((void (*)(unsigned long ulBase, unsigned char ucClkConfig))func_LPTimerClockSrcMux)
#define ROM_LPTimerClockSwitchEnable                            ((void (*)(unsigned long ulBase))func_LPTimerClockSwitchEnable)
#define ROM_LPTimerClockSwitchDisable                           ((void (*)(unsigned long ulBase))func_LPTimerClockSwitchDisable)
#define ROM_LPTimerClockSwitch                                  ((void (*)(unsigned long ulBase, unsigned short usAction))func_LPTimerClockSwitch)
#define ROM_LPTimerClockPolarity                                ((void (*)(unsigned long ulBase, unsigned short usEdge))func_LPTimerClockPolarity)
#define ROM_LPTimerEXTClockFilterDelay                          ((void (*)(unsigned long ulBase, unsigned char ucValue))func_LPTimerEXTClockFilterDelay)
#define ROM_LPTimerClockGateFilterDelay                         ((void (*)(unsigned long ulBase, unsigned char ucValue))func_LPTimerClockGateFilterDelay)
#define ROM_LPTimerClockStateGet                                ((unsigned char (*)(unsigned long ulBase, unsigned char ulClkFlags))func_LPTimerClockStateGet)
#define ROM_LPTimerExtPhaseDetectEnable                         ((void (*)(unsigned long ulBase))func_LPTimerExtPhaseDetectEnable)
#define ROM_LPTimerExtPhaseDetectDisable                        ((void (*)(unsigned long ulBase))func_LPTimerExtPhaseDetectDisable)
#define ROM_LPTimerExtPhaseDetectMode                           ((void (*)(unsigned long ulBase, unsigned short usMode))func_LPTimerExtPhaseDetectMode)
#define ROM_LPTimerIntRegister                                  ((void (*)(unsigned long ulIntChannel, unsigned long *g_pRAMVectors, void (*pfnHandler)(void)))func_LPTimerIntRegister)
#define ROM_LPTimerIntUnregister                                ((void (*)(unsigned long ulIntChannel, unsigned long *g_pRAMVectors))func_LPTimerIntUnregister)
#define ROM_LPTimerIntEnable                                    ((void (*)(unsigned long ulBase, unsigned char ucIntFlags))func_LPTimerIntEnable)
#define ROM_LPTimerIntDisable                                   ((void (*)(unsigned long ulBase))func_LPTimerIntDisable)
#define ROM_LPTimerIntStatus                                    ((unsigned char (*)(void))func_LPTimerIntStatus)
#define ROM_LPTimerIntEventGet                                  ((unsigned char (*)(unsigned long ulBase))func_LPTimerIntEventGet)
#define ROM_MCNTStart                                           ((void (*)(void))func_MCNTStart)
#define ROM_MCNTStop                                            ((void (*)(void))func_MCNTStop)
#define ROM_MCNTSetCNT32K                                       ((void (*)(unsigned long ulCounter))func_MCNTSetCNT32K)
#define ROM_MCNTGetCNT32K                                       ((unsigned long (*)(void))func_MCNTGetCNT32K)
#define ROM_MCNTGetMCNT                                         ((unsigned long (*)(void))func_MCNTGetMCNT)
#define ROM_MCNTGetIntStatus                                    ((unsigned char (*)(void))func_MCNTGetIntStatus)
#define ROM_MCNTSetClkSrc                                       ((void (*)(unsigned long ulClkSrc))func_MCNTSetClkSrc)
#define ROM_MCNTIntRegister                                     ((void (*)(unsigned long *g_pRAMVectors, void (*pfnHandler)(void)))func_MCNTIntRegister)
#define ROM_MCNTIntUnregister                                   ((void (*)(unsigned long *g_pRAMVectors))func_MCNTIntUnregister)
#define ROM_PhyTimerOffsetTMRL                                  ((void (*)(unsigned int offsetVal, int dir))func_PhyTimerOffsetTMRL)
#define ROM_PhyTimerOffsetTMRM                                  ((void (*)(unsigned int offsetVal, int dir))func_PhyTimerOffsetTMRM)
#define ROM_PhyTimerOffsetTMRH                                  ((void (*)(unsigned int offsetVal, int dir))func_PhyTimerOffsetTMRH)
#define ROM_PhyTimerIntStatGet                                  ((uint16_t (*)(void))func_PhyTimerIntStatGet)
#define ROM_PhyTimerIntStatClear                                ((void (*)(uint16_t int_reg))func_PhyTimerIntStatClear)
#define ROM_PhyTimerIntEnable                                   ((void (*)(uint16_t int_reg))func_PhyTimerIntEnable)
#define ROM_PhyTimerEnable                                      ((void (*)(void))func_PhyTimerEnable)
#define ROM_PhyTimerDisable                                     ((void (*)(void))func_PhyTimerDisable)
#define ROM_FRC_GetLocalFRC                                     ((uint32_t (*)(FRC_TIME_t * LocalFRC))func_FRC_GetLocalFRC)
#define ROM_TRX_CfgCnt_Mode                                     ((void (*)(uint8_t index, uint8_t mode))func_TRX_CfgCnt_Mode)
#define ROM_TRX_CfgCnt_Set                                      ((void (*)(uint8_t index, FRC_TIME_t* dealFRC))func_TRX_CfgCnt_Set)
#define ROM_AON_CP_Load_Flag_Get                                ((unsigned char (*)(void))func_AON_CP_Load_Flag_Get)
#define ROM_AON_AP_Load_Flag_Get                                ((unsigned char (*)(void))func_AON_AP_Load_Flag_Get)
#define ROM_AON_SECONDARY_Load_Flag_Get                         ((unsigned char (*)(void))func_AON_SECONDARY_Load_Flag_Get)
#define ROM_AON_CP_Load_Flag_Set                                ((void (*)(uint8_t flag))func_AON_CP_Load_Flag_Set)
#define ROM_AON_AP_Load_Flag_Set                                ((void (*)(uint8_t flag))func_AON_AP_Load_Flag_Set)
#define ROM_AON_SECONDARY_Load_Flag_Set                         ((void (*)(uint8_t flag))func_AON_SECONDARY_Load_Flag_Set)
#define ROM_AON_Flash_Delay_Get                                 ((void (*)(void))func_AON_Flash_Delay_Get)
#define ROM_AON_Flash_Delay_Set                                 ((void (*)(uint8_t value))func_AON_Flash_Delay_Set)
#define ROM_AON_Flash_tPWD_Delay_Get                            ((void (*)(void))func_AON_Flash_tPWD_Delay_Get)
#define ROM_AON_Flash_tRST_Delay_Get                            ((void (*)(void))func_AON_Flash_tRST_Delay_Get)
#define ROM_AON_Flash_tPWD_Delay_Set                            ((void (*)(uint8_t value))func_AON_Flash_tPWD_Delay_Set)
#define ROM_AON_BOOTMODE_GET                                    ((unsigned char (*)(void))func_AON_BOOTMODE_GET)
#define ROM_AON_CP_Memory_Remap_Enable                          ((void (*)(unsigned char ucRemapMode))func_AON_CP_Memory_Remap_Enable)
#define ROM_AON_CP_Memory_Remap_Disable                         ((void (*)(void))func_AON_CP_Memory_Remap_Disable)
#define ROM_AON_CP_Core_Release                                 ((void (*)(void))func_AON_CP_Core_Release)
#define ROM_AON_CP_Core_Hold                                    ((void (*)(void))func_AON_CP_Core_Hold)
#define ROM_AON_UP_STATUS_Get                                   ((unsigned long (*)(void))func_AON_UP_STATUS_Get)
#define ROM_AON_System_Clock_Select                             ((void (*)(unsigned char ucClkSrc))func_AON_System_Clock_Select)
#define ROM_Core_PRCM_Clock_Enable0                             ((void (*)(unsigned long ulModule))func_Core_PRCM_Clock_Enable0)
#define ROM_Core_PRCM_Clock_Disable0                            ((void (*)(unsigned long ulModule))func_Core_PRCM_Clock_Disable0)
#define ROM_Core_PRCM_Clock_Enable1                             ((void (*)(unsigned char ucModule))func_Core_PRCM_Clock_Enable1)
#define ROM_Core_PRCM_Clock_Disable1                            ((void (*)(unsigned char ucModule))func_Core_PRCM_Clock_Disable1)
#define ROM_Core_PRCM_CHIP_VER_GET                              ((unsigned long (*)(void))func_Core_PRCM_CHIP_VER_GET)
#define ROM_qspi_apb_controller_init                            ((void (*)(QSPI_FLASH_Def *plat))func_qspi_apb_controller_init)
#define ROM_qspi_apb_controller_enable                          ((void (*)(void))func_qspi_apb_controller_enable)
#define ROM_qspi_apb_controller_disable                         ((void (*)(void))func_qspi_apb_controller_disable)
#define ROM_qspi_wait_indirect_complete                         ((void (*)(void))func_qspi_wait_indirect_complete)
#define ROM_qspi_apb_command_read                               ((int (*)(unsigned int cmdlen, const uint8_t *cmdbuf, unsigned int rxlen, uint8_t *rxbuf))func_qspi_apb_command_read)
#define ROM_qspi_apb_command_write                              ((int (*)(unsigned int cmdlen, const uint8_t *cmdbuf, unsigned int txlen,  const uint8_t *txbuf))func_qspi_apb_command_write)
#define ROM_qspi_apb_config_baudrate_div                        ((void (*)(unsigned int ref_clk_hz, unsigned int sclk_hz))func_qspi_apb_config_baudrate_div)
#define ROM_qspi_apb_delay                                      ((void (*)(unsigned int ref_clk, unsigned int sclk_hz, unsigned int tshsl_ns, unsigned int tsd2d_ns, unsigned int tchsh_ns, unsigned int tslch_ns))func_qspi_apb_delay)
#define ROM_qspi_wait_idle                                      ((void (*)(void))func_qspi_wait_idle)
#define ROM_qspi_rbuf_wait_idle                                 ((void (*)(void))func_qspi_rbuf_wait_idle)
#define ROM_qspi_wbuf_wait_idle                                 ((void (*)(void))func_qspi_wbuf_wait_idle)
#define ROM_qspi_wait_writeburst_done                           ((void (*)(void))func_qspi_wait_writeburst_done)
#define ROM_qspi_apb_indirect_enable                            ((void (*)(void))func_qspi_apb_indirect_enable)
#define ROM_qspi_apb_indirect_disable                           ((void (*)(void))func_qspi_apb_indirect_disable)
#define ROM_qspi_apb_indirect_write                             ((void (*)(QSPI_FLASH_Def *plat, unsigned int dst, unsigned int n_bytes))func_qspi_apb_indirect_write)
#define ROM_qspi_apb_indirect_read                              ((void (*)(QSPI_FLASH_Def *plat, unsigned int src, unsigned int n_bytes))func_qspi_apb_indirect_read)
#define ROM_qspi_apb_set_read_instruction                       ((void (*)(unsigned char devId, unsigned int config))func_qspi_apb_set_read_instruction)
#define ROM_qspi_apb_set_write_instruction                      ((void (*)(unsigned char devId, unsigned int config))func_qspi_apb_set_write_instruction)
#define ROM_QSPIIntRegister                                     ((void (*)(unsigned long *g_pRAMVectors, void (*pfnHandler)(void)))func_QSPIIntRegister)
#define ROM_QSPIIntUnregister                                   ((void (*)(unsigned long *g_pRAMVectors))func_QSPIIntUnregister)
#define ROM_qspi_dma_bstsize_set                                ((void (*)(unsigned char ucBstsize))func_qspi_dma_bstsize_set)
#define ROM_qspi_dma_bstsize_get                                ((unsigned char (*)(void))func_qspi_dma_bstsize_get)
#define ROM_qspi_sus_cmd_check_enable                           ((void (*)(void))func_qspi_sus_cmd_check_enable)
#define ROM_qspi_sus_cmd_set                                    ((void (*)(unsigned int sus_cmd))func_qspi_sus_cmd_set)
#define ROM_qspi_sus_cs_set                                     ((void (*)(unsigned char sus_cs))func_qspi_sus_cs_set)
#define ROM_qspi_block_alldevice_set                            ((void (*)(unsigned char flag))func_qspi_block_alldevice_set)
#define ROM_qspi_block_alldevice_get                            ((unsigned char (*)(void))func_qspi_block_alldevice_get)
#define ROM_qspi_tsus_cnt_set                                   ((void (*)(unsigned char tresume_cnt, unsigned int tsus_cnt))func_qspi_tsus_cnt_set)
#define ROM_qspi_sus_opcode_set                                 ((void (*)(unsigned char sus_opcode))func_qspi_sus_opcode_set)
#define ROM_qspi_resume_opcode_set                              ((void (*)(unsigned char resume_opcode))func_qspi_resume_opcode_set)
#define ROM_qspi_tsus_timeout_cnt_set                           ((void (*)(unsigned char tsus_timeout_cnt))func_qspi_tsus_timeout_cnt_set)
#define ROM_qspi_clear_sus_cmd_hit                              ((void (*)(void))func_qspi_clear_sus_cmd_hit)
#define ROM_qspi_ap_erase_write_req                             ((void (*)(void))func_qspi_ap_erase_write_req)
#define ROM_qspi_ap_dma_erase_write_req                         ((void (*)(void))func_qspi_ap_dma_erase_write_req)
#define ROM_qspi_ap_wait_erase_write_req_ack                    ((void (*)(void))func_qspi_ap_wait_erase_write_req_ack)
#define ROM_qspi_cp_send_erase_write_req_ack                    ((void (*)(void))func_qspi_cp_send_erase_write_req_ack)
#define ROM_qspi_cp_wait_erase_write_done                       ((void (*)(void))func_qspi_cp_wait_erase_write_done)
#define ROM_qspi_ap_send_erase_write_done                       ((void (*)(void))func_qspi_ap_send_erase_write_done)
#define ROM_SEMARelease                                         ((void (*)(unsigned char ucSemaSlave))func_SEMARelease)
#define ROM_SEMAResetAll                                        ((void (*)(void))func_SEMAResetAll)
#define ROM_SEMARequest                                         ((void (*)(unsigned char ucSemaMaster, unsigned char ucSemaSlave, unsigned long ulDmacReq,unsigned short usSemaPority, unsigned char ucMasterMask))func_SEMARequest)
#define ROM_SEMARequestNonBlocking                              ((void (*)(unsigned char ucSemaSlave, unsigned long ulDmacReq,unsigned short usSemaPority, unsigned char ucMasterMask))func_SEMARequestNonBlocking)
#define ROM_SEMAReqQueueState                                   ((unsigned char (*)(void))func_SEMAReqQueueState)
#define ROM_SEMAMasterGet                                       ((unsigned char (*)(unsigned char ucSemaSlave))func_SEMAMasterGet)
#define ROM_SEMASlaveGet                                        ((void (*)(unsigned char ucSemaMaster, unsigned long* pulSlaveReg0, unsigned long* pulSlaveReg1))func_SEMASlaveGet)
#define ROM_SEMAIntRegister                                     ((void (*)(unsigned long *g_pRAMVectors, void (*pfnHandler)(void)))func_SEMAIntRegister)
#define ROM_SEMAIntUnregister                                   ((void (*)(unsigned long *g_pRAMVectors))func_SEMAIntUnregister)
#define ROM_SEMAMasterIntEnable                                 ((void (*)(unsigned long ulSemaMaster))func_SEMAMasterIntEnable)
#define ROM_SEMAMasterIntClear                                  ((void (*)(unsigned long ulSemaMaster))func_SEMAMasterIntClear)
#define ROM_SPIConfigSetExpClk                                  ((void (*)(unsigned long ulDivide,unsigned long ulProtocol,unsigned long ulMode,unsigned long ulDataWidth))func_SPIConfigSetExpClk)
#define ROM_SPIEnable                                           ((void (*)(void))func_SPIEnable)
#define ROM_SPIDisable                                          ((void (*)(void))func_SPIDisable)
#define ROM_SPIChipSelect                                       ((void (*)(unsigned long ulChipSelect))func_SPIChipSelect)
#define ROM_SPISlaveBurstEnable                                 ((void (*)(void))func_SPISlaveBurstEnable)
#define ROM_SPISlaveBurstDisable                                ((void (*)(void))func_SPISlaveBurstDisable)
#define ROM_SPISetTxFifoThreshold                               ((void (*)(unsigned char ucThreshold))func_SPISetTxFifoThreshold)
#define ROM_SPISetRxFifoThreshold                               ((void (*)(unsigned char ucThreshold))func_SPISetRxFifoThreshold)
#define ROM_SPITxFifoReset                                      ((void (*)(void))func_SPITxFifoReset)
#define ROM_SPITxFifoEnable                                     ((void (*)(void))func_SPITxFifoEnable)
#define ROM_SPITxFifoDisable                                    ((void (*)(void))func_SPITxFifoDisable)
#define ROM_SPIRxFifoReset                                      ((void (*)(void))func_SPIRxFifoReset)
#define ROM_SPIRxFifoEnable                                     ((void (*)(void))func_SPIRxFifoEnable)
#define ROM_SPIRxFifoDisable                                    ((void (*)(void))func_SPIRxFifoDisable)
#define ROM_SPISetDelay                                         ((void (*)(unsigned char ucTransNActiveDelay, unsigned char ucTransBetweenDelay,unsigned char ucTransAfterDelay, unsigned char ucTransInitDelay))func_SPISetDelay)
#define ROM_SPIManualCSIdle                                     ((void (*)(void))func_SPIManualCSIdle)
#define ROM_SPIManualCSActive                                   ((void (*)(void))func_SPIManualCSActive)
#define ROM_SPIEnManualTransmit                                 ((void (*)(void))func_SPIEnManualTransmit)
#define ROM_SPIDisManualTransmit                                ((void (*)(void))func_SPIDisManualTransmit)
#define ROM_SPIStartManualTransmit                              ((void (*)(void))func_SPIStartManualTransmit)
#define ROM_SPIModeFailEnable                                   ((void (*)(void))func_SPIModeFailEnable)
#define ROM_SPIModeFailDisable                                  ((void (*)(void))func_SPIModeFailDisable)
#define ROM_SPIIdleCountSet                                     ((void (*)(unsigned char ucCount))func_SPIIdleCountSet)
#define ROM_SPIModeGet                                          ((unsigned char (*)(void))func_SPIModeGet)
#define ROM_SPIClockPhaseGet                                    ((unsigned char (*)(void))func_SPIClockPhaseGet)
#define ROM_SPIClockPolarityGet                                 ((unsigned char (*)(void))func_SPIClockPolarityGet)
#define ROM_SPIClockDivGet                                      ((unsigned char (*)(void))func_SPIClockDivGet)
#define ROM_SPIDataWidthGet                                     ((unsigned char (*)(void))func_SPIDataWidthGet)
#define ROM_SPIRxFifoStatusGet                                  ((unsigned char (*)(unsigned char ucFlagType))func_SPIRxFifoStatusGet)
#define ROM_SPITxFifoStatusGet                                  ((unsigned char (*)(unsigned char ucFlagType))func_SPITxFifoStatusGet)
#define ROM_SPIIsEnable                                         ((unsigned char (*)(void))func_SPIIsEnable)
#define ROM_SPIIntRegister                                      ((void (*)(unsigned long *g_pRAMVectors, void(*pfnHandler)(void)))func_SPIIntRegister)
#define ROM_SPIIntUnregister                                    ((void (*)(unsigned long *g_pRAMVectors))func_SPIIntUnregister)
#define ROM_SPIIntEnable                                        ((void (*)(unsigned long ulIntFlags))func_SPIIntEnable)
#define ROM_SPIIntDisable                                       ((void (*)(unsigned long ulIntFlags))func_SPIIntDisable)
#define ROM_SPIIntClear                                         ((void (*)(unsigned long ulIntFlags))func_SPIIntClear)
#define ROM_SPIIntStatus                                        ((unsigned long (*)(void))func_SPIIntStatus)
#define ROM_SPIIntMasked                                        ((unsigned long (*)(void))func_SPIIntMasked)
#define ROM_SPIBytePut                                          ((void (*)(unsigned char ucData))func_SPIBytePut)
#define ROM_SPINBytePut                                         ((void (*)(unsigned char *pucData, unsigned long ulByteLen))func_SPINBytePut)
#define ROM_SPIBytePutNonBlocking                               ((unsigned char (*)(unsigned char ucData))func_SPIBytePutNonBlocking)
#define ROM_SPIWordPut                                          ((void (*)(unsigned long ulData))func_SPIWordPut)
#define ROM_SPINWordPut                                         ((void (*)(unsigned long *pulData, unsigned long ulWordLen))func_SPINWordPut)
#define ROM_SPIByteGet                                          ((void (*)(unsigned char *pucData))func_SPIByteGet)
#define ROM_SPIByteGetNonBlocking                               ((unsigned char (*)(unsigned char *pucData))func_SPIByteGetNonBlocking)
#define ROM_SPIWordGet                                          ((void (*)(unsigned long *pulData))func_SPIWordGet)
#define ROM_SPIByteGetWithTimeout                               ((unsigned long (*)(unsigned char *pucRecvData, unsigned long ulLenByte, unsigned long ulTimeout))func_SPIByteGetWithTimeout)
#define ROM_SPIByteClearWithTimeout                             ((unsigned long (*)(unsigned long ulLenByte, unsigned long ulTimeout))func_SPIByteClearWithTimeout)
#define ROM_SysTickEnable                                       ((void (*)(void))func_SysTickEnable)
#define ROM_SysTickDisable                                      ((void (*)(void))func_SysTickDisable)
#define ROM_SysTickIntRegister                                  ((void (*)(unsigned long *g_pRAMVectors, void (*pfnHandler)(void)))func_SysTickIntRegister)
#define ROM_SysTickIntUnregister                                ((void (*)(unsigned long *g_pRAMVectors))func_SysTickIntUnregister)
#define ROM_SysTickIntEnable                                    ((void (*)(void))func_SysTickIntEnable)
#define ROM_SysTickIntDisable                                   ((void (*)(void))func_SysTickIntDisable)
#define ROM_SysTickPeriodSet                                    ((void (*)(unsigned long ulPeriod))func_SysTickPeriodSet)
#define ROM_SysTickPeriodGet                                    ((unsigned long (*)(void))func_SysTickPeriodGet)
#define ROM_SysTickValueGet                                     ((unsigned long (*)(void))func_SysTickValueGet)
#define ROM_TickTimerEnable                                     ((void (*)(void))func_TickTimerEnable)
#define ROM_TickTimerDisable                                    ((void (*)(void))func_TickTimerDisable)
#define ROM_TickPrescaleSet                                     ((void (*)(unsigned char ucPrescale))func_TickPrescaleSet)
#define ROM_TickAPReloadSet                                     ((void (*)(unsigned char ucReload))func_TickAPReloadSet)
#define ROM_TickCPReloadSet                                     ((void (*)(unsigned char ucReload))func_TickCPReloadSet)
#define ROM_TickAPReloadGet                                     ((unsigned long (*)(void))func_TickAPReloadGet)
#define ROM_TickCPReloadGet                                     ((unsigned long (*)(void))func_TickCPReloadGet)
#define ROM_TickCounterSet                                      ((void (*)(unsigned long ulCounter))func_TickCounterSet)
#define ROM_TickCounterGet                                      ((unsigned int (*)(void))func_TickCounterGet)
#define ROM_TickAPCompareSet                                    ((void (*)(unsigned long ulCompare))func_TickAPCompareSet)
#define ROM_TickCPCompareSet                                    ((void (*)(unsigned long ulCompare))func_TickCPCompareSet)
#define ROM_TickAPIntEnable                                     ((void (*)(unsigned char ucConfig))func_TickAPIntEnable)
#define ROM_TickCPIntEnable                                     ((void (*)(unsigned char ucConfig))func_TickCPIntEnable)
#define ROM_TickAPIntDisable                                    ((void (*)(unsigned char ucConfig))func_TickAPIntDisable)
#define ROM_TickCPIntDisable                                    ((void (*)(unsigned char ucConfig))func_TickCPIntDisable)
#define ROM_TickIntRegister                                     ((void (*)(unsigned long *g_pRAMVectors, void (*pfnHandler)(void)))func_TickIntRegister)
#define ROM_TickIntUnregister                                   ((void (*)(unsigned long *g_pRAMVectors))func_TickIntUnregister)
#define ROM_TickAPReadAndClearInt                               ((unsigned char (*)(void))func_TickAPReadAndClearInt)
#define ROM_TickCPReadAndClearInt                               ((unsigned char (*)(void))func_TickCPReadAndClearInt)
#define ROM_TimerEnable                                         ((void (*)(unsigned long ulBase))func_TimerEnable)
#define ROM_TimerDisable                                        ((void (*)(unsigned long ulBase))func_TimerDisable)
#define ROM_TimerConfigure                                      ((void (*)(unsigned long ulBase, unsigned long ulConfig))func_TimerConfigure)
#define ROM_TimerInitCountValueSet                              ((void (*)(unsigned long ulBase, unsigned long ulValue))func_TimerInitCountValueSet)
#define ROM_TimerReloadValueSet                                 ((void (*)(unsigned long ulBase, unsigned long ulValue))func_TimerReloadValueSet)
#define ROM_TimerPWMValueSet                                    ((void (*)(unsigned long ulBase, unsigned long ulValue))func_TimerPWMValueSet)
#define ROM_TimerPolaritySet                                    ((void (*)(unsigned long ulBase, unsigned char ucInvert))func_TimerPolaritySet)
#define ROM_TimerPrescaleSet                                    ((void (*)(unsigned long ulBase, unsigned long ulValue))func_TimerPrescaleSet)
#define ROM_TimerPWMDelaySet                                    ((void (*)(unsigned long ulBase, unsigned long ulValue))func_TimerPWMDelaySet)
#define ROM_TimerCountOffset                                    ((void (*)(unsigned long ulBase, unsigned long ulOffsetDirect, unsigned long ulValue))func_TimerCountOffset)
#define ROM_TimerCountValueGet                                  ((unsigned long (*)(unsigned long ulBase))func_TimerCountValueGet)
#define ROM_TimerReloadValueGet                                 ((unsigned long (*)(unsigned long ulBase))func_TimerReloadValueGet)
#define ROM_TimerCaptureValueGet                                ((unsigned long (*)(unsigned long ulBase))func_TimerCaptureValueGet)
#define ROM_TimerPolarityGet                                    ((unsigned char (*)(unsigned long ulBase))func_TimerPolarityGet)
#define ROM_TimerIntRegister                                    ((void (*)(unsigned long ulIntChannel, unsigned long *g_pRAMVectors, void (*pfnHandler)(void)))func_TimerIntRegister)
#define ROM_TimerIntUnregister                                  ((void (*)(unsigned long ulIntChannel, unsigned long *g_pRAMVectors))func_TimerIntUnregister)
#define ROM_TimerIntEnable                                      ((void (*)(unsigned long ulBase, unsigned long ulIntFlags))func_TimerIntEnable)
#define ROM_TimerIntEventGet                                    ((unsigned char (*)(unsigned long ulBase))func_TimerIntEventGet)
#define ROM_TRNG_Resetblock                                     ((void (*)(void))func_TRNG_Resetblock)
#define ROM_TRNG_EnableHwRngClock                               ((void (*)(void))func_TRNG_EnableHwRngClock)
#define ROM_TRNG_SetSamlpeCountValue                            ((void (*)(uint32_t countValue))func_TRNG_SetSamlpeCountValue)
#define ROM_TRNG_ReadSamlpeCountValue                           ((uint32_t (*)(void))func_TRNG_ReadSamlpeCountValue)
#define ROM_TRNG_SetRngRoscLength                               ((void (*)(uint32_t roscLength))func_TRNG_SetRngRoscLength)
#define ROM_TRNG_FastModeBypass                                 ((void (*)(void))func_TRNG_FastModeBypass)
#define ROM_TRNG_FeModeBypass                                   ((void (*)(void))func_TRNG_FeModeBypass)
#define ROM_TRNG_80090bModeBypass                               ((void (*)(void))func_TRNG_80090bModeBypass)
#define ROM_TRNG_EnableRndSource                                ((void (*)(void))func_TRNG_EnableRndSource)
#define ROM_TRNG_DisableRndSource                               ((void (*)(void))func_TRNG_DisableRndSource)
#define ROM_TRNG_ReadValidReg                                   ((uint32_t (*)(void))func_TRNG_ReadValidReg)
#define ROM_TRNG_ReadValidISRReg                                ((uint32_t (*)(void))func_TRNG_ReadValidISRReg)
#define ROM_TRNG_CleanUpInterruptStatus                         ((void (*)(void))func_TRNG_CleanUpInterruptStatus)
#define ROM_TRNG_checkInput                                     ((int (*)(uint32_t TRNGMode,uint32_t roscLength,uint32_t sampleCount ))func_TRNG_checkInput)
#define ROM_TRNG_initializingHardware                           ((void (*)(uint32_t TRNGMode,uint32_t roscLength,uint32_t sampleCount ))func_TRNG_initializingHardware)
#define ROM_TRNG_stopHardware                                   ((void (*)(void))func_TRNG_stopHardware)
#define ROM_TRNG_rndWriteHeader                                 ((void (*)(uint32_t TRNGMode,uint32_t roscLength,uint32_t sampleCount,uint8_t* dataU8Array))func_TRNG_rndWriteHeader)
#define ROM_TRNG_loadEHR                                        ((int (*)(uint8_t* dataU8Array))func_TRNG_loadEHR)
#define ROM_TRNG_rndWriteFooter                                 ((void (*)(uint32_t Error,uint8_t* dataU8Array))func_TRNG_rndWriteFooter)
#define ROM_TRNG_collectU8Array                                 ((void (*)(uint32_t p, uint8_t* u8Array, uint32_t* u32Array))func_TRNG_collectU8Array)
#define ROM_UARTEnable                                          ((void (*)(unsigned long ulBase))func_UARTEnable)
#define ROM_UARTDisable                                         ((void (*)(unsigned long ulBase))func_UARTDisable)
#define ROM_UARTFIFOEnable                                      ((void (*)(unsigned long ulBase, unsigned char ucFIFOFlags))func_UARTFIFOEnable)
#define ROM_UARTFIFODisable                                     ((void (*)(unsigned long ulBase, unsigned char ucFIFOFlags))func_UARTFIFODisable)
#define ROM_UARTDmaTransferEnable                               ((void (*)(unsigned long ulBase))func_UARTDmaTransferEnable)
#define ROM_UARTDmaTransferDisable                              ((void (*)(unsigned long ulBase))func_UARTDmaTransferDisable)
#define ROM_UARTFIFOLevelSet                                    ((void (*)(unsigned long ulBase, unsigned long ulRxLevel, unsigned long ulTxLevel))func_UARTFIFOLevelSet)
#define ROM_UARTTxWaitSet                                       ((void (*)(unsigned long ulBase, unsigned char ucWaitTime))func_UARTTxWaitSet)
#define ROM_UARTWaitTxDone                                      ((void (*)(unsigned long ulBase))func_UARTWaitTxDone)
#define ROM_UARTConfigSetExpClk                                 ((void (*)(unsigned long ulBase, unsigned long ulPclk,unsigned long ulBaudrate, unsigned long ulConfig))func_UARTConfigSetExpClk)
#define ROM_UARTConfigGetExpClk                                 ((void (*)(unsigned long ulBase, unsigned long ulPclk,unsigned long *pulBaud, unsigned long *pulConfig))func_UARTConfigGetExpClk)
#define ROM_UARTAutoBaudrate                                    ((void (*)(unsigned long ulBase, unsigned long ulConfig))func_UARTAutoBaudrate)
#define ROM_UARTABDEndStatus                                    ((unsigned char (*)(unsigned long ulBase))func_UARTABDEndStatus)
#define ROM_UARTIntRegister                                     ((void (*)(unsigned long ulIntChannel, unsigned long *g_pRAMVectors, void (*pfnHandler)(void)))func_UARTIntRegister)
#define ROM_UARTIntUnregister                                   ((void (*)(unsigned long ulIntChannel, unsigned long *g_pRAMVectors))func_UARTIntUnregister)
#define ROM_UARTIntEnable                                       ((void (*)(unsigned long ulBase, unsigned long ulIntFlags))func_UARTIntEnable)
#define ROM_UARTIntDisable                                      ((void (*)(unsigned long ulBase, unsigned long ulIntFlags))func_UARTIntDisable)
#define ROM_UARTIntReadAndClear                                 ((unsigned short (*)(unsigned long ulBase))func_UARTIntReadAndClear)
#define ROM_UARTIntMasked                                       ((unsigned short (*)(unsigned long ulBase))func_UARTIntMasked)
#define ROM_UARTRxIdle                                          ((unsigned char (*)(unsigned long ulBase))func_UARTRxIdle)
#define ROM_UARTFIFOFlush                                       ((void (*)(unsigned long ulBase, unsigned char ucFIFOFlags))func_UARTFIFOFlush)
#define ROM_UARTRxFifoStatusGet                                 ((unsigned char (*)(unsigned long ulBase, unsigned char ucFlagType))func_UARTRxFifoStatusGet)
#define ROM_UARTTxFifoStatusGet                                 ((unsigned char (*)(unsigned long ulBase, unsigned char ucFlagType))func_UARTTxFifoStatusGet)
#define ROM_UARTFIFOByteLevel                                   ((unsigned char (*)(unsigned long ulBase, unsigned char ucFIFOFlags))func_UARTFIFOByteLevel)
#define ROM_UARTWakeUpModeConfig                                ((void (*)(unsigned long ulBase, unsigned char ucConfig, unsigned char ucPattern1, unsigned char ucPattern2))func_UARTWakeUpModeConfig)
#define ROM_UARTWakeUpModeEnable                                ((void (*)(unsigned long ulBase))func_UARTWakeUpModeEnable)
#define ROM_UARTWakeUpModeDisable                               ((void (*)(unsigned long ulBase))func_UARTWakeUpModeDisable)
#define ROM_UARTSequenceDetectModeSet                           ((void (*)(unsigned long ulBase, unsigned char ulValidBits, unsigned char ulPattern))func_UARTSequenceDetectModeSet)
#define ROM_UARTSequenceDetectEnable                            ((void (*)(unsigned long ulBase))func_UARTSequenceDetectEnable)
#define ROM_UARTSequenceDetectDisable                           ((void (*)(unsigned long ulBase))func_UARTSequenceDetectDisable)
#define ROM_UARTTimeOutConfig                                   ((void (*)(unsigned long ulBase, unsigned char ucStartCondition, unsigned char ucValue))func_UARTTimeOutConfig)
#define ROM_UARTTimeOutEnable                                   ((void (*)(unsigned long ulBase))func_UARTTimeOutEnable)
#define ROM_UARTTimeOutDisable                                  ((void (*)(unsigned long ulBase))func_UARTTimeOutDisable)
#define ROM_UARTRXEnaStatus                                     ((unsigned char (*)(unsigned long ulBase))func_UARTRXEnaStatus)
#define ROM_UARTFlowCtrlEnable                                  ((void (*)(unsigned long ulBase))func_UARTFlowCtrlEnable)
#define ROM_UARTFlowCtrlDisable                                 ((void (*)(unsigned long ulBase))func_UARTFlowCtrlDisable)
#define ROM_UARTFlowCtrlConfig                                  ((void (*)(unsigned long ulBase, unsigned char ucConfig))func_UARTFlowCtrlConfig)
#define ROM_UARTFlowCtrlRtsGet                                  ((unsigned char (*)(unsigned long ulBase))func_UARTFlowCtrlRtsGet)
#define ROM_UARTFlowCtrlRtsSet                                  ((void (*)(unsigned long ulBase))func_UARTFlowCtrlRtsSet)
#define ROM_UARTFlowCtrlRtsClear                                ((void (*)(unsigned long ulBase))func_UARTFlowCtrlRtsClear)
#define ROM_UARTFlowCtrlCtsGet                                  ((unsigned char (*)(unsigned long ulBase))func_UARTFlowCtrlCtsGet)
#define ROM_UARTFlowCtrlCtsSet                                  ((void (*)(unsigned long ulBase))func_UARTFlowCtrlCtsSet)
#define ROM_UARTFlowCtrlCtsClear                                ((void (*)(unsigned long ulBase))func_UARTFlowCtrlCtsClear)
#define ROM_UARTStartOffsetConfig                               ((void (*)(unsigned long ulBase, unsigned char ucValue))func_UARTStartOffsetConfig)
#define ROM_UARTStartOffsetEnable                               ((void (*)(unsigned long ulBase))func_UARTStartOffsetEnable)
#define ROM_UARTStartOffsetFlag                                 ((unsigned char (*)(unsigned long ulBase))func_UARTStartOffsetFlag)
#define ROM_UARTCharGet                                         ((unsigned char (*)(unsigned long ulBase))func_UARTCharGet)
#define ROM_UARTCharGetWithTimeout                              ((unsigned long (*)(unsigned long ulBase, unsigned char *pucRecvData, unsigned long ulLenByte, unsigned long ulTimeout))func_UARTCharGetWithTimeout)
#define ROM_UARTHalfWordGet                                     ((unsigned short (*)(unsigned long ulBase))func_UARTHalfWordGet)
#define ROM_UARTWordGet                                         ((unsigned long (*)(unsigned long ulBase))func_UARTWordGet)
#define ROM_UARTCharGetNonBlocking                              ((unsigned char (*)(unsigned long ulBase))func_UARTCharGetNonBlocking)
#define ROM_UARTHalfWordGetNonBlocking                          ((unsigned short (*)(unsigned long ulBase))func_UARTHalfWordGetNonBlocking)
#define ROM_UARTWordGetNonBlocking                              ((unsigned long (*)(unsigned long ulBase))func_UARTWordGetNonBlocking)
#define ROM_UARTCharPut                                         ((void (*)(unsigned long ulBase, unsigned char ucData))func_UARTCharPut)
#define ROM_UARTHalfWordPut                                     ((void (*)(unsigned long ulBase, unsigned short usData))func_UARTHalfWordPut)
#define ROM_UARTWordPut                                         ((void (*)(unsigned long ulBase, unsigned long ulData))func_UARTWordPut)
#define ROM_UARTCharPutNonBlocking                              ((unsigned char (*)(unsigned long ulBase, unsigned char ucData))func_UARTCharPutNonBlocking)
#define ROM_UARTHalfWordPutNonBlocking                          ((unsigned char (*)(unsigned long ulBase, unsigned short usData))func_UARTHalfWordPutNonBlocking)
#define ROM_UARTWordPutNonBlocking                              ((unsigned char (*)(unsigned long ulBase, unsigned long ulData))func_UARTWordPutNonBlocking)
#define ROM_UARTStringPut                                       ((void (*)(unsigned long ulBase, unsigned char *str))func_UARTStringPut)
#define ROM_UARTPrintf                                          ((void (*)(unsigned long ulBase, char* fmt, ...))func_UARTPrintf)
#define ROM_UTCTimerStop                                        ((void (*)(void))func_UTCTimerStop)
#define ROM_UTCTimerRun                                         ((void (*)(void))func_UTCTimerRun)
#define ROM_UTCCalStop                                          ((void (*)(void))func_UTCCalStop)
#define ROM_UTCCalRun                                           ((void (*)(void))func_UTCCalRun)
#define ROM_UTCDivStop                                          ((void (*)(void))func_UTCDivStop)
#define ROM_UTCDivEn                                            ((void (*)(void))func_UTCDivEn)
#define ROM_UTCHourModeSet                                      ((void (*)(unsigned long ulMode))func_UTCHourModeSet)
#define ROM_UTCHourModeGet                                      ((unsigned long (*)(void))func_UTCHourModeGet)
#define ROM_UTCTimerSet                                         ((void (*)(unsigned long ulAMPM, unsigned long ulHour, unsigned long ulMin, unsigned long ulSec, unsigned long ulMinSec))func_UTCTimerSet)
#define ROM_UTCTimerChangeGet                                   ((unsigned long (*)(void))func_UTCTimerChangeGet)
#define ROM_UTCTimerGet                                         ((void (*)(unsigned char *ulAMPM, unsigned char *ulHour, unsigned char *ulMin, unsigned char *ulSec, unsigned char *ulMinSec, unsigned long ulRegData))func_UTCTimerGet)
#define ROM_UTCTimerGetBy10ms                                   ((unsigned long (*)(void))func_UTCTimerGetBy10ms)
#define ROM_UTCTimerAlarmSet                                    ((void (*)(unsigned long ulAMPM, unsigned long ulHour, unsigned long ulMin, unsigned long ulSec, unsigned long ulMS))func_UTCTimerAlarmSet)
#define ROM_UTCTimerAlarmGet                                    ((void (*)(unsigned long *ulAMPM, unsigned long *ulHour, unsigned long *ulMin, unsigned long *ulSec, unsigned long *ulMS))func_UTCTimerAlarmGet)
#define ROM_UTCTimerAlarmSetBy10ms                              ((void (*)(unsigned long ulMinSec))func_UTCTimerAlarmSetBy10ms)
#define ROM_UTCCalSet                                           ((void (*)(unsigned long ulCentury, unsigned long ulYear, unsigned long ulMonth, unsigned long ulDate, unsigned long ulDay))func_UTCCalSet)
#define ROM_UTCCalChangeGet                                     ((unsigned long (*)(void))func_UTCCalChangeGet)
#define ROM_UTCCalGet                                           ((void (*)(unsigned char *ulCentury, unsigned char *ulYear, unsigned char *ulMonth, unsigned char *ulDate, unsigned char *ulDay, unsigned long ulRegData))func_UTCCalGet)
#define ROM_UTCCalAlarmSet                                      ((void (*)(unsigned long ulMonth, unsigned long ulDate))func_UTCCalAlarmSet)
#define ROM_UTCCalAlarmGet                                      ((void (*)(unsigned char *ulMonth, unsigned long *ulDate))func_UTCCalAlarmGet)
#define ROM_UTCAlarmEnable                                      ((void (*)(unsigned long ulAlarmFlags))func_UTCAlarmEnable)
#define ROM_UTCAlarmDisable                                     ((void (*)(unsigned long ulAlarmFlags))func_UTCAlarmDisable)
#define ROM_UTCIntEnable                                        ((void (*)(unsigned long ulIntFlags))func_UTCIntEnable)
#define ROM_UTCIntDisable                                       ((void (*)(unsigned long ulIntFlags))func_UTCIntDisable)
#define ROM_UTCIntMaskSet                                       ((void (*)(unsigned long ulIntMask))func_UTCIntMaskSet)
#define ROM_UTCIntMaskGet                                       ((unsigned long (*)(void))func_UTCIntMaskGet)
#define ROM_UTCIntStatusSet                                     ((void (*)(unsigned long ulIntFlags))func_UTCIntStatusSet)
#define ROM_UTCIntStatusGet                                     ((unsigned long (*)(void))func_UTCIntStatusGet)
#define ROM_UTCValidStatusGet                                   ((unsigned long (*)(void))func_UTCValidStatusGet)
#define ROM_UTCKeepRun                                          ((void (*)(unsigned long ulKeepUTC))func_UTCKeepRun)
#define ROM_UTCClkCntSet                                        ((void (*)(unsigned long ulClkCnt))func_UTCClkCntSet)
#define ROM_UTCClkCntGet                                        ((unsigned long (*)(unsigned long ulRegData))func_UTCClkCntGet)
#define ROM_UTCIntRegister                                      ((void (*)(unsigned long *g_pRAMVectors, void (*pfnHandler)(void)))func_UTCIntRegister)
#define ROM_UTCIntUnregister                                    ((void (*)(unsigned long *g_pRAMVectors))func_UTCIntUnregister)
#define ROM_UTC32768Sel                                         ((void (*)(unsigned char ucMode))func_UTC32768Sel)
#define ROM_UTC32768Get                                         ((unsigned char (*)(void))func_UTC32768Get)
#define ROM_UTCAlarmCntCheckEnable                              ((void (*)(void))func_UTCAlarmCntCheckEnable)
#define ROM_UTCAlarmCntCheckDisable                             ((void (*)(void))func_UTCAlarmCntCheckDisable)
#define ROM_UTCAlarmCntCFG                                      ((void (*)(unsigned long ulCnt))func_UTCAlarmCntCFG)
#define ROM_UTCAlarmCntGet                                      ((unsigned long (*)(void))func_UTCAlarmCntGet)
#define ROM_UTCClkCntConvert                                    ((unsigned long (*)(unsigned long ulRegData))func_UTCClkCntConvert)
#define ROM_UTCWDTCtrlConfig                                    ((void (*)(unsigned long ulConfig))func_UTCWDTCtrlConfig)
#define ROM_UTCWDTEnable                                        ((void (*)(void))func_UTCWDTEnable)
#define ROM_UTCWDTDisable                                       ((void (*)(void))func_UTCWDTDisable)
#define ROM_UTCWDTLongTimerDataValid                            ((void (*)(void))func_UTCWDTLongTimerDataValid)
#define ROM_UTCWDTLongTimerDataInvalid                          ((void (*)(void))func_UTCWDTLongTimerDataInvalid)
#define ROM_UTCWDTTickConfig                                    ((void (*)(unsigned char ucAccuracy))func_UTCWDTTickConfig)
#define ROM_UTCWDTLongTimerDataSet                              ((void (*)(unsigned long ulTime))func_UTCWDTLongTimerDataSet)
#define ROM_UTCWDTShortTimerDataSet                             ((void (*)(unsigned char ucStartValue))func_UTCWDTShortTimerDataSet)
#define ROM_UTCWDTStartAfterWakeupSet                           ((void (*)(unsigned char ucActive))func_UTCWDTStartAfterWakeupSet)
#define ROM_UTCWDTCalendarDataSet                               ((void (*)(unsigned long ulCalendar))func_UTCWDTCalendarDataSet)
#define ROM_UTCWDTIntStatusGet                                  ((unsigned long (*)(void))func_UTCWDTIntStatusGet)
#define ROM_UTCWDTClearInt                                      ((void (*)(unsigned char ucInt))func_UTCWDTClearInt)
#define ROM_WatchdogRunning                                     ((unsigned char (*)(unsigned long ulBase))func_WatchdogRunning)
#define ROM_WatchdogEnable                                      ((void (*)(unsigned long ulBase))func_WatchdogEnable)
#define ROM_WatchdogDisable                                     ((void (*)(unsigned long ulBase))func_WatchdogDisable)
#define ROM_WatchdogResetEnable                                 ((void (*)(unsigned long ulBase))func_WatchdogResetEnable)
#define ROM_WatchdogResetDisable                                ((void (*)(unsigned long ulBase))func_WatchdogResetDisable)
#define ROM_WatchdogTimerRepeatEnable                           ((void (*)(unsigned long ulBase))func_WatchdogTimerRepeatEnable)
#define ROM_WatchdogTimerRepeatDisable                          ((void (*)(unsigned long ulBase))func_WatchdogTimerRepeatDisable)
#define ROM_WatchdogReloadSet                                   ((void (*)(unsigned long ulBase, unsigned long ulLoadVal))func_WatchdogReloadSet)
#define ROM_WatchdogValueGet                                    ((unsigned short (*)(unsigned long ulBase))func_WatchdogValueGet)
#define ROM_WatchdogIntRegister                                 ((void (*)(unsigned long *g_pRAMVectors, void(*pfnHandler)(void)))func_WatchdogIntRegister)
#define ROM_WatchdogIntUnregister                               ((void (*)(unsigned long *g_pRAMVectors))func_WatchdogIntUnregister)
#define ROM_WatchdogIntEnable                                   ((void (*)(unsigned long ulBase))func_WatchdogIntEnable)
#define ROM_WatchdogIntMaskedGet                                ((unsigned long (*)(unsigned long ulBase))func_WatchdogIntMaskedGet)
#define ROM_WatchdogReadClearInt                                ((unsigned long (*)(unsigned long ulBase))func_WatchdogReadClearInt)


#endif

#endif // __ROM_H__
