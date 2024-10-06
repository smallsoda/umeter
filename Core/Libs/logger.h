/*
 * Logger
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include "stm32f1xx_hal.h"

#include "cmsis_os.h"
#include "queue.h"
#include "task.h"

struct logger
{
	UART_HandleTypeDef *uart;
	xQueueHandle queue;
	TaskHandle_t task;
};


void logger_init(struct logger *logger, UART_HandleTypeDef *uart);
void logger_irq(struct logger *logger);
void logger_task(struct logger *logger);

#endif /* LOGGER_H_ */
