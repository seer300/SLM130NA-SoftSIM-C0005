#ifndef __HW_CRYPTO_H__
#define __HW_CRYPTO_H__

#include "hw_memmap.h"

//*****************************************************************************
//
// The following are defines for the AES register offsets.
//
//*****************************************************************************
#define REG_AES_CTL                     (0x00 + AES_BASE)
#define REG_AES_STA                     (0x04 + AES_BASE)
#define REG_AES_DONE                    (0x05 + AES_BASE)
#define REG_SZ_STA                      (0x06 + AES_BASE)
#define REG_AES_KEY0                    (0x08 + AES_BASE)
#define REG_AES_IV0                     (0x28 + AES_BASE)
#define REG_AES_AADLEN0                 (0x38 + AES_BASE)
#define REG_AES_AADLEN1                 (0x3C + AES_BASE)
#define REG_AES_PLEN                    (0x40 + AES_BASE)
#define REG_AES_TAG0                    (0x44 + AES_BASE)


//*****************************************************************************
//
// The following are defines for the bit fields in the REG_AES_CTL register.
//
//*****************************************************************************
#define AES_CTL_START_Pos               0
#define AES_CTL_START                   (1UL << AES_CTL_START_Pos)
#define AES_CTL_KEY_LOAD_Pos            1
#define AES_CTL_KEY_LOAD                (1UL << AES_CTL_KEY_LOAD_Pos)
#define AES_CTL_EIA_Pos                 6
#define AES_CTL_EIA_Msk                 (1UL << AES_CTL_EIA_Pos)
#define AES_CTL_UIA_Pos                 7
#define AES_CTL_UIA_Msk                 (1UL << AES_CTL_UIA_Pos)
#define AES_CTL_KEY_LEN_Pos             8
#define AES_CTL_KEY_LEN_Msk             (3UL << AES_CTL_KEY_LEN_Pos)
#define AES_CTL_KEY_LEN_128             (0UL << AES_CTL_KEY_LEN_Pos)
#define AES_CTL_KEY_LEN_192             (1UL << AES_CTL_KEY_LEN_Pos)
#define AES_CTL_KEY_LEN_256             (2UL << AES_CTL_KEY_LEN_Pos)
#define AES_CTL_ENC_DEC_Pos             10
#define AES_CTL_ENC_DEC_Msk             (1UL << AES_CTL_ENC_DEC_Pos)
#define AES_CTL_ENC                     (1UL << AES_CTL_ENC_DEC_Pos)
#define AES_CTL_DEC                     (0UL << AES_CTL_ENC_DEC_Pos)
#define AES_CTL_MODE_Pos                11
#define AES_CTL_MODE_Msk                (7UL << AES_CTL_MODE_Pos)
#define AES_CTL_MODE_ECB                (0UL << AES_CTL_MODE_Pos)
#define AES_CTL_MODE_CBC                (1UL << AES_CTL_MODE_Pos)
#define AES_CTL_MODE_CFB                (2UL << AES_CTL_MODE_Pos)
#define AES_CTL_MODE_OFB                (3UL << AES_CTL_MODE_Pos)
#define AES_CTL_MODE_CTR                (4UL << AES_CTL_MODE_Pos)
#define AES_CTL_MODE_CCM                (5UL << AES_CTL_MODE_Pos)
#define AES_CTL_CCM_ADATA_LEN_Pos       16
#define AES_CTL_CCM_ADATA_LEN_Msk       (7UL << AES_CTL_CCM_ADATA_LEN_Pos)		// ccm_t = (tlen - 2)/2
#define AES_CTL_CCM_LOAD_LEN_Pos        19
#define AES_CTL_CCM_LOAD_LEN_Msk        (7UL << AES_CTL_CCM_LOAD_LEN_Pos)		// ccm_q = 15 - nlen - 1
#define AES_CTL_DMA_EN_Pos              22
#define AES_CTL_DMA_EN                  (1UL << AES_CTL_DMA_EN_Pos)
#define AES_CTL_FORCE_CLK_EN_Pos        23
#define AES_CTL_FORCE_CLK_EN            (1UL << AES_CTL_FORCE_CLK_EN_Pos)
#define AES_CTL_CLK_DIV_Pos             24
#define AES_CTL_CLK_DIV_Msk             (1UL << AES_CTL_CLK_DIV_Pos)			// 1, AES core clock divided by 2, for timing
#define AES_CTL_CLK_DIV_EN              (1UL << AES_CTL_CLK_DIV_Pos)
#define AES_CTL_INPUT_ENDIAN_Pos        25
#define AES_CTL_INPUT_ENDIAN_Msk        (1UL << AES_CTL_INPUT_ENDIAN_Pos)
#define AES_CTL_INPUT_ENDIAN_SMALK      (0UL << AES_CTL_INPUT_ENDIAN_Pos)
#define AES_CTL_INPUT_ENDIAN_BIG        (1UL << AES_CTL_INPUT_ENDIAN_Pos)
#define AES_CTL_OUTPUT_ENDIAN_Pos       26
#define AES_CTL_OUTPUT_ENDIAN_Msk       (1UL << AES_CTL_OUTPUT_ENDIAN_Pos)
#define AES_CTL_OUTPUT_ENDIAN_SMALK     (0UL << AES_CTL_OUTPUT_ENDIAN_Pos)
#define AES_CTL_OUTPUT_ENDIAN_BIG       (1UL << AES_CTL_OUTPUT_ENDIAN_Pos)
#define AES_CTL_SZ_SEL_Pos              27
#define AES_CTL_SZ_SEL_Msk              (3UL << AES_CTL_SZ_SEL_Pos)
#define AES_CTL_SZ_SEL_AES              (0UL << AES_CTL_SZ_SEL_Pos)
#define AES_CTL_SZ_SEL_ZUC              (1UL << AES_CTL_SZ_SEL_Pos)
#define AES_CTL_SZ_SEL_S3G              (3UL << AES_CTL_SZ_SEL_Pos)
#define AES_CTL_KEY_ENDIAN_Pos          29
#define AES_CTL_KEY_ENDIAN_Msk          (1UL << AES_CTL_KEY_ENDIAN_Pos)
#define AES_CTL_KEY_ENDIAN_SMALL        (0UL << AES_CTL_KEY_ENDIAN_Pos)
#define AES_CTL_KEY_ENDIAN_BIG          (1UL << AES_CTL_KEY_ENDIAN_Pos)
#define AES_CTL_IV_ENDIAN_Pos           30
#define AES_CTL_IV_ENDIAN_Msk           (1UL << AES_CTL_IV_ENDIAN_Pos)
#define AES_CTL_IV_ENDIAN_SMALL         (0UL << AES_CTL_IV_ENDIAN_Pos)
#define AES_CTL_IV_ENDIAN_BIG           (1UL << AES_CTL_IV_ENDIAN_Pos)
#define AES_CTL_S3G_ZUC_ENDIAN_Pos      31
#define AES_CTL_S3G_ZUC_ENDIAN_Msk      (1UL << AES_CTL_S3G_ZUC_ENDIAN_Pos)
#define AES_CTL_S3G_ZUC_ENDIAN_BIG      (0UL << AES_CTL_S3G_ZUC_ENDIAN_Pos)
#define AES_CTL_S3G_ZUC_ENDIAN_SMALL    (1UL << AES_CTL_S3G_ZUC_ENDIAN_Pos)

//*******************************************************************************
//
// The following are defines for the bit fields in the BYTE REG_AES_STA register.
//
//*******************************************************************************
#define AES_STA_AES_STA_Pos           	0										// 1, AES done
#define AES_STA_AES_STA_Msk				(1UL << AES_STA_AES_STA_Pos)
#define AES_STA_AES_STA_DONE			(1UL << AES_STA_AES_STA_Pos)


//********************************************************************************
//
// The following are defines for the bit fields in the BYTE REG_AES_DONE register.
//
//********************************************************************************
#define AES_DONE_KEY_DONE_Pos			0
#define AES_DONE_KEY_DONE_Msk			(1UL << AES_DONE_KEY_DONE_Pos)			// key load done, software read clear
#define AES_DONE_KEY_DONE				(1UL << AES_DONE_KEY_DONE_Pos)


//********************************************************************************
//
// The following are defines for the bit fields in the BYTE REG_SZ_STA register.
//
//********************************************************************************
#define SZ_STA_S3G_ZUC_STATUS_Pos		0
#define SZ_STA_S3G_ZUC_STATUS_Msk		(1UL << SZ_STA_S3G_ZUC_STATUS_Pos)
#define SZ_STA_S3G_ZUC_DONE				(1UL << SZ_STA_S3G_ZUC_STATUS_Pos)


//*****************************************************************************
//
// The following are defines for the SHA register offsets.
//
//*****************************************************************************
#define REG_SHA_LEN                     (0x00 + SHA_BASE)
#define REG_SHA_CTRL					(0x04 + SHA_BASE)
#define REG_SHA_START					(0x05 + SHA_BASE)
#define REG_SHA_INTR_CLR				(0x06 + SHA_BASE)
#define REG_SHA_STAT					(0x07 + SHA_BASE)
#define REG_SHA_RESULT					(0x08 + SHA_BASE)
#define REG_SHA_DATA_IN					(0x20 + SHA_BASE)

//********************************************************************************
//
// The following are defines for the bit fields in the REG_SHA_CTRL register.
//
//********************************************************************************
#define SHA_CTRL_DMA_EN_Pos				0
#define SHA_CTRL_DMA_EN_Msk				(1UL << SHA_CTRL_DMA_EN_Pos)
#define SHA_CTRL_DMA_EN					(1UL << SHA_CTRL_DMA_EN_Pos)
#define SHA_CTRL_INTR_EN_Pos			1
#define SHA_CTRL_INTR_EN_Msk			(1UL << SHA_CTRL_INTR_EN_Pos)
#define SHA_CTRL_INTR_EN				(1UL << SHA_CTRL_INTR_EN_Pos)
#define	SHA_CTRL_CLK_MODE_Pos			2
#define	SHA_CTRL_CLK_MODE_Msk			(1UL << SHA_CTRL_CLK_MODE_Pos)
#define	SHA_CTRL_CLK_MODE				(1UL << SHA_CTRL_CLK_MODE_Pos)
#define	SHA_CTRL_INPUT_ENDIAN_Pos		3
#define	SHA_CTRL_INPUT_ENDIAN_Msk	    (1UL << SHA_CTRL_INPUT_ENDIAN_Pos)
#define	SHA_CTRL_INPUT_ENDIAN_BIG		(1UL << SHA_CTRL_INPUT_ENDIAN_Pos)
#define	SHA_CTRL_INPUT_ENDIAN_SMALL		(1UL << SHA_CTRL_INPUT_ENDIAN_Pos)

//********************************************************************************
//
// The following are defines for the bit fields in the REG_SHA_START register.
//
//********************************************************************************
#define	SHA_START_Pos					0
#define SHA_START_Msk					(1 << SHA_START_Pos)
#define SHA_START						(1 << SHA_START_Pos)

//********************************************************************************
//
// The following are defines for the bit fields in the REG_SHA_INTR_CLR register.
//
//********************************************************************************
#define	SHA_FIN_INTR_CLR_Pos			0
#define SHA_FIN_INTR_CLR_Msk			(1 << SHA_FIN_INTR_CLR_Pos)
#define SHA_FIN_INTR_CLR				(1 << SHA_FIN_INTR_CLR_Pos)

//********************************************************************************
//
// The following are defines for the bit fields in the REG_SHA_STAT register.
//
//********************************************************************************
#define	SHA_STAT_ALL_FIN_Pos			0
#define SHA_STAT_ALL_FIN_Msk			(1 << SHA_STAT_ALL_FIN_Pos)
#define SHA_STAT_ALL_FIN				(1 << SHA_STAT_ALL_FIN_Pos)
#define	SHA_STAT_BLOCK_FIN_Pos			1
#define SHA_STAT_BLOCK_FIN_Msk			(1 << SHA_STAT_BLOCK_FIN_Pos)
#define SHA_STAT_BLOCK_FIN				(1 << SHA_STAT_BLOCK_FIN_Pos)


#endif // __HW_AES_H__
