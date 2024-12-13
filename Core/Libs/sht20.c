/*
 * SHT20 humidity and temperature sensor
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "sht20.h"

#include <string.h>

#include "cmsis_os.h"

#define I2C_ADDRESS 0x80

#define I2C_CMD_USER       0xE7
#define I2C_CMD_HOLD_T     0xE3
#define I2C_CMD_HOLD_RH    0xE5
#define I2C_CMD_NO_HOLD_T  0xF3
#define I2C_CMD_NO_HOLD_RH 0xF5

#define MASK_REG_USER_RESERVED 0x38 // 0011 1000
#define DEFAULT_REG_USER       0x02 // 0000 0010

#define MASK_REG_DATA 0xFFFC

#define CRC_POLYNOMIAL 0x0131

#define I2C_TIMEOUT 50

#define READ_ATTEMPTS 3
#define READ_WAIT     100

#define COEFF_T1 (1000 * 175.72 / 65536)
#define COEFF_T2 (1000 * 46.85)
#define COEFF_RH1 (1000 * 125 / 65536)
#define COEFF_RH2 (1000 * 6)


inline static void power_off(struct sht20 *sen)
{
	HAL_GPIO_WritePin(sen->pwr_port, sen->pwr_pin, GPIO_PIN_RESET);
}

inline static void power_on(struct sht20 *sen)
{
	HAL_GPIO_WritePin(sen->pwr_port, sen->pwr_pin, GPIO_PIN_SET);
}

/******************************************************************************/
void sht20_init(struct sht20 *sen, I2C_HandleTypeDef *i2c,
		GPIO_TypeDef *pwr_port, uint16_t pwr_pin)
{
	memset(sen, 0, sizeof(*sen));
	sen->i2c = i2c;
	sen->pwr_port = pwr_port;
	sen->pwr_pin = pwr_pin;

	power_off(sen);
}

/******************************************************************************/
int sht20_is_available(struct sht20 *sen)
{
	HAL_StatusTypeDef status;
	uint8_t buf;

	power_on(sen);
	osDelay(10);

	status = HAL_I2C_Mem_Read(sen->i2c, I2C_ADDRESS, I2C_CMD_USER,
			I2C_MEMADD_SIZE_8BIT, &buf, 1, I2C_TIMEOUT);
	power_off(sen);

	if (status != HAL_OK)
		return -1;

	buf &= ~MASK_REG_USER_RESERVED;

	if (buf != DEFAULT_REG_USER)
		return -1;

	return 0;
}

static uint8_t calc_crc8(uint8_t *data, size_t size)
{
	uint8_t crc = 0;

	for (size_t i = 0; i < size; i++)
	{
		crc ^= data[i];
		for (uint8_t bit = 0; bit < 8; bit++)
		{
			if (crc & 0x80)
				crc = (crc << 1) ^ CRC_POLYNOMIAL;
			else
				crc = (crc << 1);
		}
	}

	return crc;
}

/******************************************************************************/
int sht20_read_temp(struct sht20 *sen, int32_t *value)
{
	int attempts = READ_ATTEMPTS;
	HAL_StatusTypeDef status;
	uint8_t cmd, buf[3];
	uint16_t reg;

	power_on(sen);
	osDelay(50);

	cmd = I2C_CMD_NO_HOLD_T;
	status = HAL_I2C_Master_Transmit(sen->i2c, I2C_ADDRESS, &cmd, 1,
			I2C_TIMEOUT);
	if (status != HAL_OK)
	{
		power_off(sen);
		return -1;
	}
	
	while (attempts)
	{
		attempts--;
		osDelay(READ_WAIT);

		status = HAL_I2C_Master_Receive(sen->i2c, I2C_ADDRESS, buf, sizeof(buf),
				I2C_TIMEOUT);
		if (status != HAL_OK && !attempts)
		{
			power_off(sen);
			return -1;
		}
	}

	power_off(sen);

	if (calc_crc8(buf, 2) != buf[2])
		return -1;

	reg = (uint16_t) buf[0] << 8 | buf[1];
	reg &= MASK_REG_DATA;

	*value = (int32_t)((int64_t) reg * COEFF_T1 - COEFF_T2);
	return 0;
}

/******************************************************************************/
int sht20_read_hum(struct sht20 *sen, int32_t *value)
{
	int attempts = READ_ATTEMPTS;
	HAL_StatusTypeDef status;
	uint8_t cmd, buf[3];
	uint16_t reg;

	power_on(sen);
	osDelay(50);

	cmd = I2C_CMD_NO_HOLD_RH;
	status = HAL_I2C_Master_Transmit(sen->i2c, I2C_ADDRESS, &cmd, 1,
			I2C_TIMEOUT);
	if (status != HAL_OK)
	{
		power_off(sen);
		return -1;
	}
	
	while (attempts)
	{
		attempts--;
		osDelay(READ_WAIT);

		status = HAL_I2C_Master_Receive(sen->i2c, I2C_ADDRESS, buf, sizeof(buf),
				I2C_TIMEOUT);
		if (status != HAL_OK && !attempts)
		{
			power_off(sen);
			return -1;
		}
	}

	power_off(sen);

	if (calc_crc8(buf, 2) != buf[2])
		return -1;

	reg = (uint16_t) buf[0] << 8 | buf[1];
	reg &= MASK_REG_DATA;

	*value = (int32_t)((int64_t) reg * COEFF_RH1 - COEFF_RH2);
	return 0;
}
