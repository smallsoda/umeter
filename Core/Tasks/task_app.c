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

	struct sim800l_voltage vd;
	struct sim800l_http httpd;
	char *request;
	char *url;
	int status;

	request = pvPortMalloc(512);
	if (!request)
		vTaskDelete(NULL);

	url = pvPortMalloc(PARAMS_APP_URL_SIZE + 32);
	if (!request)
		vTaskDelete(NULL);

	vd.context = &status;
	httpd.context = &status;
//	httpd.url = app->params->url_app;
	httpd.url = url;
//	httpd.request = request;
	httpd.request = NULL;

	strcpy(url, app->params->url_app);
	strcat(url, "/api/gettime");

	sim800l_http(app->mod, &httpd, http_callback, 60000);
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	// TODO: Use JSON response
	if (!status)
	{
		*app->timestamp = strtoul(httpd.response, NULL, 0);
		logger_add_str(app->logger, TAG, false, httpd.response);
	}
	if (httpd.response)
		vPortFree(httpd.response);

	//
	strcpy(url, app->params->url_app);
	httpd.request = request;

	strjson_init(request);
	strjson_uint(request, "ts", *app->timestamp);
	strjson_str(request, "name", PARAMS_DEVICE_NAME);
	strjson_str(request, "bl_git", (char *) app->bl->hash);
	strjson_str(request, "bl_dt", (char *) app->bl->datetime);
	strjson_uint(request, "bl_status", app->bl->status);
	strjson_str(request, "app_git", GIT_COMMIT_HASH);
	strjson_str(request, "app_dt", PARAMS_DATETIME);
	strjson_uint(request, "app_ver", PARAMS_FW_VERSION);
	strjson_str(request, "mcu", app->params->mcu_uid);
	strjson_str(request, "apn", app->params->apn);
	strjson_str(request, "url_ota", app->params->url_ota);
	strjson_str(request, "url_app", app->params->url_app);
	strjson_uint(request, "period", app->params->period);

	sim800l_http(app->mod, &httpd, http_callback, 60000);
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	// TODO: Parse JSON response

	if (httpd.response)
		vPortFree(httpd.response);

	for (;;)
	{
		sim800l_voltage(app->mod, &vd, voltage_callback, 60000);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if (status)
			vd.voltage = 0;

		strjson_init(request);
		strjson_uint(request, "ts", *app->timestamp);
		strjson_uint(request, "ticks", xTaskGetTickCount());
		strjson_int(request, "bat", vd.voltage);
		strjson_null(request, "temp");
		strjson_null(request, "count");

		sim800l_http(app->mod, &httpd, http_callback, 60000);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		// TODO: Parse JSON response

		if (httpd.response)
			vPortFree(httpd.response);

		blink();
		osDelay(app->params->period * 1000);
	}
}

/******************************************************************************/
void task_app(struct app *app)
{
	handle = osThreadNew(task, app, &attributes);
}
