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
#include "ota.h"

#define SENSORS_QUEUE_LEN 16 // ?

struct item
{
	uint32_t value;
	uint32_t timestamp;
};

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
	QueueHandle_t qcnt;
	QueueHandle_t qtmp;
	QueueHandle_t qhum;

	struct avoltage *avlt;
	struct counter *cnt;
	struct tmpx75 *tmp;
	struct aht20 *aht;
	volatile uint32_t *timestamp;
	params_t *params;

	struct logger *logger;
	struct actual *actual;
};

struct app
{
	struct sim800l *mod;
	struct sensors *sens;
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

#endif /* UMETER_TASKS_H_ */
