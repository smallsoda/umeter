/*
 * Pulse counter
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024-2025
 */

#include "counter.h"

#include <string.h>

#include "atomic.h"


/******************************************************************************/
void counter_init(struct counter *cnt, GPIO_TypeDef *pwr_port, uint16_t pwr_pin)
{
	memset(cnt, 0, sizeof(*cnt));
	cnt->count = 0;
	cnt->pwr_port = pwr_port;
	cnt->pwr_pin = pwr_pin;
}

/******************************************************************************/
void counter_irq(struct counter *cnt)
{
	atomic_inc(&cnt->count);
}

/******************************************************************************/
void counter_power_on(struct counter *cnt)
{
	HAL_GPIO_WritePin(cnt->pwr_port, cnt->pwr_pin, GPIO_PIN_RESET);
}

/******************************************************************************/
void counter_power_off(struct counter *cnt)
{
	HAL_GPIO_WritePin(cnt->pwr_port, cnt->pwr_pin, GPIO_PIN_SET);
}

/******************************************************************************/
uint32_t counter(struct counter *cnt)
{
	return cnt->count;
}
