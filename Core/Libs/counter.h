/*
 * Pulse counter
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024-2026
 */

#ifndef COUNTER_H_
#define COUNTER_H_

#include "stm32f4xx_hal.h"

struct counter
{
	volatile uint32_t count;
	GPIO_TypeDef *pwr_port;
	uint16_t pwr_pin;
};


void counter_init(struct counter *cnt, GPIO_TypeDef *pwr_port,
		uint16_t pwr_pin);
void counter_irq(struct counter *cnt);
void counter_power_on(struct counter *cnt);
void counter_power_off(struct counter *cnt);
uint32_t counter(struct counter *cnt);

#endif /* COUNTER_H_ */
