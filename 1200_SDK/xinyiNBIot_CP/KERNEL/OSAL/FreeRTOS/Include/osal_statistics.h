#ifndef OSAL_STATISTICS_H
#define OSAL_STATISTICS_H

// trace task record
extern void vTaskSwitchOut( void );
extern void vTaskSwitchIn( void );
void vTaskSwitchInit(void);

#define traceTASK_SWITCHED_OUT() vTaskSwitchOut()
#define traceTASK_SWITCHED_IN() vTaskSwitchIn()


#endif  /* OSAL_STATISTICS_H */
