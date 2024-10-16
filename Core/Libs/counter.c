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
void counter_init(struct counter *cnt, volatile uint32_t *timestamp)
{
	memset(cnt, 0, sizeof(*cnt));
	cnt->count = 0;
	cnt->timestamp = timestamp;
	cnt->mutex = xSemaphoreCreateMutex();

	cnt->add = 0;
	cnt->get = 0;
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

inline static size_t next_pos(size_t pos)
{
	return (pos + 1) & (COUNTER_ITEMS_SIZE - 1);
}

/******************************************************************************/
void counter_list_add(struct counter *cnt)
{
	xSemaphoreTake(cnt->mutex, portMAX_DELAY);

	cnt->items[cnt->add].count = cnt->count;
	cnt->items[cnt->add].timestamp = *cnt->timestamp;
	cnt->add = next_pos(cnt->add);

	// Overflow
	if (cnt->add == cnt->get)
		cnt->get = next_pos(cnt->get);

	xSemaphoreGive(cnt->mutex);
}

/******************************************************************************/
int counter_list_add_safe(struct counter *cnt)
{
	xSemaphoreTake(cnt->mutex, portMAX_DELAY);

	// Full
	if (next_pos(cnt->add) == cnt->get)
	{
		xSemaphoreGive(cnt->mutex);
		return -1;
	}

	cnt->items[cnt->add].count = cnt->count;
	cnt->items[cnt->add].timestamp = *cnt->timestamp;
	cnt->add = next_pos(cnt->add);

	xSemaphoreGive(cnt->mutex);
	return 0;
}

/******************************************************************************/
int counter_list_get(struct counter *cnt, uint32_t *count, uint32_t *timestamp)
{
	xSemaphoreTake(cnt->mutex, portMAX_DELAY);

	// Empty
	if (cnt->add == cnt->get)
	{
		xSemaphoreGive(cnt->mutex);
		return -1;
	}

	*count = cnt->items[cnt->get].count;
	*timestamp = cnt->items[cnt->get].timestamp;
	cnt->get = next_pos(cnt->get);

	xSemaphoreGive(cnt->mutex);
	return 0;
}

/******************************************************************************/
int counter_list_peek(struct counter *cnt, uint32_t *count, uint32_t *timestamp)
{
	xSemaphoreTake(cnt->mutex, portMAX_DELAY);

	// Empty
	if (cnt->add == cnt->get)
	{
		xSemaphoreGive(cnt->mutex);
		return -1;
	}

	*count = cnt->items[cnt->get].count;
	*timestamp = cnt->items[cnt->get].timestamp;

	xSemaphoreGive(cnt->mutex);
	return 0;
}
