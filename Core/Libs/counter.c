/*
 * Pulse counter
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "counter.h"

#include <string.h>

#include "atomic.h"


/******************************************************************************/
void counter_init(struct counter *cnt)
{
	memset(cnt, 0, sizeof(*cnt));
	cnt->count = 0;
}

/******************************************************************************/
void counter_irq(struct counter *cnt)
{
	atomic_inc(&cnt->count);
}

/******************************************************************************/
uint32_t counter(struct counter *cnt)
{
	return cnt->count;
}
