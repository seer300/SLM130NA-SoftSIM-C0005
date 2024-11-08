/**
 ******************************************************************************
 * @file    hal_aes.h
 * @brief   此文件包含aes外设的变量，枚举，结构体定义，函数声明等.
 *
 ******************************************************************************
 * @attention
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 * http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

#pragma once
#include "hal_def.h"
#include "crypto.h"
#include "sema.h"
#include "prcm.h"

/**
 * @brief  AES工作状态枚举
 */
typedef enum
{
    HAL_AES_STATE_READY = 0x00U,   /*!< AES已就绪 */
    HAL_AES_STATE_BUSY = 0x01U     /*!< AES正被占用 */
} HAL_AES_StateTypeDef;

/**
 * @brief  AES工作模式枚举
 */
typedef enum
{
    HAL_AES_NONE_MODE = 0x00U,   /*!< AES 无工作模式 */
    HAL_AES_ECB_MODE = 0x01U,    /*!< AES ECB模式 */
    HAL_AES_CBC_MODE = 0x02U,    /*!< AES CBC模式 */
    HAL_AES_CFB_MODE = 0x04U,    /*!< AES CFB模式 */
    HAL_AES_OFB_MODE = 0x08U,    /*!< AES OFB模式 */
    HAL_AES_CTR_MODE = 0x10U     /*!< AES CTR模式 */
} HAL_AES_WorkModeTypeDef;

/**
 * @brief  AES硬件锁枚举
 */
typedef enum
{
    HAL_AES_SEMA_NONE = 0x00U,   /*!< AES不使用硬件锁 */
    HAL_AES_SEMA_USE = 0x01U     /*!< AES使用硬件锁 */
} HAL_AES_SEMA_UseTypeDef;

/**
 * @brief  AES硬件锁请求优先级枚举
 */
typedef enum
{
    HAL_AES_SEMA_REQ_PRI_0 = SEMA_SEMA_REQ_PRI_0,   /*!< AES硬件锁请求优先级最低 */
    HAL_AES_SEMA_REQ_PRI_1 = SEMA_SEMA_REQ_PRI_1,   /*!< AES硬件锁请求优先级次低 */
    HAL_AES_SEMA_REQ_PRI_2 = SEMA_SEMA_REQ_PRI_2,   /*!< AES硬件锁请求优先级次高 */
    HAL_AES_SEMA_REQ_PRI_3 = SEMA_SEMA_REQ_PRI_3    /*!< AES硬件锁请求优先级次高 */
} HAL_AES_SEMA_ReqPriTypeDef;

/**
 * @brief  AES密钥长度枚举
 */
typedef enum
{
    HAL_AES_KEYLENBITS_128 = AES_KEY_LEN_BITS_128,   /*!< AES密钥长度为128bits */
    HAL_AES_KEYLENBITS_192 = AES_KEY_LEN_BITS_192,   /*!< AES密钥长度为192bits */
    HAL_AES_KEYLENBITS_256 = AES_KEY_LEN_BITS_256    /*!< AES密钥长度为256bits */
} HAL_AES_KeyLenBitsTypeDef;

/**
 * @brief  AES密钥结构体
 */
typedef struct
{
    uint8_t *pKey;                          /*!< 密钥存储地址 */
    HAL_AES_KeyLenBitsTypeDef KeyBitsLen;   /*!< 密钥长度（单位：比特） */
    uint8_t *pIV;                           /*!< 初始向量存储地址 */
} HAL_AES_Key_HandleTypeDef;

/**
 * @brief  AES初始化结构体
 */
typedef struct
{
    HAL_AES_WorkModeTypeDef WorkMode;       /*!< AES工作模式 */

    HAL_AES_SEMA_UseTypeDef UseSema;        /*!< AES硬件锁使用标识 */
    HAL_AES_SEMA_ReqPriTypeDef SemaReqPri;  /*!< AES硬件锁请求优先级 */

    HAL_LockTypeDef Lock;                   /*!< AES设备锁 */

    volatile HAL_AES_StateTypeDef State;    /*!< AES工作状态 */
} HAL_AES_Init_HandleTypeDef;

/**
 * @brief AES阻塞数据加密API
 * 
 * @param aes 详情参考 @ref HAL_AES_Init_HandleTypeDef.
 * @param key 详情参考 @ref HAL_AES_Key_HandleTypeDef.
 * @param pInput 明文存储地址
 * @param InputLenByte 明文长度
 * @param pOutput 密文存储地址
 * @return 函数执行状态. 详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_AES_Encrypt(HAL_AES_Init_HandleTypeDef *aes, HAL_AES_Key_HandleTypeDef *key, uint8_t *pInput, uint32_t InputLenByte, uint8_t *pOutput);

/**
 * @brief AES阻塞数据解密API
 * 
 * @param aes 详情参考 @ref HAL_AES_Init_HandleTypeDef.
 * @param key 详情参考 @ref HAL_AES_Key_HandleTypeDef.
 * @param pInput 密文存储地址
 * @param InputLenByte 密文长度
 * @param pOutput 明文存储地址
 * @return 函数执行状态. 详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_AES_Decrypt(HAL_AES_Init_HandleTypeDef *aes, HAL_AES_Key_HandleTypeDef *key, uint8_t *pInput, uint32_t InputLenByte, uint8_t *pOutput);
