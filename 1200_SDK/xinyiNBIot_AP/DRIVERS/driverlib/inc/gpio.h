#ifndef __GPIO_H__
#define __GPIO_H__

#include "hw_gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct{
    GPIO_PadTypeDef Pin;
    GPIO_ModeTypeDef Mode;
    GPIO_RemapTypeDef PinRemap;
    GPIO_PullTypeDef Pull;
    GPIO_IntTypeDef Int;
    GPIO_DrvStrengthTypeDef DrvStrength;
} GPIO_InitTypeDef;

//*****************************************************************************
//
// Values that can be passed to GPIOModeSet as the ucMode parameter
//
//*****************************************************************************
#define GPIO_MODE_SW                    0x00000000 //software mode
#define GPIO_MODE_HW                    0x00000001 //hardware mode

//*****************************************************************************
//
// Values that can be passed to GPIOModeSet as the ucConfig parameter
//
//*****************************************************************************
#define GPIO_CTL_REGISTER               0x00000000 //register control
#define GPIO_CTL_PERIPHERAL             0x00000001 //periphral control

//*****************************************************************************
//
// Values that can be passed to GPIODirectionSet as the ucConfig parameter
//
//*****************************************************************************
#define GPIO_DIR_MODE_IN                0x00000000  // Pin is a GPIO input
#define GPIO_DIR_MODE_OUT               0x00000001  // Pin is a GPIO output
#define GPIO_DIR_MODE_INOUT             0x00000002  // Pin is a GPIO input and output

//*****************************************************************************
//
// Values that can be passed to GPIOIntTypeSet as the ucConfig parameter
//
//*****************************************************************************
#define GPIO_INT_EDGE                   0x00000000
#define GPIO_INT_LEVEL                  0x00000001

//*****************************************************************************
//
// Values that can be passed to GPIOIntEdgeSet as the ucConfig parameter
//
//*****************************************************************************
#define GPIO_INT_EDGE_SINGLE             0x00000000
#define GPIO_INT_EDGE_BOTH               0x00000001

//*****************************************************************************
//
// Values that can be passed to GPIOIntSingleEdgeSet as the ucConfig parameter
//
//*****************************************************************************
#define GPIO_INT_EDGE_RISE               0x00000000
#define GPIO_INT_EDGE_FALL               0x00000001

//*****************************************************************************
//
// The following values define the number for the PadValue argument to several
// of the APIs.
//
//*****************************************************************************



#define  GPIO_PER_MODE  0x00000010U
#define  GPIO_IO_MODE   0x00000020U

#define  GPIO_DIR_INPUT 0x00000001U
#define  GPIO_DIR_OD    0x00000002U
#define  GPIO_DIR_PP    0x00000004U
#define  GPIO_DIR_INOUT 0x00000008U



//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
/**
  * @brief  GPIO status enumeration
  */
typedef enum
{
	GPIO_PIN_RESET = 0,	    /*!< GPIO is low */
	GPIO_PIN_SET			/*!< GPIO is high */
} GPIO_PinState;

/**
 * @brief 用于GPIO_Init的初始化速度选择
 *        建议使用 FAST_SPEED
 */
typedef enum
{
    NORMAL_SPEED = 0,
    FAST_SPEED = 1
} GPIO_InitSpeedTypedef;

extern void GPIO_Init(GPIO_InitTypeDef *GPIO_InitStu, GPIO_InitSpeedTypedef speed);
extern void GPIO_AllocateRemove(GPIO_RemapTypeDef PeriNum);
extern void GPIO_WritePin(GPIO_PadTypeDef PadNum, GPIO_PinState PinState);
// extern GPIO_PinState GPIO_GetWritePin(GPIO_PadTypeDef PadNum);
// extern GPIO_PinState GPIO_ReadPin(GPIO_PadTypeDef PadNum);
extern void GPIO_InputPeriSelect(GPIO_PadTypeDef PadNum, GPIO_RemapTypeDef PeriNum);
extern void GPIO_InputPeriSelectCmd(GPIO_RemapTypeDef PeriNum, FunctionalState NewState);
extern void GPIO_InputPeriInvertCmd(uint8_t PeriNum, FunctionalState NewState);
extern uint8_t GPIO_ConflictStatusGet(GPIO_PadTypeDef PadNum);
extern uint8_t GPIO_AllocationStatusGet(GPIO_PadTypeDef PadNum);
extern void GPIO_PullUpCmd(GPIO_PadTypeDef PadNum, FunctionalState NewState);
extern void GPIO_PullDownCmd(GPIO_PadTypeDef PadNum, FunctionalState NewState);
extern void GPIO_InputCmd(GPIO_PadTypeDef PadNum, FunctionalState NewState);
extern void GPIO_OutputCmd(GPIO_PadTypeDef PadNum, FunctionalState NewState);
extern void Gpio_PullupConfig(GPIO_PadTypeDef *PadArray, uint8_t PadNum, FunctionalState NewState);
extern void Gpio_PulldownConfig(GPIO_PadTypeDef *PadArray, uint8_t PadNum, FunctionalState NewState);
extern void Gpio_InputConfig(GPIO_PadTypeDef *PadArray, uint8_t PadNum, FunctionalState NewState);
extern void Gpio_OutputConfig(GPIO_PadTypeDef *PadArray, uint8_t PadNum, FunctionalState NewState);
extern void GPIO_AnalogCmd(GPIO_PadTypeDef PadNum, FunctionalState NewState);
extern void GPIO_IntCmd(GPIO_PadTypeDef PadNum, FunctionalState NewState);
extern void GPIO_IntMaskCmd(GPIO_PadTypeDef PadNum, FunctionalState NewState);
extern uint8_t GPIO_GetIntStatus(GPIO_PadTypeDef PadNum);
extern uint64_t GPIO_GetAllIntStatus(void);
extern void GPIO_IntStatusClear(GPIO_PadTypeDef PadNum);
extern void GPIO_ClearAllIntStatus(void);
extern void ADC_GPIO_Init(GPIO_PadTypeDef PadNum);
extern void GPIO_AllPad_Enable_Rcr(void);
extern void GPIO_AllPad_Disable_Rcr(void);
extern void GPIO_SegPad_Enable_LcdMode(uint32_t SegPos);
extern void GPIO_SegPad_Disable_LcdMode(uint8_t SegPos);
extern void GPIO_ComPad_Enable_LcdMode(uint8_t ComPos);
extern void GPIO_ComPad_Disable_LcdMode(uint8_t ComPos);
extern void GPIO_Pad_Enable_Rcr(void);
extern void GPIO_Pad_Disable_Rcr(void);
extern void Gpio_LeakCurrentEnable(void);
extern void Gpio_LeakCurrentDisable(void);
extern GPIO_PinState GPIO_Read_InOutPin(GPIO_PadTypeDef GPIO_Pin);
extern void GPIO_SetPull(GPIO_PadTypeDef GPIO_Pin, GPIO_PullTypeDef pull);
extern void GPIO_TogglePin(GPIO_PadTypeDef GPIO_Pin);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __GPIO_H__
