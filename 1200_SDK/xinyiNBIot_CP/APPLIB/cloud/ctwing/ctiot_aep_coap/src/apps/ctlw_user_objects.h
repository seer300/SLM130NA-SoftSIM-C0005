#ifndef __CTLW_USER_OBJECTS_H
#define __CTLW_USER_OBJECTS_H
#include "ctlw_lwm2m_sdk.h"
#ifdef __cplusplus
extern "C"
{
#endif
	/*添加用户定义的object*/
	#define CTLW_TEST_OBJECT_ID 3303
	ctlw_lwm2m_object_t * ctlw_get_test_object(void);
	ctlw_lwm2m_object_t *ctlw_get_user_object_firmware(void);
	ctlw_lwm2m_object_t *ctlw_get_user_object_device(void);
#ifdef __cplusplus
}
#endif
#endif //__CTLW_USER_OBJECTS_H
