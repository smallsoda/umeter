/*
 * Application task
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "ptasks.h"

#include <stdlib.h>
#include <string.h>

#define JSMN_HEADER
#include "jsmn.h"

#include "strjson.h"
#include "tmpx75.h"
#include "params.h"
#include "main.h"
#include "fws.h"

#include "logger.h"
#define TAG "APP"

#define JSON_MAX_TOKENS 8

#define HTTP_TIMEOUT_1MIN 60000
#define HTTP_TIMEOUT_2MIN 120000

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

// https://github.com/zserge/jsmn/blob/master/example/simple.c
static int jsoneq(const char *json, jsmntok_t *tok, const char *s)
{
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0)
		return 0;
	return -1;
}

static int parse_json_time(const char *response, uint32_t *ts)
{
	jsmn_parser parser;
	jsmntok_t tokens[JSON_MAX_TOKENS];
	int ret;

	memset(tokens, 0, sizeof(tokens));
	jsmn_init(&parser);
	ret = jsmn_parse(&parser, response, strlen(response), tokens,
			JSON_MAX_TOKENS);

	if (ret <= 0)
		return -1;

	if (tokens[0].type != JSMN_OBJECT)
		return -1;

	for (int i = 1; (i + 1) < ret; i += 2)
	{
		if (jsoneq(response, &tokens[i], "ts") == 0)
			*ts = strtoul(response + tokens[i + 1].start, NULL, 0);
	}

	return 0;
}

static void task(void *argument)
{
	struct app *app = argument;

	struct sim800l_voltage vd;
	struct sim800l_http httpd;
	int32_t temperature;
	bool meas_temp;
	char *request;
	char *url;
	int status;
	int ret;

	request = pvPortMalloc(512);
	if (!request)
		vTaskDelete(NULL);

	url = pvPortMalloc(PARAMS_APP_URL_SIZE + 32);
	if (!request)
		vTaskDelete(NULL);

	vd.context = &status;
	httpd.context = &status;

	// TMPx75
	ret = tmpx75_is_available(app->tmp);
	if (!ret)
		meas_temp = true;
	else
		meas_temp = false;

	// <- /api/time
	strcpy(url, app->params->url_app);
	strcat(url, "/api/time");
	httpd.url = url;
	httpd.request = NULL;
	httpd.response = NULL;

	sim800l_http(app->mod, &httpd, http_callback, HTTP_TIMEOUT_2MIN);
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	if (!status)
	{
		uint32_t temp;
		ret = parse_json_time(httpd.response, &temp);
		if (!ret)
			*app->timestamp = temp;

		logger_add_str(app->logger, TAG, false, httpd.response);
	}
	if (httpd.response)
		vPortFree(httpd.response);

	// -> /api/info
	strjson_init(request);
	strjson_str(request, "uid", app->params->mcu_uid); // ?
	strjson_uint(request, "ts", *app->timestamp);
	strjson_str(request, "name", PARAMS_DEVICE_NAME);
	strjson_str(request, "bl_git", (char *) app->bl->hash);
	strjson_uint(request, "bl_status", app->bl->status);
	strjson_str(request, "app_git", GIT_COMMIT_HASH);
	strjson_uint(request, "app_ver", PARAMS_FW_VERSION);
	strjson_str(request, "mcu", app->params->mcu_uid);
	strjson_str(request, "apn", app->params->apn);
	strjson_str(request, "url_ota", app->params->url_ota);
	strjson_str(request, "url_app", app->params->url_app);
	strjson_uint(request, "period", app->params->period);

	strcpy(url, app->params->url_app);
	strcat(url, "/api/info");
	httpd.url = url;
	httpd.request = request;
	httpd.response = NULL;

	sim800l_http(app->mod, &httpd, http_callback, HTTP_TIMEOUT_2MIN);
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	if (httpd.response)
		vPortFree(httpd.response);

	for (;;)
	{
		// TODO: Replace
		char strcnt[32];
		struct count_item item;
		while (xQueueReceive(app->cntq->queue, &item, 0) == pdTRUE )
		{
			utoa(item.timestamp, strcnt, 10);
			logger_add_str(app->logger, "CNT:timestamp", false, strcnt);

			utoa(item.count, strcnt, 10);
			logger_add_str(app->logger, "CNT:count", false, strcnt);
		}

		sim800l_voltage(app->mod, &vd, voltage_callback, 60000);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if (status)
			vd.voltage = 0;

		if (meas_temp)
			ret = tmpx75_read_temp(app->tmp, &temperature);

		// -> /api/data
		strjson_init(request);
		strjson_str(request, "uid", app->params->mcu_uid); // ?
		strjson_uint(request, "ts", *app->timestamp);
		strjson_uint(request, "ticks", xTaskGetTickCount());
		strjson_int(request, "bat", vd.voltage);
		if (meas_temp && !ret)
			strjson_int(request, "temp", temperature);
		else
			strjson_null(request, "temp");
		strjson_null(request, "count");

		strcpy(url, app->params->url_app);
		strcat(url, "/api/data");
		httpd.url = url;
		httpd.request = request;
		httpd.response = NULL;

		sim800l_http(app->mod, &httpd, http_callback, HTTP_TIMEOUT_1MIN);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

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
