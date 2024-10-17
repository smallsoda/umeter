/*
 * Pulse counter
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#ifndef COUNTER_H_
#define COUNTER_H_

#include <stdint.h>

struct counter
{
	volatile uint32_t count;
};


void counter_init(struct counter *cnt);
void counter_irq(struct counter *cnt);
uint32_t counter(struct counter *cnt);

#endif /* COUNTER_H_ */
