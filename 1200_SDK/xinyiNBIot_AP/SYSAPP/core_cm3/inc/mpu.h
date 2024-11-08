#ifndef __MPU_H__
#define __MPU_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "xinyi2100.h"

#if (__MPU_PRESENT == 1U)

/*!< MPU type */
#define MPU_REGION_TYPE_IREGION                                  (0x0UL)
#define MPU_REGION_TYPE_DREGION                                  (0x1UL)
#define MPU_REGION_TYPE_SEPARATE                                 (0x2UL)
#define IS_MPU_REGION_TYPE(TYPE) ((TYPE) == MPU_REGION_TYPE_IREGION || \
                                  (TYPE) == MPU_REGION_TYPE_DREGION || \
                                  (TYPE) == MPU_REGION_TYPE_SEPARATE)

/*!< MPU control */
#define MPU_CONTROL_PRIVILEGED_DEFAULT_MEMORY_MAP                (MPU_CTRL_PRIVDEFENA_Msk)
#define MPU_CONTROL_HARDFAULT_NMI_USE_MPU                        (MPU_CTRL_HFNMIENA_Msk)


/*!< MPU region number group */
#define MPU_REGION_NUMBER_0                                      (0x00000000UL)
#define MPU_REGION_NUMBER_1                                      (0x00000001UL)
#define MPU_REGION_NUMBER_2                                      (0x00000002UL)
#define MPU_REGION_NUMBER_3                                      (0x00000003UL)
#define MPU_REGION_NUMBER_4                                      (0x00000004UL)
#define MPU_REGION_NUMBER_5                                      (0x00000005UL)
#define MPU_REGION_NUMBER_6                                      (0x00000006UL)
#define MPU_REGION_NUMBER_7                                      (0x00000007UL)
#define IS_MPU_REGION_NUMBER(REGIONNUM) ((REGIONNUM) <= MPU_REGION_NUMBER_7)

/*!<!< MPU region length group */
#define MPU_REGION_LENGTH_32B                                    (5 - 1)
#define MPU_REGION_LENGTH_64B                                    (6 - 1)
#define MPU_REGION_LENGTH_128B                                   (7 - 1)
#define MPU_REGION_LENGTH_256B                                   (8 - 1)
#define MPU_REGION_LENGTH_512B                                   (9 - 1)
#define MPU_REGION_LENGTH_1K                                     (10 - 1)
#define MPU_REGION_LENGTH_2K                                     (11 - 1)
#define MPU_REGION_LENGTH_4K                                     (12 - 1)
#define MPU_REGION_LENGTH_8K                                     (13 - 1)
#define MPU_REGION_LENGTH_16K                                    (14 - 1)
#define MPU_REGION_LENGTH_32K                                    (15 - 1)
#define MPU_REGION_LENGTH_64K                                    (16 - 1)
#define MPU_REGION_LENGTH_128K                                   (17 - 1)
#define MPU_REGION_LENGTH_256K                                   (18 - 1)
#define MPU_REGION_LENGTH_512K                                   (19 - 1)
#define MPU_REGION_LENGTH_1M                                     (20 - 1)
#define MPU_REGION_LENGTH_2M                                     (21 - 1)
#define MPU_REGION_LENGTH_4M                                     (22 - 1)
#define MPU_REGION_LENGTH_8M                                     (23 - 1)
#define MPU_REGION_LENGTH_16M                                    (24 - 1)
#define MPU_REGION_LENGTH_32M                                    (25 - 1)
#define MPU_REGION_LENGTH_64M                                    (26 - 1)
#define MPU_REGION_LENGTH_128M                                   (27 - 1)
#define MPU_REGION_LENGTH_256M                                   (28 - 1)
#define MPU_REGION_LENGTH_512M                                   (29 - 1)
#define MPU_REGION_LENGTH_1G                                     (30 - 1)
#define MPU_REGION_LENGTH_2G                                     (31 - 1)
#define MPU_REGION_LENGTH_4G                                     (32 - 1)
#define IS_MPU_REGION_LENGTH(REGIONLEN) ((REGIONLEN) >= MPU_REGION_LENGTH_32B && (REGIONLEN) <= MPU_REGION_LENGTH_4G)

/*!< MPU region config XN group, enable or disable instruction access */
#define MPU_REGION_CONFIG_XN_INSTRUCTION_ACCESS_ENABLE           (0x0UL << MPU_RASR_XN_Pos)
#define MPU_REGION_CONFIG_XN_INSTRUCTION_ACCESS_DISABLE          (0x1UL << MPU_RASR_XN_Pos)
/*!< MPU region config AP group, access permission of privileged and user */
#define MPU_REGION_CONFIG_AP_NO_ACCESS                           (0x0UL << MPU_RASR_AP_Pos)
#define MPU_REGION_CONFIG_AP_PRIVILEGED_ACCESS_ONLY              (0x1UL << MPU_RASR_AP_Pos)
#define MPU_REGION_CONFIG_AP_PRIVILEGED_ACCESS_USER_READ_ONLY    (0x2UL << MPU_RASR_AP_Pos)
#define MPU_REGION_CONFIG_AP_FULL_ACCESS                         (0x3UL << MPU_RASR_AP_Pos)
#define MPU_REGION_CONFIG_AP_PRIVILEGED_READ_ONLY                (0x5UL << MPU_RASR_AP_Pos)
#define MPU_REGION_CONFIG_AP_READ_ONLY                           (0x6UL << MPU_RASR_AP_Pos)
/*!< MPU region config AP group, access permission of privileged and user */
#define MPU_REGION_CONFIG_TEX_S_C_B_STRONGLY_ORDERED_SHARED                               ((0x0 << MPU_RASR_TEX_Pos) | (0x0 << MPU_RASR_C_Pos) | (0x0 << MPU_RASR_B_Pos) | (0x0 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_SHARED_DEVICE_SHARED                                  ((0x0 << MPU_RASR_TEX_Pos) | (0x0 << MPU_RASR_C_Pos) | (0x1 << MPU_RASR_B_Pos) | (0x0 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_OUTER_INNER_WRITETHROUGH_NO_WRITE_ALLOCATE            ((0x0 << MPU_RASR_TEX_Pos) | (0x1 << MPU_RASR_C_Pos) | (0x0 << MPU_RASR_B_Pos) | (0x0 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_OUTER_INNER_WRITETHROUGH_NO_WRITE_ALLOCATE_SHARED     ((0x0 << MPU_RASR_TEX_Pos) | (0x1 << MPU_RASR_C_Pos) | (0x0 << MPU_RASR_B_Pos) | (0x1 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_OUTER_INNER_WRITEBACK_NO_WRITE_ALLOCATE               ((0x0 << MPU_RASR_TEX_Pos) | (0x1 << MPU_RASR_C_Pos) | (0x1 << MPU_RASR_B_Pos) | (0x0 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_OUTER_INNER_WRITEBACK_NO_WRITE_ALLOCATE_SHARED        ((0x0 << MPU_RASR_TEX_Pos) | (0x1 << MPU_RASR_C_Pos) | (0x1 << MPU_RASR_B_Pos) | (0x1 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_OUTER_INNER_WRITEBACK_NO_CACHEABLE                    ((0x1 << MPU_RASR_TEX_Pos) | (0x0 << MPU_RASR_C_Pos) | (0x0 << MPU_RASR_B_Pos) | (0x0 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_OUTER_INNER_WRITEBACK_NO_CACHEABLE_SHARED             ((0x1 << MPU_RASR_TEX_Pos) | (0x0 << MPU_RASR_C_Pos) | (0x0 << MPU_RASR_B_Pos) | (0x1 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_IMPLEMENTATION_DEFINED                                ((0x1 << MPU_RASR_TEX_Pos) | (0x1 << MPU_RASR_C_Pos) | (0x0 << MPU_RASR_B_Pos) | (0x0 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_OUTER_INNER_WRITEBACK_WRITE_READ_ALLOCATE             ((0x1 << MPU_RASR_TEX_Pos) | (0x1 << MPU_RASR_C_Pos) | (0x1 << MPU_RASR_B_Pos) | (0x0 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_OUTER_INNER_WRITEBACK_WRITE_READ_ALLOCATE_SHARED      ((0x1 << MPU_RASR_TEX_Pos) | (0x1 << MPU_RASR_C_Pos) | (0x1 << MPU_RASR_B_Pos) | (0x1 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_NONSHARED_DEVICE                                      ((0x2 << MPU_RASR_TEX_Pos) | (0x0 << MPU_RASR_C_Pos) | (0x0 << MPU_RASR_B_Pos) | (0x0 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_CACHE_MEMORY_NONCACHEABLE                             ((0x4 << MPU_RASR_TEX_Pos) | (0x0 << MPU_RASR_C_Pos) | (0x0 << MPU_RASR_B_Pos) | (0x0 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_CACHE_MEMORY_NONCACHEABLE_SHARED                      ((0x4 << MPU_RASR_TEX_Pos) | (0x0 << MPU_RASR_C_Pos) | (0x0 << MPU_RASR_B_Pos) | (0x1 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_CACHE_MEMORY_WRITEBACK_WRITE_READ_ALLOCATE            ((0x5 << MPU_RASR_TEX_Pos) | (0x0 << MPU_RASR_C_Pos) | (0x1 << MPU_RASR_B_Pos) | (0x0 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_CACHE_MEMORY_WRITEBACK_WRITE_READ_ALLOCATE_SHARED     ((0x5 << MPU_RASR_TEX_Pos) | (0x0 << MPU_RASR_C_Pos) | (0x1 << MPU_RASR_B_Pos) | (0x1 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_CACHE_MEMORY_WRITETHROUGH_NO_WRITE_ALLOCATE           ((0x6 << MPU_RASR_TEX_Pos) | (0x1 << MPU_RASR_C_Pos) | (0x0 << MPU_RASR_B_Pos) | (0x0 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_CACHE_MEMORY_WRITETHROUGH_NO_WRITE_ALLOCATE_SHARED    ((0x6 << MPU_RASR_TEX_Pos) | (0x1 << MPU_RASR_C_Pos) | (0x0 << MPU_RASR_B_Pos) | (0x1 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_CACHE_MEMORY_WRITEBACK_NO_WRITE_ALLOCATE              ((0x7 << MPU_RASR_TEX_Pos) | (0x1 << MPU_RASR_C_Pos) | (0x1 << MPU_RASR_B_Pos) | (0x0 << MPU_RASR_S_Pos))
#define MPU_REGION_CONFIG_TEX_S_C_B_CACHE_MEMORY_WRITEBACK_NO_WRITE_ALLOCATE_SHARED       ((0x7 << MPU_RASR_TEX_Pos) | (0x1 << MPU_RASR_C_Pos) | (0x1 << MPU_RASR_B_Pos) | (0x1 << MPU_RASR_S_Pos))

/*!< MPU region config SRD group, disable subregion or not */
#define MPU_SUB_REGION_NUMBER_0                                  (0x01UL << MPU_RASR_SRD_Pos)
#define MPU_SUB_REGION_NUMBER_1                                  (0x02UL << MPU_RASR_SRD_Pos)
#define MPU_SUB_REGION_NUMBER_2                                  (0x04UL << MPU_RASR_SRD_Pos)
#define MPU_SUB_REGION_NUMBER_3                                  (0x08UL << MPU_RASR_SRD_Pos)
#define MPU_SUB_REGION_NUMBER_4                                  (0x10UL << MPU_RASR_SRD_Pos)
#define MPU_SUB_REGION_NUMBER_5                                  (0x20UL << MPU_RASR_SRD_Pos)
#define MPU_SUB_REGION_NUMBER_6                                  (0x40UL << MPU_RASR_SRD_Pos)
#define MPU_SUB_REGION_NUMBER_7                                  (0x80UL << MPU_RASR_SRD_Pos)
#define MPU_SUB_REGION_ALL                                       (0xFFUL << MPU_RASR_SRD_Pos)


uint32_t MPU_GetNumberOfRegion(uint32_t RegionType);
void MPU_Control(uint32_t Contrl, FunctionalState NewState);
void MPU_Cmd(FunctionalState NewState);
void MPU_RegionInit(uint32_t RegionNum, uint32_t RegionAddr, uint32_t RegionLen, uint32_t Config);
void MPU_SubregionCmd(uint32_t RegionNum, uint32_t SubRegionNum, FunctionalState NewState);


#endif  /* __MPU_PRESENT == 1U */

#ifdef __cplusplus
 }
#endif

#endif  /* __MPU_H__ */
