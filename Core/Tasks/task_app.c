/*
 * Application task
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "ptasks.h"

#include <stdlib.h>
#include <string.h>
#include "params.h"
#include "main.h"
#include "fws.h"

static osThreadId_t handle;
static const osThreadAttr_t attributes = {
  .name = "app",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static void voltage_callback(int status, void *data)
{
	struct sim800l_voltage *vd = data;

//	BaseType_t woken = pdFALSE;
//	vTaskNotifyGiveFromISR(handle, &woken);

	*((int *) vd->context) = status;
}

static void http_callback(int status, void *data)
{
	struct sim800l_http *httpd = data;

	BaseType_t woken = pdFALSE;
	vTaskNotifyGiveFromISR(handle, &woken);

	*((int *) httpd->context) = status;
}

static void blink(void)
{
	osDelay(100);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
	osDelay(100);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

static void task(void *argument)
{
	struct app *app = argument;

	const char *url = "echo.free.beeceptor.com";
	const char *request = "{\"voltage\": 1234, \"location\": null}";
	struct sim800l_voltage vd;
	struct sim800l_http httpd;
	int status;

	vd.context = &status;
	httpd.context = &status;
	httpd.url = (char *) url;
//	httpd.request = NULL;
	httpd.request = (char *) request;

	for (;;)
	{
		sim800l_voltage(app->mod, &vd, voltage_callback, 5000);
//		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		sim800l_http(app->mod, &httpd, http_callback, 60000);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if (!status)
			xQueueSendToBack(app->logger->queue, &httpd.response, portMAX_DELAY);

		blink();
		osDelay(30000);
	}
}

/******************************************************************************/
void task_app(struct app *app)
{
	handle = osThreadNew(task, app, &attributes);
}
