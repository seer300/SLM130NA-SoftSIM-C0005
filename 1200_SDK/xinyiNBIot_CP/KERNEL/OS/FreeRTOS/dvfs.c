#include "driver_adapt.h"

#include "task_attribute.h"


typedef struct
{
	char    *taskname;
	uint32_t frequency;
} dvfs_table_t;


dvfs_table_t task_freq_table[] =
{
	{LED_TASK_NAME, 39168000},
	{CSP_TASK_NAME, 39168000}
};

