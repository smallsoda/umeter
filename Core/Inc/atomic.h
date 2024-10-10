/*
 * Atomic operations
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#ifndef ATOMIC_H_
#define ATOMIC_H_

#include "stm32f103xb.h"


inline static void atomic_inc(volatile uint32_t *pv)
{
	uint32_t value;
	do
	{
		value = __LDREXW(pv);
		value++;
	} while(__STREXW(value, pv));
}

#endif /* ATOMIC_H_ */
