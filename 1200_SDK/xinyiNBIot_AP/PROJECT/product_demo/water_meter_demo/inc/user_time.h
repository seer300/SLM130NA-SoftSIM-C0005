#pragma once
#include "utc.h"

RTC_TimeTypeDef Time;
RTC_TimeTypeDef spi_test_time;
uint8_t g_eeprom_peri;
uint8_t g_send_peri;
uint8_t g_eeprom_buf[256];

void User_RTC_Refresh(void);