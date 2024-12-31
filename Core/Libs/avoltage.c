/*
 * Analog voltage meter
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "avoltage.h"

#include <string.h>

#define ADC_TIMEOUT 100
#define ADC_MAX     4095
#define ADC_REF     3300


/******************************************************************************/
void avoltage_init(struct avoltage *avlt, ADC_HandleTypeDef *adc, int ratio)
{
	memset(avlt, 0, sizeof(*avlt));
	avlt->adc = adc;
	avlt->ratio = ratio;
}

/******************************************************************************/
int avoltage_calib(struct avoltage *avlt)
{
	// TODO: Wrong values after calibration

//	HAL_StatusTypeDef status;

//	status = HAL_ADCEx_Calibration_Start(avlt->adc);
//	if (status != HAL_OK)
//		return -1;

	return 0;
}

/******************************************************************************/
int avoltage(struct avoltage *avlt)
{
	HAL_StatusTypeDef status;
	uint32_t value;

	status = HAL_ADC_Start(avlt->adc);
	if (status != HAL_OK)
		return -1;

	status = HAL_ADC_PollForConversion(avlt->adc, ADC_TIMEOUT);
	if (status != HAL_OK)
		return -1;

	value = HAL_ADC_GetValue(avlt->adc);

	return value * avlt->ratio * ADC_REF / ADC_MAX;
}
