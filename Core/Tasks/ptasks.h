/*
 * Application tasks
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#ifndef UMETER_TASKS_H_
#define UMETER_TASKS_H_

#include "counter.h"
#include "sim800l.h"
#include "logger.h"
#include "params.h"
#include "ota.h"

#define COUNTER_QUEUE_LEN 16 // ?

struct count_item
{
	uint32_t count;
	uint32_t timestamp;
};

struct count_queue
{
	QueueHandle_t queue;
	struct counter *cnt;
	volatile uint32_t *timestamp;
};

struct app
{
	struct sim800l *mod;
	struct logger *logger;
	struct tmpx75 *tmp;
	struct count_queue *cntq;

	volatile uint32_t *timestamp;
	volatile struct bl_params *bl;
	params_t *params;
};


void task_logger(struct logger *logger);
void task_sim800l(struct sim800l *mod);
void task_ota(struct ota *ota);
void task_app(struct app *app);
void task_info(struct app *app);
void task_system(IWDG_HandleTypeDef *wdg);
void task_counter(struct count_queue *cntq);
void task_blink(void);

#endif /* UMETER_TASKS_H_ */
