#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>

#include "test_cmsis.h"

#define RECORD_MSG_MAX_NUM    256


typedef struct
{
	char *taskname;
	uint32_t start_tick;
	uint32_t end_tick;
} schedule_msg_t;

typedef struct
{
	schedule_msg_t schedule_msg[RECORD_MSG_MAX_NUM];
	uint16_t record_cnt;
	uint16_t data_full_flag;
} schedule_array_t;


schedule_array_t data_msg1;
schedule_array_t data_msg2;

schedule_array_t *pxCurrentMsg = &data_msg1;
schedule_array_t *pxOverflowMsg = &data_msg2;

uint32_t global_sequence = 0;

void write_message_to_log(void);

void record_message_in_schedule(void)
{
	char *taskname = pcTaskGetName(NULL);
	uint32_t ticks = xTaskGetTickCountFromISR();

	uint32_t record_cnt = pxCurrentMsg->record_cnt;

	pxCurrentMsg->schedule_msg[record_cnt].taskname = taskname;
	pxCurrentMsg->schedule_msg[record_cnt].end_tick = ticks;

	record_cnt++;

	if(record_cnt < RECORD_MSG_MAX_NUM)
	{
		pxCurrentMsg->schedule_msg[record_cnt].start_tick = ticks;
		pxCurrentMsg->record_cnt = record_cnt;
	}
	else
	{
		pxCurrentMsg->record_cnt = 0;
		pxCurrentMsg->data_full_flag = 1;

		schedule_array_t *tmpMsg = pxCurrentMsg;
		pxCurrentMsg = pxOverflowMsg;
		pxOverflowMsg = tmpMsg;

		pxCurrentMsg->schedule_msg[0].start_tick = ticks;

		osThreadFlagsSet(test_cmsis_log_handler, 0x00000001U);
	}
}

void write_message_to_log(void)
{
	if(pxOverflowMsg->data_full_flag == 1)
	{
		pxOverflowMsg->data_full_flag = 0;

		char *tmp_dat = pvPortMalloc(256);
		int len;

		char *taskname;
		uint32_t start_tick, end_tick;

		len = sprintf(tmp_dat, "\r\n sequence    task name            start tick    end tick\r\n");
		print_log(tmp_dat, len);

		for(uint32_t i=0; i<RECORD_MSG_MAX_NUM; i++)
		{
			taskname = pxOverflowMsg->schedule_msg[i].taskname;
			start_tick = pxOverflowMsg->schedule_msg[i].start_tick;
			end_tick = pxOverflowMsg->schedule_msg[i].end_tick;

			len = sprintf(tmp_dat, " %7lu     %-16s      %-10lu    %-10lu\r\n", global_sequence+i+1, taskname, start_tick, end_tick);
			print_log(tmp_dat, len);
		}

		global_sequence += RECORD_MSG_MAX_NUM;

		vPortFree(tmp_dat);
	}
}
