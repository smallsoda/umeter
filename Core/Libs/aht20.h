/*
 * AHT20 humidity and temperature sensor
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024-2025
 */

#ifndef AHT20_H_
#define AHT20_H_

#include "stm32f1xx_hal.h"

typedef void (*aht20_hw_init)(void);

struct aht20
{
	I2C_HandleTypeDef *i2c;
	aht20_hw_init hw_init;
	GPIO_TypeDef *pwr_port;
	uint16_t pwr_pin;
};


void aht20_init(struct aht20 *sen, I2C_HandleTypeDef *i2c,
		aht20_hw_init hw_init, GPIO_TypeDef *pwr_port, uint16_t pwr_pin);
int aht20_is_available(struct aht20 *sen);
int aht20_read(struct aht20 *sen, int32_t *temp, int32_t *hum);

#endif /* AHT20_H_ */
