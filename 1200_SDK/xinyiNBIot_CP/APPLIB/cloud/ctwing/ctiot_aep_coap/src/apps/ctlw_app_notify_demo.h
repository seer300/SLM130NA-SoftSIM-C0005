#ifndef __CTLW_APP_NOTIFY_DEMO_H
#define __CTLW_APP_NOTIFY_DEMO_H

void ctlw_app_notify_demo_1(char* uri, ctiot_notify_type notifyType, ctiot_notify_subType subType, uint16_t value, uint32_t data1, uint32_t data2, void *reservedData);

void ctlw_app_notify_demo_2(char* uri, ctiot_notify_type notifyType, ctiot_notify_subType subType, uint16_t value, uint32_t data1, uint32_t data2, void *reservedData);

float ctiot_app_demo_get_value(void);
void  ctiot_app_demo_set_value(float value);

#endif //__CTLW_APP_NOTIFY_DEMO_H
