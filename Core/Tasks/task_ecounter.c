/*
 * Extended counter task
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2025
 */

#include "ptasks.h"

#include <stdlib.h>
#include <string.h>

#define COUNT_MIN_INIT 0xFFFFFFFF

static osThreadId_t handle;
static const osThreadAttr_t attributes = {
  .name = "ecounter",
  .stack_size = 64 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};


static void task(void *argument)
{
	struct ecounter *ecnt = argument;
	struct item item;

	uint32_t count = 0;
	size_t sumcnt = 0;
	uint32_t min = COUNT_MIN_INIT;
	uint32_t max = 0;
	uint32_t sum = 0;
	uint32_t value;

	TickType_t wake = xTaskGetTickCount();
	TickType_t psen = xTaskGetTickCount();

	counter_power_on(ecnt->cnt);
	osDelay(10);

	count = counter(ecnt->cnt);

	for (;;)
	{
		vTaskDelayUntil(&wake, pdMS_TO_TICKS(ecnt->params->mtime_count * 1000));

		value = counter(ecnt->cnt) - count;
		count = counter(ecnt->cnt);

		if (value < min)
			min = value;

		if (value > max)
			max = value;

		sum += value;
		sumcnt++;

		xSemaphoreTake(ecnt->actual->mutex, portMAX_DELAY);
		ecnt->actual->count = value;
		xSemaphoreGive(ecnt->actual->mutex);

		if ((xTaskGetTickCount() - psen) >=
				pdMS_TO_TICKS(ecnt->params->period_sen * 1000))
		{
			psen = xTaskGetTickCount();

			// max
			item.value = max;
			item.timestamp = *ecnt->timestamp;
			mqueue_set(ecnt->qec_max, &item);

			// min
			if (min != COUNT_MIN_INIT)
				item.value = min;
			else
				item.value = 0;
			item.timestamp = *ecnt->timestamp;
			mqueue_set(ecnt->qec_min, &item);

			// avg
			if (sumcnt)
				item.value = sum / sumcnt;
			else
				item.value = 0;
			item.timestamp = *ecnt->timestamp;
			mqueue_set(ecnt->qec_avg, &item);

			min = COUNT_MIN_INIT;
			max = 0;
			sum = 0;
			sumcnt = 0;
		}
	}
}

/******************************************************************************/
void task_ecounter(struct ecounter *ecnt)
{
	handle = osThreadNew(task, ecnt, &attributes);
}

