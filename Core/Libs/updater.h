/*
 * UART firmware updater
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#ifndef UPDATER_H_
#define UPDATER_H_

#include "stm32f1xx_hal.h"

#include "cmsis_os.h"
#include "stream_buffer.h"

#include "w25q.h"

#define UPDATER_PAYLOAD_SIZE 16

struct updater
{
	UART_HandleTypeDef *uart;
	StreamBufferHandle_t stream;
	struct w25q *mem;
};

enum updater_cmd
{
	UPDATER_CMD_START = 0x00,
	UPDATER_CMD_DATA  = 0x01,
	UPDATER_CMD_RESET = 0x02
};

struct updater_packet
{
	uint32_t cmd;
	uint32_t checksum;
	uint32_t payload[UPDATER_PAYLOAD_SIZE];
};


void updater_init(struct updater *upd, struct w25q *mem,
		UART_HandleTypeDef *uart);
void updater_irq(struct updater *upd, const char *buf, size_t len);
void updater_task(struct updater *upd);

#endif /* UPDATER_H_ */
