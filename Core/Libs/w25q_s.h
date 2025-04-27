/*
 * W25Q Serial FLASH memory with mutex lock
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2025
 */

#ifndef W25Q_S_H_
#define W25Q_S_H_

#include "cmsis_os.h"
#include "semphr.h"

#include "w25q.h"

#define W25Q_S_TIMEOUT pdMS_TO_TICKS(100)

#include <stdlib.h>
#include <string.h>
#include "logger.h"
#define W25Q_S_TAG "W25Q_S"
extern struct logger logger;

inline static void logger_w25q_s(const char *tag, uint32_t address, uint8_t *data,
		uint16_t size)
{
	char buf[128];
	char valbuf[32];

	if (size > 16)
		size = 16;

	strcpy(buf, tag);
	strcat(buf, " ");

	utoa(address, &buf[strlen(buf)], 10);
	strcat(buf, ": ");

	for (uint16_t i = 0; i < size; i++)
	{
		utoa(data[i], valbuf, 16);
		strcat(buf, valbuf);
		strcat(buf, " ");
	}

	logger_add_str(&logger, W25Q_S_TAG, false, buf);
}

struct w25q_s
{
	SemaphoreHandle_t mutex;
	struct w25q mem;
};


inline static void w25q_s_init(struct w25q_s *smem,
		SPI_HandleTypeDef *spi, GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
	smem->mutex = xSemaphoreCreateMutex();
	w25q_init(&smem->mem, spi,cs_port, cs_pin);
}

inline static void w25q_s_sector_erase(struct w25q_s *smem,
		uint32_t address)
{
	if (xSemaphoreTake(smem->mutex, W25Q_S_TIMEOUT) == pdFALSE)
		return;

	w25q_sector_erase(&smem->mem, address);
	xSemaphoreGive(smem->mutex);

	logger_w25q_s("ER", address, NULL, 0);
}

inline static void w25q_s_read_data(struct w25q_s *smem, uint32_t address,
		uint8_t *data, uint16_t size)
{
	if (xSemaphoreTake(smem->mutex, W25Q_S_TIMEOUT) == pdFALSE)
		return;

	w25q_read_data(&smem->mem, address, data, size);
	xSemaphoreGive(smem->mutex);

	logger_w25q_s("RD", address, data, size);
}

inline static void w25q_s_write_data(struct w25q_s *smem,
		uint32_t address, uint8_t *data, uint16_t size)
{
	if (xSemaphoreTake(smem->mutex, W25Q_S_TIMEOUT) == pdFALSE)
		return;

	w25q_write_data(&smem->mem, address, data, size);
	xSemaphoreGive(smem->mutex);

	logger_w25q_s("WR", address, data, size);
}

inline static size_t w25q_s_get_capacity(struct w25q_s *smem)
{
	size_t cap;
	if (xSemaphoreTake(smem->mutex, W25Q_S_TIMEOUT) == pdFALSE)
		return 0;

	cap = w25q_get_capacity(&smem->mem);
	xSemaphoreGive(smem->mutex);
	return cap;
}

inline static uint8_t w25q_s_get_manufacturer_id(struct w25q_s *smem)
{
	uint8_t mid;
	if (xSemaphoreTake(smem->mutex, W25Q_S_TIMEOUT) == pdFALSE)
		return 0;

	mid = w25q_get_manufacturer_id(&smem->mem);
	xSemaphoreGive(smem->mutex);
	return mid;
}

#endif /* W25Q_S_H_ */
