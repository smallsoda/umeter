/*
 * Logger
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "logger.h"

#include <string.h>
#include <stdlib.h>

#define MAX_DATA_LEN 128


/******************************************************************************/
void logger_init(struct logger *logger, UART_HandleTypeDef *uart, size_t qsize)
{
	memset(logger, 0, sizeof(*logger));
	logger->uart = uart;
	logger->queue = xQueueCreate(qsize, sizeof(void *));
	logger->task = NULL;
}

/******************************************************************************/
void logger_irq(struct logger *logger)
{
	BaseType_t woken = pdFALSE;

	if (logger->task)
		vTaskNotifyGiveFromISR(logger->task, &woken);
}

/******************************************************************************/
void logger_task(struct logger *logger)
{
	char *string;

	logger->task = xTaskGetCurrentTaskHandle();

	for (;;)
	{
		xQueueReceive(logger->queue, &string, portMAX_DELAY);
		HAL_UART_Transmit_DMA(logger->uart, (uint8_t *) string, strlen(string));
		ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));
		vPortFree(string);
	}
}

/******************************************************************************/
#ifdef LOGGER
int logger_add(struct logger *logger, const char *tag, bool big,
		const char *buf, size_t len)
{
	BaseType_t status;
	char ticks[16];
	char *log;
	size_t ll;

	if (!big && len > MAX_DATA_LEN)
		len = MAX_DATA_LEN;

	itoa(xTaskGetTickCount(), ticks, 10);

	ll = strlen(tag) + 1 + strlen(ticks) + 1 + len + 2 + 1;
	log = pvPortMalloc(ll);
	if (!log)
		return -1;

	strcpy(log, tag);
	strcat(log, ",");
	strcat(log, ticks);
	strcat(log, ",");
	memcpy(&log[strlen(log)], buf, len);
	log[ll - 3] = '\r';
	log[ll - 2] = '\n';
	log[ll - 1] = '\0';

	for (int i = 0; i < ll - 3; i++)
	{
		if (log[i] == '\r')
			log[i] = 'r';
		else if (log[i] == '\n')
			log[i] = 'n';
		else if ((log[i] < 0x20) || (log[i] > 0x7E))
			log[i] = '*';
	}

	status = xQueueSendToBack(logger->queue, &log, 0);
	if (status != pdTRUE)
	{
		vPortFree(log);
		return -1;
	}

	return 0;
}
#endif

/******************************************************************************/
#ifdef LOGGER
int logger_add_str(struct logger *logger, const char *tag, bool big,
		const char *buf)
{
	return logger_add(logger, tag, big, buf, strlen(buf));
}
#endif
