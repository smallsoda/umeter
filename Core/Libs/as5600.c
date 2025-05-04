/*
 * AS5600 contactless potentiometer
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2025
 */

#include "as5600.h"

#include <string.h>

#include "cmsis_os.h"

#define I2C_TIMEOUT 50

#define I2C_ADDRESS 0x6C

#define I2C_REG_STATUS     0x0B
#define I2C_REG_ANGLE_HIGH 0x0E
#define I2C_REG_ANGLE_LOW  0x0F

#define REG_STATUS_MASK (AS5600_STATUS_MH | AS5600_STATUS_ML | AS5600_STATUS_MD)

#define COEFF_1 360
#define COEFF_2 4095


inline static void power_off(struct as5600 *sen)
{
	HAL_I2C_DeInit(sen->i2c);
	HAL_GPIO_WritePin(sen->pwr_port, sen->pwr_pin, GPIO_PIN_RESET);
}

inline static void power_on(struct as5600 *sen)
{
	HAL_GPIO_WritePin(sen->pwr_port, sen->pwr_pin, GPIO_PIN_SET);
	osDelay(20); // 10
	sen->hw_init();
}

/******************************************************************************/
void as5600_init(struct as5600 *sen, I2C_HandleTypeDef *i2c,
		as5600_hw_init hw_init, GPIO_TypeDef *pwr_port, uint16_t pwr_pin)
{
	memset(sen, 0, sizeof(*sen));
	sen->i2c = i2c;
	sen->hw_init = hw_init;
	sen->pwr_port = pwr_port;
	sen->pwr_pin = pwr_pin;

	power_off(sen);
}

static int get_status(struct as5600 *sen)
{
	HAL_StatusTypeDef status;
	uint8_t buf;

	power_on(sen);

	status = HAL_I2C_Mem_Read(sen->i2c, I2C_ADDRESS, I2C_REG_STATUS,
			I2C_MEMADD_SIZE_8BIT, &buf, 1, I2C_TIMEOUT);
	power_off(sen);

	if (status != HAL_OK)
		return -1;

	return buf;
}

/******************************************************************************/
int as5600_is_available(struct as5600 *sen)
{
	return get_status(sen) < 0 ? -1 : 0;
}

/******************************************************************************/
int as5600_status(struct as5600 *sen)
{
	int status = get_status(sen);
	return status < 0 ? -1 : status & REG_STATUS_MASK;
}

/******************************************************************************/
int32_t as5600_read(struct as5600 *sen)
{
	HAL_StatusTypeDef status;
	uint8_t buf[2];
	int32_t angle;

	power_on(sen);

	status = HAL_I2C_Mem_Read(sen->i2c, I2C_ADDRESS, I2C_REG_ANGLE_HIGH,
			I2C_MEMADD_SIZE_8BIT, buf, 2, I2C_TIMEOUT);
	power_off(sen);
	
	if (status != HAL_OK)
		return -1;

	angle = (((uint16_t) buf[0] << 8) | buf[1]) & 0x0FFF;
	angle = angle * 1000 * COEFF_1 / COEFF_2;
	return angle;
}
