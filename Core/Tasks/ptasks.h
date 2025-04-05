/*
 * Application tasks
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024-2025
 */

#ifndef UMETER_TASKS_H_
#define UMETER_TASKS_H_

#include "cmsis_os.h"
#include "semphr.h"

#include "avoltage.h"
#include "counter.h"
#include "sim800l.h"
#include "siface.h"
#include "params.h"
#include "mqueue.h"
#include "ota.h"

#define SENSORS_QUEUE_SECNUM 2

struct actual
{
	SemaphoreHandle_t mutex;

	int avail;

	int voltage;
	uint32_t count;
	int32_t humidity;
	int32_t temperature;
};

struct sensors
{
	mqueue_t *qtmp;
	mqueue_t *qhum;

	struct avoltage *avlt;
	struct aht20 *aht;
	volatile uint32_t *timestamp;
	params_t *params;

	struct logger *logger;
	struct actual *actual;
};

struct ecounter
{
	mqueue_t *qec_avg;
	mqueue_t *qec_max;
	mqueue_t *qec_min;

	struct counter *cnt;
	volatile uint32_t *timestamp;
	params_t *params;

	struct actual *actual;
};

struct app
{
	struct sim800l *mod;
	struct sensors *sens;
	struct ecounter *ecnt;
	struct logger *logger;

	volatile uint32_t *timestamp;
	volatile struct bl_params *bl;
	params_t *params;
};


void task_siface(struct siface *siface);
void task_sim800l(struct sim800l *mod);
void task_ota(struct ota *ota);
void task_app(struct app *app);
void task_info(struct app *app);
void task_system(IWDG_HandleTypeDef *wdg);
void task_sensors(struct sensors *sens);
void task_ecounter(struct ecounter *ecnt);

#endif /* UMETER_TASKS_H_ */
