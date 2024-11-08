/*****************************************************************************************************************************	 
 * user_lcd.h
 ****************************************************************************************************************************/

#ifndef USER_LCD_H__
#define USER_LCD_H__

typedef enum {
    USER_LCD_MENU1 = 0,
    USER_LCD_MENU2,
    USER_LCD_MENU3,
    USER_LCD_MENU4,
    USER_LCD_MENU5
} e_lcd_menu;

extern void UserGetTime(void);
extern void UserLcdMenuSwitch(void);
extern void UserLcdShowSend(void);
extern void UserLcdFunc(void);

#endif

