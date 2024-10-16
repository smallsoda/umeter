/*
 * Pulse counter
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#ifndef COUNTER_H_
#define COUNTER_H_

#include <stdint.h>
#include <stddef.h>

#include "cmsis_os.h"
#include "semphr.h"

#define COUNTER_ITEMS_SIZE 16 // Only power of 2 is allowed

struct count_item
{
	uint32_t timestamp;
	uint32_t count;
};

struct counter
{
	SemaphoreHandle_t mutex;
	volatile uint32_t count;
	volatile uint32_t *timestamp;

	size_t add;
	size_t get;
	struct count_item items[COUNTER_ITEMS_SIZE];
};


void counter_init(struct counter *cnt, volatile uint32_t *timestamp);
void counter_irq(struct counter *cnt);
uint32_t counter(struct counter *cnt);

void counter_list_add(struct counter *cnt);
int counter_list_add_safe(struct counter *cnt);
int counter_list_get(struct counter *cnt, uint32_t *count, uint32_t *timestamp);
int counter_list_peek(struct counter *cnt, uint32_t *count,
		uint32_t *timestamp);

#endif /* COUNTER_H_ */
