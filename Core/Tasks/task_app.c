/*
 * Application task
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024-2025
 */

#include "ptasks.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define JSMN_HEADER
#include "jsmn.h"

#include "strjson.h"
#include "base64.h"
#include "params.h"
#include "queue.h"
#include "main.h"
#include "hmac.h"
#include "fws.h"

#include "logger.h"
#define TAG "APP"

#define JSON_MAX_TOKENS 8

#define MAX_QTY_FROM_QUEUE 4

#define NET_LEV_MIN -113

#define SENSORS_BUF_LEN (MAX_QTY_FROM_QUEUE * sizeof(struct item))
#define SENSORS_STR_LEN (4 * ((SENSORS_BUF_LEN + 2) / 3) + 1)

#define HTTP_TIMEOUT_1MIN 60000
#define HTTP_TIMEOUT_2MIN 120000

#define NETSCAN_TIMEOUT_1MIN 60000

#define READ_TAMPER (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15)) // TODO

static osThreadId_t handle;
static const osThreadAttr_t attributes = {
  .name = "app",
  .stack_size = 288 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

struct netprms
{
	int32_t mcc;
	int32_t mnc;
	int32_t lac;
	int32_t cid;
	int32_t lev;
};


//static void voltage_callback(int status, void *data)
//{
//	struct sim800l_voltage *vd = data;
//	BaseType_t woken = pdFALSE;
//
//	*((int *) vd->context) = status;
//
//	vTaskNotifyGiveFromISR(handle, &woken);
//}

static void http_callback(int status, void *data)
{
	struct sim800l_http *httpd = data;
	BaseType_t woken = pdFALSE;

	*((int *) httpd->context) = status;

	vTaskNotifyGiveFromISR(handle, &woken);
}

static void netscan_callback(int status, void *data)
{
	struct sim800l_netscan *netscan = data;
	struct netprms *prms = netscan->context;
	BaseType_t woken = pdFALSE;

	if (netscan->lev > prms->lev)
	{
		prms->mcc = netscan->mcc;
		prms->mnc = netscan->mnc;
		prms->lac = netscan->lac;
		prms->cid = netscan->cid;
		prms->lev = netscan->lev;
	}

	if (status != 0)
		vTaskNotifyGiveFromISR(handle, &woken);
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

static void sensor_base64(mqueue_t *queue, char *buf)
{
	struct item item[MAX_QTY_FROM_QUEUE];
	size_t enclen;
	int i = 0;
	int ret;

	for (;;)
	{
		if (i >= MAX_QTY_FROM_QUEUE)
			break;

		ret = mqueue_get(queue, &item[i]);
		if (ret == -MFIFO_ERR_INVALID_CRC)
			continue;
		if (ret)
			break;

		i++;
	}

	base64_encode((unsigned char *) item, sizeof(struct item) * i, buf,
			&enclen);
	buf[enclen] = '\0';
}

static void strtolower(char *data)
{
	while (*data)
	{
		*data = tolower(*data);
		data++;
	}
}

static void task(void *argument)
{
	struct app *app = argument;

//	struct sim800l_voltage vd;
	struct sim800l_netscan netscan;
	struct sim800l_http httpd;
	struct netprms netprms;
	TickType_t wake;

	char *request;
	char *sensor;
	char *hmac;
	char *url;

	int voltage;
	int status;
	int avail;
	int ret;

	// TODO: free all resources before vTaskDelete()

	request = pvPortMalloc(512); // NOTE: ?
	if (!request)
		vTaskDelete(NULL);

	sensor = pvPortMalloc(SENSORS_STR_LEN);
	if (!sensor)
		vTaskDelete(NULL);

	hmac = pvPortMalloc(HMAC_BASE64_LEN);
	if (!hmac)
		vTaskDelete(NULL);

	url = pvPortMalloc(PARAMS_APP_URL_SIZE + 32);
	if (!request)
		vTaskDelete(NULL);

	memset(&netprms, 0, sizeof(netprms));
	netprms.lev = NET_LEV_MIN;

//	vd.context = &status;
	httpd.context = &status;
	netscan.context = &netprms;

	// <- /api/time
	strcpy(url, app->params->url_app);
	strcat(url, "/api/time");
	httpd.url = url;
	httpd.req_auth = NULL;
	httpd.res_auth = NULL; // Unnecessary
	httpd.res_auth_get = true;
	httpd.request = NULL;
	httpd.response = NULL; // Unnecessary

	sim800l_http(app->mod, &httpd, http_callback, HTTP_TIMEOUT_2MIN);
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	if (!status && httpd.res_auth)
	{
		uint32_t temp;

		// NOTE: Ignore case of characters
		hmac_base64(app->params->secret, httpd.response, httpd.rlen, hmac);
		strtolower(hmac);
		if (strcmp(hmac, httpd.res_auth) == 0)
		{
			ret = parse_json_time(httpd.response, &temp);
			if (!ret)
				*app->timestamp = temp;

			logger_add_str(app->logger, TAG, false, httpd.response);
		}
	}
	if (httpd.res_auth)
		vPortFree(httpd.res_auth);
	if (httpd.response)
		vPortFree(httpd.response);

	// Available sensors
	xSemaphoreTake(app->sens->actual->mutex, portMAX_DELAY);
	avail = app->sens->actual->avail;
	xSemaphoreGive(app->sens->actual->mutex);

	// -> /api/info
	strjson_init(request);
	strjson_uint(request, "uid", app->params->id);
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
	strjson_uint(request, "period_app", app->params->period_app);
	strjson_uint(request, "period_sen", app->params->period_sen);
	strjson_uint(request, "mtime_count", app->params->mtime_count);
	strjson_int(request, "sens", avail);

	strcpy(url, app->params->url_app);
	strcat(url, "/api/info");
	hmac_base64(app->params->secret, request, strlen(request), hmac);

	httpd.url = url;
	httpd.req_auth = hmac;
	httpd.res_auth = NULL; // Unnecessary
	httpd.res_auth_get = false;
	httpd.request = request;
	httpd.response = NULL; // Unnecessary

	sim800l_http(app->mod, &httpd, http_callback, HTTP_TIMEOUT_2MIN);
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	if (httpd.response)
		vPortFree(httpd.response);

	// Net scan
	sim800l_netscan(app->mod, &netscan, netscan_callback, NETSCAN_TIMEOUT_1MIN);
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	// -> /api/cnet
	strcpy(url, app->params->url_app);
	strcat(url, "/api/cnet");
	strjson_init(request);
	strjson_uint(request, "uid", app->params->id);
	strjson_uint(request, "ts", *app->timestamp);
	strjson_int(request, "mcc", netprms.mcc);
	strjson_int(request, "mnc", netprms.mnc);
	strjson_int(request, "lac", netprms.lac);
	strjson_int(request, "cid", netprms.cid);
	strjson_int(request, "lev", netprms.lev);

	hmac_base64(app->params->secret, request, strlen(request), hmac);
	httpd.url = url;
	httpd.req_auth = hmac;
	httpd.res_auth = NULL; // Unnecessary
	httpd.res_auth_get = false;
	httpd.request = request;
	httpd.response = NULL; // Unnecessary

	sim800l_http(app->mod, &httpd, http_callback, HTTP_TIMEOUT_1MIN);
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	if (httpd.response)
		vPortFree(httpd.response);

	wake = xTaskGetTickCount();

	for (;;)
	{
//		// Voltage
//		sim800l_voltage(app->mod, &vd, voltage_callback, 60000);
//		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
//		if (status)
//		{
//			vd.voltage = 0;
//		}
//		else
//		{
//			xSemaphoreTake(app->sens->actual->mutex, portMAX_DELAY);
//			app->sens->actual->voltage = vd.voltage;
//			xSemaphoreGive(app->sens->actual->mutex);
//		}

		// Voltage and distance
		xSemaphoreTake(app->sens->actual->mutex, portMAX_DELAY);
		voltage = app->sens->actual->voltage;
		xSemaphoreGive(app->sens->actual->mutex);

		// -> /api/data
		strjson_init(request);
		strjson_uint(request, "uid", app->params->id);
		strjson_uint(request, "ts", *app->timestamp);
		strjson_uint(request, "ticks", xTaskGetTickCount());
		if (voltage)
			strjson_int(request, "bat", voltage);
		sensor_base64(app->ecnt->qec_avg, sensor); /* Counter [avg] */
		if (*sensor)
			strjson_str(request, "count", sensor);
		sensor_base64(app->ecnt->qec_min, sensor); /* Counter [min] */
		if (*sensor)
			strjson_str(request, "count_min", sensor);
		sensor_base64(app->ecnt->qec_max, sensor); /* Counter [max] */
		if (*sensor)
			strjson_str(request, "count_max", sensor);
		sensor_base64(app->sens->qtmp, sensor); /* Temperature */
		if (*sensor)
			strjson_str(request, "temp", sensor);
		sensor_base64(app->sens->qhum, sensor); /* Humidity */
		if (*sensor)
			strjson_str(request, "hum", sensor);
		strjson_int(request, "tamper", READ_TAMPER);

		strcpy(url, app->params->url_app);
		strcat(url, "/api/data");
		hmac_base64(app->params->secret, request, strlen(request), hmac);
		httpd.url = url;
		httpd.req_auth = hmac;
		httpd.res_auth = NULL; // Unnecessary
		httpd.res_auth_get = false;
		httpd.request = request;
		httpd.response = NULL; // Unnecessary

		sim800l_http(app->mod, &httpd, http_callback, HTTP_TIMEOUT_1MIN);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if (httpd.response)
			vPortFree(httpd.response);

		// Are all queues empty?
		if (!mqueue_is_empty(app->ecnt->qec_avg))
			continue; /* for */
		if (!mqueue_is_empty(app->ecnt->qec_min))
			continue; /* for */
		if (!mqueue_is_empty(app->ecnt->qec_max))
			continue; /* for */
		if (!mqueue_is_empty(app->sens->qtmp))
			continue; /* for */
		if (!mqueue_is_empty(app->sens->qhum))
			continue; /* for */

		blink();
		vTaskDelayUntil(&wake, pdMS_TO_TICKS(app->params->period_app * 1000));
	}
}

/******************************************************************************/
void task_app(struct app *app)
{
	handle = osThreadNew(task, app, &attributes);
}
