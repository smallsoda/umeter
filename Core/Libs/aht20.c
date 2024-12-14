/*
 * AHT20 humidity and temperature sensor
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "aht20.h"

#include <string.h>

#include "cmsis_os.h"

#define I2C_ADDRESS 0x70

#define I2C_STATUS_CALIB_MASK 0x08 // 0000 1000
#define I2C_STATUS_BUSY_MASK  0x80 // 1000 0000

#define CRC_INITIAL_VALUE 0xFF
#define CRC_POLYNOMIAL    0x31

#define I2C_TIMEOUT 50

#define POW2_20  1048576
#define COEFF_RH (1000 * 100)
#define COEFF_T1 (1000 * 200)
#define COEFF_T2 (1000 * 50)

#define I2C_CMD_STATUS 0x71
static const uint8_t cmd_init[] = {0xBE, 0x08, 0x00};
static const uint8_t cmd_meas[] = {0xAC, 0x33, 0x00};


inline static void power_off(struct aht20 *sen)
{
	HAL_GPIO_WritePin(sen->pwr_port, sen->pwr_pin, GPIO_PIN_RESET);
}

inline static void power_on(struct aht20 *sen)
{
	HAL_GPIO_WritePin(sen->pwr_port, sen->pwr_pin, GPIO_PIN_SET);
}

/******************************************************************************/
void aht20_init(struct aht20 *sen, I2C_HandleTypeDef *i2c,
		GPIO_TypeDef *pwr_port, uint16_t pwr_pin)
{
	memset(sen, 0, sizeof(*sen));
	sen->i2c = i2c;
	sen->pwr_port = pwr_port;
	sen->pwr_pin = pwr_pin;

	power_off(sen);
}

/******************************************************************************/
int aht20_is_available(struct aht20 *sen)
{
	HAL_StatusTypeDef status;
	uint8_t buf;

	power_on(sen);
	osDelay(40);

	status = HAL_I2C_Mem_Read(sen->i2c, I2C_ADDRESS, I2C_CMD_STATUS,
			I2C_MEMADD_SIZE_8BIT, &buf, 1, I2C_TIMEOUT);
	power_off(sen);

	if (status != HAL_OK)
		return -1;

	return 0;
}

static uint8_t calc_crc8(uint8_t *data, size_t size)
{
	uint8_t crc = CRC_INITIAL_VALUE;

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
int aht20_read(struct aht20 *sen, int32_t *temp, int32_t *hum)
{
	HAL_StatusTypeDef status;
	uint32_t raw_temp;
	uint32_t raw_hum;
	uint8_t buf[7];

	power_on(sen);
	osDelay(40);

	/* Status */
	status = HAL_I2C_Mem_Read(sen->i2c, I2C_ADDRESS, I2C_CMD_STATUS,
			I2C_MEMADD_SIZE_8BIT, buf, 1, I2C_TIMEOUT);
	if (status != HAL_OK)
	{
		power_off(sen);
		return -1;
	}
	
	/* Not calibrated */
	if ((buf[0] & I2C_STATUS_CALIB_MASK) == 0)
	{
		status = HAL_I2C_Master_Transmit(sen->i2c, I2C_ADDRESS,
				(uint8_t *) cmd_init, sizeof(cmd_init), I2C_TIMEOUT);
		if (status != HAL_OK)
		{
			power_off(sen);
			return -1;
		}
		osDelay(10);
	}

	/* Trigger measurement */
	status = HAL_I2C_Master_Transmit(sen->i2c, I2C_ADDRESS,
			(uint8_t *) cmd_meas, sizeof(cmd_meas), I2C_TIMEOUT);
	if (status != HAL_OK)
	{
		power_off(sen);
		return -1;
	}
	osDelay(100); /* "Wait for 80ms for the measurement to be completed" */

	/* Receive data */
	status = HAL_I2C_Master_Receive(sen->i2c, I2C_ADDRESS, buf, sizeof(buf),
			I2C_TIMEOUT);
	power_off(sen);
	if (status != HAL_OK)
		return -1;

	/* Busy */
	if (buf[0] & I2C_STATUS_BUSY_MASK)
		return -1;

	/* CRC */
	if (calc_crc8(buf, 6) != buf[6])
		return -1;

	raw_hum = (uint32_t) buf[1] << 12 | (uint32_t) buf[2] << 4 | buf[3] >> 4;
	raw_temp = (uint32_t) buf[3] << 16 | (uint32_t) buf[4] << 8 | buf[5];
	raw_temp &= 0x000FFFFF;

	*hum = (int32_t)((int64_t) raw_hum * COEFF_RH / POW2_20);
	*temp = (int32_t)((int64_t) raw_temp * COEFF_T1 / POW2_20 - COEFF_T2);

	return 0;
}
