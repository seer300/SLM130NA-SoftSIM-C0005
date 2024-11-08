/*****************************************************************************************************************************	 
 * user_adc.c
 ****************************************************************************************************************************/

#include "user_debug.h"
#include "mcu_adapt.h"
#include "user_adc.h"
#include "vmcu.h"
#include "adc.h"
#include "type_adapt.h"

__TYPE_IRQ_FUNC uint16_t UserMainPowerDetect(void)
{
    uint16_t main_power_value;

    DisablePrimask();
    VmcuGpioModeSet(MAIN_POWER_CTL_PIN, 0x00);  // GPIO12控制ADC2通道采集的信号的关断 
    VmcuGpioWrite(MAIN_POWER_CTL_PIN, 1); 
    main_power_value = VmcuAdcRead(ADC_AUX_ADC2);
    VmcuGpioWrite(MAIN_POWER_CTL_PIN, 0);
    EnablePrimask();	

    return main_power_value;
}

static uint16_t UserBackPowerDetect(void)
{
    uint16_t back_power_value = 0;

    DisablePrimask();
    VmcuGpioModeSet(BACK_POWER_CTL_PIN, 0x00);  // GPIO11控制ADC1通道采集的信号的关断 
    VmcuGpioWrite(BACK_POWER_CTL_PIN, 1); 

    for (uint32_t i=0; i<BAK_POWER_TETECT_CNT_PER_MIN; i++)
    {
        back_power_value += VmcuAdcRead(ADC_AUX_ADC1);
    }
    back_power_value /= BAK_POWER_TETECT_CNT_PER_MIN;

    VmcuGpioWrite(BACK_POWER_CTL_PIN, 0);
    EnablePrimask();

    return back_power_value;
}

static uint16_t UserLightingSampleDetect(void)
{
    uint16_t lignt_sample_value;

    DisablePrimask();
    VmcuGpioModeSet(LIGHTING_SAMPLE_CTL_PIN, 0x00);  // GPIO13输出2v电压供adc采样
    VmcuGpioWrite(LIGHTING_SAMPLE_CTL_PIN, 1);
    lignt_sample_value = VmcuAdcRead(ADC_CMP_INP);
    VmcuGpioWrite(LIGHTING_SAMPLE_CTL_PIN, 0);
    EnablePrimask();

    return lignt_sample_value;
}

void UserAdcFunc(void)
{
    UserMainPowerDetect();
    
    UserLightingSampleDetect();

    if(is_event_set(BAK_POWER_DETECT_EVENT))
    {
        clear_event(BAK_POWER_DETECT_EVENT);
        UserBackPowerDetect();
    }
}