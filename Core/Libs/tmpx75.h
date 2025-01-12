/*
 * TMPx75 temperature sensor
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024-2025
 */

#ifndef TMPX75_H_
#define TMPX75_H_

#include "stm32f1xx_hal.h"

typedef void (*tmpx75_hw_init)(void);

struct tmpx75
{
	I2C_HandleTypeDef *i2c;
	tmpx75_hw_init hw_init;
	GPIO_TypeDef *pwr_port;
	uint16_t pwr_pin;

	uint8_t address;
};

void tmpx75_init(struct tmpx75 *sen, I2C_HandleTypeDef *i2c,
		tmpx75_hw_init hw_init, GPIO_TypeDef *pwr_port, uint16_t pwr_pin,
		uint8_t address);
int tmpx75_is_available(struct tmpx75 *sen);
int tmpx75_read_temp(struct tmpx75 *sen, int32_t *value);


#endif /* TMPX75_H_ */
