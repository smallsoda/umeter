/*
 * Sensors task
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024-2025
 */

#include "ptasks.h"

#include <stdlib.h>
#include <string.h>

#include "aht20.h"

#include "logger.h"
#define TAG "SENSORS"

//#define AVOLTAGE_CALIB

enum
{
	AVAIL_VOL    = 0x01,
	AVAIL_CNT    = 0x02, // Not used
	AVAIL_TMPx75 = 0x04, // Not used
	AVAIL_AHT20  = 0x08,
	AVAIL_DIST   = 0x10  // Not used
};

enum
{
	DRDY_VOL  = 0x01,
	DRDY_CNT  = 0x02, // Not used
	DRDY_TMP  = 0x04,
	DRDY_HUM  = 0x08,
	DRDY_DIST = 0x10  // Not used
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
	int voltage = 0;

	char *savail;

	TickType_t wake = xTaskGetTickCount();

	// Voltage
#ifdef AVOLTAGE_CALIB
	ret = avoltage_calib(sens->avlt);
	if (!ret)
		avail |= AVAIL_VOL;
#endif /* AVOLTAGE_CALIB */
	avail |= AVAIL_VOL;

	// AHT20
	ret = aht20_is_available(sens->aht);
	if (!ret)
		avail |= AVAIL_AHT20;

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

	for (;;)
	{
		// Update sensor readings
		drdy = 0;

		if (avail & AVAIL_VOL)
		{
			voltage = avoltage(sens->avlt);
			if (voltage >= 0)
				drdy |= DRDY_VOL;
		}
		if (avail & AVAIL_AHT20)
		{
			ret = aht20_read(sens->aht, &temperature, &humidity);
			if (!ret)
				drdy |= DRDY_TMP | DRDY_HUM;
		}

		// Save sensor readings
		xSemaphoreTake(sens->actual->mutex, portMAX_DELAY);
		if (drdy & DRDY_VOL)
			sens->actual->voltage = voltage;
		if (drdy & DRDY_TMP)
			sens->actual->temperature = temperature;
		if (drdy & DRDY_HUM)
			sens->actual->humidity = humidity;
		xSemaphoreGive(sens->actual->mutex);

		// Add sensor readings to queues
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
	}
}

/******************************************************************************/
void task_sensors(struct sensors *sens)
{
	handle = osThreadNew(task, sens, &attributes);
}

