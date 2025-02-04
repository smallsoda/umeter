/*
 * Counter task
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024-2025
 */

#include "ptasks.h"

#include <stdlib.h>
#include <string.h>

#include "hcsr04.h"
#include "tmpx75.h"
#include "aht20.h"

#include "logger.h"
#define TAG "SENSORS"

enum
{
	AVAIL_VOL    = 0x01,
	AVAIL_CNT    = 0x02,
	AVAIL_TMPx75 = 0x04,
	AVAIL_AHT20  = 0x08,
	AVAIL_DIST   = 0x10
};

enum
{
	DRDY_VOL  = 0x01,
	DRDY_CNT  = 0x02,
	DRDY_TMP  = 0x04,
	DRDY_HUM  = 0x08,
	DRDY_DIST = 0x10
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
	int distance = 0;
	int voltage = 0;

	char *savail;

	TickType_t wake = xTaskGetTickCount();

	// Voltage
//	ret = avoltage_calib(sens->avlt);
//	if (!ret)
//		avail |= AVAIL_VOL;
	avail |= AVAIL_VOL;

	// Counter
	avail |= AVAIL_CNT;

	// TMPx75
	ret = tmpx75_is_available(sens->tmp);
	if (!ret)
		avail |= AVAIL_TMPx75;

	// AHT20
	ret = aht20_is_available(sens->aht);
	if (!ret)
		avail |= AVAIL_AHT20;

	// Distance
	avail |= AVAIL_DIST;

	// Available sensors
	xSemaphoreTake(sens->actual->mutex, portMAX_DELAY);
	sens->actual->avail = avail;
	xSemaphoreGive(sens->actual->mutex);

	savail = pvPortMalloc(sizeof(avail) * 8 + 2); // + "b\0"
	if (savail)
	{
		itoa(avail, savail, 2);
		strcat(savail, "b");
		logger_add_str(sens->logger, TAG, false, savail);
		vPortFree(savail);
	}

	for(;;)
	{
		// Init value (for counter only)
		if (avail & AVAIL_CNT)
		{
			counter_power_on(sens->cnt);
			osDelay(1);

			count = counter(sens->cnt);
		}

		// Measurement time delay (for counter only)
		if (avail & AVAIL_CNT)
			osDelay(sens->params->mtime_count * 1000);

		// Update sensor readings
		drdy = 0;

		if (avail & AVAIL_VOL)
		{
			voltage = avoltage(sens->avlt);
			if (voltage >= 0)
				drdy |= DRDY_VOL;
		}
		if (avail & AVAIL_CNT)
		{
			count = counter(sens->cnt) - count;
			drdy |= DRDY_CNT;

			if (sens->params->period_sen != sens->params->mtime_count)
				counter_power_off(sens->cnt);
		}
		if ((avail & AVAIL_TMPx75) && !(avail & AVAIL_AHT20))
		{
			ret = tmpx75_read_temp(sens->tmp, &temperature);
			if (!ret)
				drdy |= DRDY_TMP;
		}
		if (avail & AVAIL_AHT20)
		{
			ret = aht20_read(sens->aht, &temperature, &humidity);
			if (!ret)
				drdy |= DRDY_TMP | DRDY_HUM;
		}
		if (avail & AVAIL_DIST)
		{
			distance = hcsr04_read(sens->hcsr);
			if (distance >= 0)
				drdy |= DRDY_DIST;
		}

//		char *dbgstr = pvPortMalloc(128);
//		if (dbgstr)
//		{
//			itoa(distance, dbgstr, 10);
//			logger_add_str(sens->logger, "DISTANCE", false, dbgstr);
//			vPortFree(dbgstr);
//		}

		// Save sensor readings
		xSemaphoreTake(sens->actual->mutex, portMAX_DELAY);
		if (drdy & DRDY_VOL)
			sens->actual->voltage = voltage;
		if (drdy & DRDY_CNT)
			sens->actual->count = count;
		if (drdy & DRDY_TMP)
			sens->actual->temperature = temperature;
		if (drdy & DRDY_HUM)
			sens->actual->humidity = humidity;
		if (drdy & DRDY_DIST)
			sens->actual->distance = distance;
		xSemaphoreGive(sens->actual->mutex);

		// Add sensor readings to queues
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

		vTaskDelayUntil(&wake, pdMS_TO_TICKS(sens->params->period_sen * 1000));
//		osDelay(1000);
	}
}

/******************************************************************************/
void task_sensors(struct sensors *sens)
{
	handle = osThreadNew(task, sens, &attributes);
}

