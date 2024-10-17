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
	struct count_queue *cntq = argument;

	struct count_item item;

	for(;;)
	{
		/**
		 * TODO: Use cntq->queue size and app->params->period to determine
		 * optimal delay value
		 */
		osDelay(60000);

		item.count = counter(cntq->cnt);
		item.timestamp = *cntq->timestamp;
		xQueueSendToBack(cntq->queue, &item, portMAX_DELAY);
	}
}

/******************************************************************************/
void task_counter(struct count_queue *cntq)
{
	handle = osThreadNew(task, cntq, &attributes);
}

