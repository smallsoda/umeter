/*
 * Application interface
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "appiface.h"

#include <string.h>
#include <stdlib.h>
#include "cmsis_os.h"
#include "main.h"

#include "strjson.h"
#include "siface.h"
#include "fws.h"

#define JSMN_HEADER
#include "jsmn.h"

#define JSON_MAX_TOKENS 8

#define APPIFACE_KEY "iface-appum-0.1"


// https://github.com/zserge/jsmn/blob/master/example/simple.c
static int jsoneq(const char *json, jsmntok_t *tok, const char *s)
{
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0)
		return 0;
	return -1;
}

static int parse(struct appiface *appif, const char *request, char *response)
{
	jsmn_parser parser;
	jsmntok_t tokens[JSON_MAX_TOKENS];
	jsmntok_t *tcmd = NULL;
	jsmntok_t *tparam = NULL;
	jsmntok_t *tvalue = NULL;
	uint32_t tmp;
	size_t len;
	int ret;

	memset(tokens, 0, sizeof(tokens));
	jsmn_init(&parser);
	ret = jsmn_parse(&parser, request, strlen(request), tokens,
			JSON_MAX_TOKENS);

	if (ret <= 0)
		return -1;

	if (tokens[0].type != JSMN_OBJECT)
		return -1;

	for (int i = 1; (i + 1) < ret; i += 2)
	{
		if (jsoneq(request, &tokens[i], "cmd") == 0)
			tcmd = &tokens[i + 1];
		else if (jsoneq(request, &tokens[i], "param") == 0)
			tparam = &tokens[i + 1];
		else if (jsoneq(request, &tokens[i], "value") == 0)
			tvalue = &tokens[i + 1];
	}

	/* Commands */
	if (!tcmd)
		return -1;

	if (jsoneq(request, tcmd, "iface") == 0)
	{
		strjson_str(response, "iface", APPIFACE_KEY);
	}
	else if (jsoneq(request, tcmd, "rd_param") == 0)
	{
		if (!tparam)
			return -1;

		if (jsoneq(request, tparam, "uid") == 0)
			strjson_str(response, "uid", appif->params->mcu_uid);
		else if (jsoneq(request, tparam, "ts") == 0)
			strjson_uint(response, "ts", *appif->timestamp);
		else if (jsoneq(request, tparam, "ticks") == 0)
			strjson_uint(response, "ticks", xTaskGetTickCount());
		else if (jsoneq(request, tparam, "name") == 0)
			strjson_str(response, "name", PARAMS_DEVICE_NAME);
		else if (jsoneq(request, tparam, "bl_git") == 0)
			strjson_str(response, "bl_git", (char *) appif->bl->hash);
		else if (jsoneq(request, tparam, "bl_status") == 0)
			strjson_uint(response, "bl_status", appif->bl->status);
		else if (jsoneq(request, tparam, "app_git") == 0)
			strjson_str(response, "app_git", GIT_COMMIT_HASH);
		else if (jsoneq(request, tparam, "app_ver") == 0)
			strjson_uint(response, "app_ver", PARAMS_FW_VERSION);
		else if (jsoneq(request, tparam, "mcu") == 0)
			strjson_str(response, "mcu", appif->params->mcu_uid);
		else if (jsoneq(request, tparam, "apn") == 0)
			strjson_str(response, "apn", appif->uparams.apn);
		else if (jsoneq(request, tparam, "url_ota") == 0)
			strjson_str(response, "url_ota", appif->uparams.url_ota);
		else if (jsoneq(request, tparam, "url_app") == 0)
			strjson_str(response, "url_app", appif->uparams.url_app);
		else if (jsoneq(request, tparam, "period") == 0)
			strjson_uint(response, "period", appif->uparams.period);
		else if (jsoneq(request, tparam, "bat") == 0)
		{
			xSemaphoreTake(appif->actual->mutex, portMAX_DELAY);
			strjson_uint(response, "bat", appif->actual->voltage);
			xSemaphoreGive(appif->actual->mutex);
		}
		else if (jsoneq(request, tparam, "count") == 0)
		{
			xSemaphoreTake(appif->actual->mutex, portMAX_DELAY);
			strjson_uint(response, "count", appif->actual->count);
			xSemaphoreGive(appif->actual->mutex);
		}
		else if (jsoneq(request, tparam, "temp") == 0)
		{
			xSemaphoreTake(appif->actual->mutex, portMAX_DELAY);
			strjson_int(response, "temp", appif->actual->temperature);
			xSemaphoreGive(appif->actual->mutex);
		}
		else if (jsoneq(request, tparam, "hum") == 0)
		{
			xSemaphoreTake(appif->actual->mutex, portMAX_DELAY);
			strjson_int(response, "hum", appif->actual->humidity);
			xSemaphoreGive(appif->actual->mutex);
		}
		else
			return -1;
	}
	else if (jsoneq(request, tcmd, "wr_param") == 0)
	{
		if (!tparam || !tvalue)
			return -1;

		len = tvalue->end - tvalue->start;

		if (jsoneq(request, tparam, "apn") == 0)
		{
			strncpy(appif->uparams.apn, request + tvalue->start,
					(sizeof(appif->uparams.apn) - 1) < len ?
					(sizeof(appif->uparams.apn) - 1) : len);
			appif->uparams.apn[len] = '\0';
		}
		else if (jsoneq(request, tparam, "url_ota") == 0)
		{
			strncpy(appif->uparams.url_ota, request + tvalue->start,
					(sizeof(appif->uparams.url_ota) - 1) < len ?
					(sizeof(appif->uparams.url_ota) - 1) : len);
			appif->uparams.url_ota[len] = '\0';
		}
		else if (jsoneq(request, tparam, "url_app") == 0)
		{
			strncpy(appif->uparams.url_app, request + tvalue->start,
					(sizeof(appif->uparams.url_app) - 1) < len ?
					(sizeof(appif->uparams.url_app) - 1) : len);
			appif->uparams.url_app[len] = '\0';
		}
		else if (jsoneq(request, tparam, "period") == 0)
		{
			tmp = strtoul(request + tvalue->start, NULL, 10);
			if (tmp == 0)
				return -1;

			appif->uparams.period = tmp;
		}
		else
		{
			return -1;
		}
	}
	else if (jsoneq(request, tcmd, "save") == 0)
	{
		vTaskSuspendAll();
		params_set(&appif->uparams);

		NVIC_SystemReset(); // --> RESET
	}
	else if (jsoneq(request, tcmd, "reset") == 0)
	{
		NVIC_SystemReset(); // --> RESET
	}

	strjson_str(response, "status", "ok");
	return 0;
}

int appiface(void *iface)
{
	struct siface *siface = iface;
	struct appiface *appif = siface->context;
	char *response;
	char *tmp;
	int ret;

	HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_4); // TODO: Remove

	tmp = pvPortMalloc(256); // NOTE: ?
	if (!tmp)
		return -1;

	strcpy(tmp, "@,,");
	response = tmp + strlen(tmp);
	strjson_init(response);

	ret = parse(appif, siface->buf, response);
	if (ret < 0)
		strjson_str(response, "status", "error");

	strcat(response, "\r\n");

	ret = siface_add(siface, tmp);
	if (ret < 0)
	{
		vPortFree(tmp);
		return -1;
	}

	return 0;
}
