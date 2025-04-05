/*
 * W25Q Serial FLASH memory with mutex lock
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2025
 */

#ifndef W25Q_SAFE_H_
#define W25Q_SAFE_H_

#include "cmsis_os.h"
#include "semphr.h"

#include "w25q.h"

#define W25Q_SAFE_TIMEOUT pdMS_TO_TICKS(100)

struct w25q_safe
{
	SemaphoreHandle_t mutex;
	struct w25q mem;
};


inline static void w25q_safe_init(struct w25q_safe *smem,
		SPI_HandleTypeDef *spi, GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
	smem->mutex = xSemaphoreCreateMutex();
	w25q_init(&smem->mem, spi,cs_port, cs_pin);
}

inline static void w25q_safe_sector_erase(struct w25q_safe *smem,
		uint32_t address)
{
	if (xSemaphoreTake(smem->mutex, W25Q_SAFE_TIMEOUT) == pdFALSE)
		return;

	w25q_sector_erase(&smem->mem, address);
	xSemaphoreGive(smem->mutex);
}

inline static void w25q_safe_read_data(struct w25q_safe *smem, uint32_t address,
		uint8_t *data, uint16_t size)
{
	if (xSemaphoreTake(smem->mutex, W25Q_SAFE_TIMEOUT) == pdFALSE)
		return;

	w25q_read_data(&smem->mem, address, data, size);
	xSemaphoreGive(smem->mutex);
}

inline static void w25q_safe_write_data(struct w25q_safe *smem,
		uint32_t address, uint8_t *data, uint16_t size)
{
	if (xSemaphoreTake(smem->mutex, W25Q_SAFE_TIMEOUT) == pdFALSE)
		return;

	w25q_write_data(&smem->mem, address, data, size);
	xSemaphoreGive(smem->mutex);
}

inline static size_t w25q_safe_get_capacity(struct w25q_safe *smem)
{
	size_t cap;
	if (xSemaphoreTake(smem->mutex, W25Q_SAFE_TIMEOUT) == pdFALSE)
		return;

	cap = w25q_get_capacity(&smem->mem);
	xSemaphoreGive(smem->mutex);
	return cap;
}

#endif /* W25Q_SAFE_H_ */
