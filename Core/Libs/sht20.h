/*
 * SHT20 humidity and temperature sensor
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#ifndef SHT20_H_
#define SHT20_H_

#include "stm32f1xx_hal.h"

struct sht20
{
	I2C_HandleTypeDef *i2c;
	GPIO_TypeDef *pwr_port;
	uint16_t pwr_pin;
};

void sht20_init(struct sht20 *sen, I2C_HandleTypeDef *i2c,
		GPIO_TypeDef *pwr_port, uint16_t pwr_pin);
int sht20_is_available(struct sht20 *sen);
int sht20_read_temp(struct sht20 *sen, int32_t *value);
int sht20_read_hum(struct sht20 *sen, int32_t *value);


#endif /* SHT20_H_ */
