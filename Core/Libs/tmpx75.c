/*
 * TMPx75 temperature sensor
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "tmpx75.h"

#include <string.h>

#include "cmsis_os.h"

#define I2C_REG_TEMP    0x00
#define I2C_REG_CONF    0x01
#define I2C_REG_T_LOW   0x02
#define I2C_REG_T_HIGH  0x03
#define I2C_REG_ONESHOT 0x04

#define MODE_SHUTDOWN 0x01 // 0000 0001
#define MODE_ONESHOT  0x20 // 0010 0000

#define DEFAULT_T_LOW  0x4B00
#define DEFAULT_T_HIGH 0x5000

#define I2C_TIMEOUT 50

#define COEFF (0.0625 / 16)


inline static void power_off(struct tmpx75 *tmp)
{
	HAL_GPIO_WritePin(tmp->pwr_port, tmp->pwr_pin, GPIO_PIN_RESET);
}

inline static void power_on(struct tmpx75 *tmp)
{
	HAL_GPIO_WritePin(tmp->pwr_port, tmp->pwr_pin, GPIO_PIN_SET);
}

/******************************************************************************/
void tmpx75_init(struct tmpx75 *tmp, I2C_HandleTypeDef *i2c,
		GPIO_TypeDef *pwr_port, uint16_t pwr_pin, uint8_t address)
{
	memset(tmp, 0, sizeof(*tmp));
	tmp->i2c = i2c;
	tmp->pwr_port = pwr_port;
	tmp->pwr_pin = pwr_pin;
	tmp->address = address;

	power_off(tmp);
}

/******************************************************************************/
int tmpx75_is_available(struct tmpx75 *tmp)
{
	HAL_StatusTypeDef status;
	uint8_t buf[2];
	uint16_t reg;

	power_on(tmp);
	osDelay(10);

	status = HAL_I2C_Mem_Read(tmp->i2c, tmp->address, I2C_REG_T_LOW, 1, buf, 2,
			I2C_TIMEOUT);
	power_off(tmp);

	if (status != HAL_OK)
		return -1;

	reg = ((uint16_t) buf[0] << 8) | buf[1];

	if (reg != DEFAULT_T_LOW)
		return -1;

	return 0;
}

/******************************************************************************/
int tmpx75_read_temp(struct tmpx75 *tmp, int32_t *value)
{
	HAL_StatusTypeDef status;
	uint8_t buf[2];
	int16_t reg;

	power_on(tmp);
	osDelay(50);

	status = HAL_I2C_Mem_Read(tmp->i2c, tmp->address, I2C_REG_TEMP, 1, buf, 2,
			I2C_TIMEOUT);
	power_off(tmp);

	if (status != HAL_OK)
		return -1;

	reg = (int16_t) (((uint16_t) buf[0] << 8) | buf[1]);
	*value = (int32_t) reg * 1000 * COEFF;
	return 0;
}
