/*
 * Logger
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "logger.h"

#include <string.h>


/******************************************************************************/
void logger_init(struct logger *logger, UART_HandleTypeDef *uart)
{
	;
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
