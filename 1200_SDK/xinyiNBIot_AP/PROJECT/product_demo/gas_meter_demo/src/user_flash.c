/*****************************************************************************************************************************	 
 * user_flash.c
 ****************************************************************************************************************************/

#include "user_debug.h"
#include "xy_flash.h"
#include "type_adapt.h"
#include "user_flash.h"

__RAM_FUNC void UserFlashFunc(void)
{
    if(is_event_set(FLASH_WRITE_EVENT))
    {
        uint8_t wbuf1[512] = {0};

        for(int i = 0 ; i < 512; i++)
        {
            wbuf1[i] = (uint8_t)i;
        }

        if(xy_Flash_Write(USER_FLASH_BASE, wbuf1, 512) == false)
        {
            jk_printf("\r\nFS W FAIL\r\n");
            xy_assert(0);
        }
        else
        {
            jk_printf("\r\nFS W SUCCESS\r\n");
        }

        clear_event(FLASH_WRITE_EVENT);
    }
}