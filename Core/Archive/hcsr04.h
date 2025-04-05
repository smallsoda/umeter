/*
 * HC-SR04 ultrasonic sensor
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2025
 */

#ifndef HCSR04_H_
#define HCSR04_H_

#include "stm32f1xx_hal.h"

#include <stdbool.h>

#include "cmsis_os.h"
#include "semphr.h"

struct hcsr04
{
	SemaphoreHandle_t sema;

	DWT_Type *dwt;
	GPIO_TypeDef *trig_port;
	uint16_t trig_pin;

	bool sstatus;
	uint32_t count;
};


void hcsr04_init(struct hcsr04 *sen, DWT_Type *dwt, GPIO_TypeDef *trig_port,
		uint16_t trig_pin);
void hcsr04_irq(struct hcsr04 *sen);
int hcsr04_read(struct hcsr04 *sen);


#endif /* HCSR04_H_ */
