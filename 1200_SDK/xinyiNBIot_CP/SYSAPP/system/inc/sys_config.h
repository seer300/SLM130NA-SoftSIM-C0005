#pragma once

#define CLKTICK_FREQ32K_DIV 32
/**
 * @brief 从retention中获取Sys Div
 */
uint32_t Get_Sys_Div();

void Set_32K_Freq(unsigned int freq_32k);

/**
 * @brief 从retention中获取Peri1 Div
 */
uint32_t Get_Peri1_Div();

/**
 * @brief 从retention中获取Peri2 Div
 */
uint32_t Get_Peri2_Div();
