/*
 * Analog voltage meter
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024-2025
 */

#ifndef AVOLTAGE_H_
#define AVOLTAGE_H_

#include "stm32f1xx_hal.h"

#include "cmsis_os.h"
#include "semphr.h"

struct avoltage
{
	SemaphoreHandle_t mutex;
	ADC_HandleTypeDef *adc;
	int ratio;
};


void avoltage_init(struct avoltage *avlt, ADC_HandleTypeDef *adc, int ratio);
int avoltage_calib(struct avoltage *avlt);
int avoltage(struct avoltage *avlt);

#endif /* AVOLTAGE_H_ */
