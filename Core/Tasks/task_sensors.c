/*
 * Counter task
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "ptasks.h"

#include <stdbool.h>

#include "tmpx75.h"

static osThreadId_t handle;
static const osThreadAttr_t attributes = {
  .name = "sensors",
  .stack_size = 96 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};


static void task(void *argument)
{
	struct sensors *sens = argument;
	struct item item;
	int ret;

	int32_t temperature;
	uint32_t count;

	TickType_t ticks = xTaskGetTickCount();
	TickType_t wake = xTaskGetTickCount();
	bool btmp = false;
	bool bcnt = false;

	// TMPx75
	ret = tmpx75_is_available(sens->tmp);
	if (!ret)
		btmp = true;

	// Counter
	bcnt = true;

	for(;;)
	{
		vTaskDelayUntil(&wake, pdMS_TO_TICKS(1000));

		// Update sensor readings
		if (bcnt)
			count = counter(sens->cnt);
		if (btmp)
			ret = tmpx75_read_temp(sens->tmp, &temperature);

		// Save sensor readings
		xSemaphoreTake(sens->actual->mutex, portMAX_DELAY);
		if (bcnt)
			sens->actual->count = count;
		if (btmp && !ret)
			sens->actual->temperature = temperature;
		xSemaphoreGive(sens->actual->mutex);

		/**
		 * TODO: Use COUNTER_QUEUE_LEN and app->params->period to determine
		 * optimal delay value
		 */
		// Add sensor readings to queues
		if ((xTaskGetTickCount() - ticks) >= pdMS_TO_TICKS(60000))
		{
			ticks = xTaskGetTickCount();

			if (bcnt)
			{
				item.value = count;
				item.timestamp = *sens->timestamp;
				xQueueSendToBack(sens->qcnt, &item, 0);
			}

			if (btmp && !ret)
			{
				item.value = temperature;
				item.timestamp = *sens->timestamp;
				xQueueSendToBack(sens->qtmp, &item, 0);
			}
		}
	}
}

/******************************************************************************/
void task_sensors(struct sensors *sens)
{
	handle = osThreadNew(task, sens, &attributes);
}

