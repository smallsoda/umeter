/*
 * Application task
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "ptasks.h"

#include <stdlib.h>
#include <string.h>
#include "strjson.h"
#include "params.h"
#include "main.h"
#include "fws.h"

#include "logger.h"
#define TAG "APP"

static osThreadId_t handle;
static const osThreadAttr_t attributes = {
  .name = "app",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static void voltage_callback(int status, void *data)
{
	struct sim800l_voltage *vd = data;

	BaseType_t woken = pdFALSE;
	vTaskNotifyGiveFromISR(handle, &woken);

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
	struct sim800l_voltage vd;
	struct sim800l_http httpd;
	int status;

	char *request = pvPortMalloc(256);
	if (!request)
		vTaskDelete(NULL);

	vd.context = &status;
	httpd.context = &status;
	httpd.url = (char *) url;
//	httpd.request = NULL;
	httpd.request = request;

	for (;;)
	{
		sim800l_voltage(app->mod, &vd, voltage_callback, 60000);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if (!status)
		{
			strjson_init(request);
			strjson_str(request, "status", "ok");
			strjson_str(request, "name", PARAMS_DEVICE_NAME);
			strjson_int(request, "battery", vd.voltage);
		}
		else
		{
			strjson_init(request);
			strjson_str(request, "status", "err");
			strjson_str(request, "name", PARAMS_DEVICE_NAME);
			strjson_int(request, "battery", 0);
		}

		sim800l_http(app->mod, &httpd, http_callback, 60000);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if (!status)
			logger_add(app->logger, TAG, true, httpd.response, httpd.rlen);

		vPortFree(httpd.response);

		blink();
		osDelay(30000);
	}
}

/******************************************************************************/
void task_app(struct app *app)
{
	handle = osThreadNew(task, app, &attributes);
}
