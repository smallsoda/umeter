/*
 * Button
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2025
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#include "stm32f1xx_hal.h"

struct button
{
	GPIO_TypeDef *port;
	uint16_t pin;
	int counter;
	int state;

	void *callback;
};


void button_init(struct button *btn, GPIO_TypeDef *port, uint16_t pin,
		void *callback);
void button_task(struct button *btn);

#endif /* BUTTON_H_ */
