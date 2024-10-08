/*
 * Logger
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include "cmsis_os.h"
#include "queue.h"
#include "task.h"

#define LOGGER

struct logger
{
	UART_HandleTypeDef *uart;
	xQueueHandle queue;
	TaskHandle_t task;
};


void logger_init(struct logger *logger, UART_HandleTypeDef *uart, size_t qsize);
void logger_irq(struct logger *logger);
void logger_task(struct logger *logger);

#ifdef LOGGER
int logger_add(struct logger *logger, const char *tag, bool big,
		const char *buf, size_t len);
#else
inline static int logger_add(struct logger *logger, const char *tag, bool big,
		const char *buf, size_t len)
{
	return 0;
}
#endif

#ifdef LOGGER
int logger_add_str(struct logger *logger, const char *tag, bool big,
		const char *buf);
#else
inline static int logger_add_str(struct logger *logger, const char *tag,
		bool big, const char *buf)
{
	return 0;
}
#endif

#endif /* LOGGER_H_ */
