/*****************************************************************************************************************************	 
 * user_adc.h
 ****************************************************************************************************************************/

#ifndef USER_ADC_H__
#define USER_ADC_H__

#include "basic_config.h"

#define BAK_POWER_DETECT_EVENT          EVENT_USER_DEFINE4
#define BAK_POWER_TETECT_CNT_PER_MIN    (6)
#define MAIN_POWER_CTL_PIN              (MCU_GPIO12)
#define BACK_POWER_CTL_PIN              (MCU_GPIO11)
#define LIGHTING_SAMPLE_CTL_PIN         (MCU_GPIO13)

extern uint16_t UserMainPowerDetect(void);
extern void UserAdcFunc(void);

#endif

