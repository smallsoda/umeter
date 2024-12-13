/*
 * Counter task
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "ptasks.h"

#include <stdbool.h>

#include "tmpx75.h"
#include "sht20.h"

enum
{
	AVAIL_CNT    = 0x01,
	AVAIL_TMPx75 = 0x02,
	AVAIL_SHT20  = 0x04
};

enum
{
	DRDY_CNT = 0x01,
	DRDY_TMP = 0x02,
	DRDY_HUM = 0x04
};

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
	int avail = 0; // Sensor is available (bit mask)
	int drdy; // Sensor data was successfully read (bit mask)
	int ret;

	int32_t temperature = 0;
	int32_t humidity = 0;
	uint32_t count = 0;

	TickType_t ticks = xTaskGetTickCount();
	TickType_t wake = xTaskGetTickCount();

	// Counter
	avail |= AVAIL_CNT;

	// TMPx75
	ret = tmpx75_is_available(sens->tmp);
	if (!ret)
		avail |= AVAIL_TMPx75;

	// SHT20
	ret = sht20_is_available(sens->sht);
	if (!ret)
		avail |= AVAIL_SHT20;

	for(;;)
	{
		vTaskDelayUntil(&wake, pdMS_TO_TICKS(1000));

		// Update sensor readings
		drdy = 0;

		if (avail & AVAIL_CNT)
		{
			count = counter(sens->cnt);
			drdy |= DRDY_CNT;
		}
		if ((avail & AVAIL_TMPx75) && !(avail & AVAIL_SHT20))
		{
			ret = tmpx75_read_temp(sens->tmp, &temperature);
			if (!ret)
				drdy |= DRDY_TMP;
		}
		if (avail & AVAIL_SHT20)
		{
			ret = sht20_read_temp(sens->sht, &temperature);
			if (!ret)
				drdy |= DRDY_TMP;

			ret = sht20_read_hum(sens->sht, &humidity);
			if (!ret)
				drdy |= DRDY_HUM;
		}

		// Save sensor readings
		xSemaphoreTake(sens->actual->mutex, portMAX_DELAY);
		if (drdy & DRDY_CNT)
			sens->actual->count = count;
		if (drdy & DRDY_TMP)
			sens->actual->temperature = temperature;
		if (drdy & DRDY_HUM)
			sens->actual->humidity = humidity;
		xSemaphoreGive(sens->actual->mutex);

		/**
		 * TODO: Use COUNTER_QUEUE_LEN and app->params->period to determine
		 * optimal delay value
		 */
		// Add sensor readings to queues
		if ((xTaskGetTickCount() - ticks) >= pdMS_TO_TICKS(60000))
		{
			ticks = xTaskGetTickCount();

			if (drdy & DRDY_CNT)
			{
				item.value = count;
				item.timestamp = *sens->timestamp;
				xQueueSendToBack(sens->qcnt, &item, 0);
			}

			if (drdy & DRDY_TMP)
			{
				item.value = temperature;
				item.timestamp = *sens->timestamp;
				xQueueSendToBack(sens->qtmp, &item, 0);
			}

			if (drdy & DRDY_HUM)
			{
				item.value = humidity;
				item.timestamp = *sens->timestamp;
				xQueueSendToBack(sens->qhum, &item, 0);
			}
		}
	}
}

/******************************************************************************/
void task_sensors(struct sensors *sens)
{
	handle = osThreadNew(task, sens, &attributes);
}

