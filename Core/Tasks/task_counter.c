/*
 * Counter task
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "ptasks.h"

static osThreadId_t handle;
static const osThreadAttr_t attributes = {
  .name = "counter",
  .stack_size = 64 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static void task(void *argument)
{
	struct counter *cnt = argument;

	for(;;)
	{
		/**
		 * TODO: Use COUNTER_ITEMS_SIZE and app->params->period to determine
		 * optimal delay value
		 */
		osDelay(60000);
		counter_list_add(cnt);
	}
}

/******************************************************************************/
void task_counter(struct counter *cnt)
{
	handle = osThreadNew(task, cnt, &attributes);
}

