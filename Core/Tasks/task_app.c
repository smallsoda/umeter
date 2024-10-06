/*
 * Application task
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "umeter_tasks.h"

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

// TODO: Remove
static void info(struct app *app)
{
	const char *header_bl  = " BL: ";
	const char *header_app = "APP: ";
	char *buf;

	buf = pvPortMalloc(strlen(header_bl) +
			strlen((char *) app->bl->datetime) + 3);
	if (!buf)
		for(;;);
	strcpy(buf, header_bl);
	strcat(buf, (char *) app->bl->datetime);
	strcat(buf, "\r\n");
	xQueueSendToBack(app->logger->queue, &buf, 0);

	buf = pvPortMalloc(strlen(header_bl) + strlen((char *) app->bl->hash) + 3);
	if (!buf)
		for(;;);
	strcpy(buf, header_bl);
	strcat(buf, (char *) app->bl->hash);
	strcat(buf, "\r\n");
	xQueueSendToBack(app->logger->queue, &buf, 0);

	buf = pvPortMalloc(strlen(header_app) + strlen(PARAMS_TIMESTAMP) + 3);
	if (!buf)
		for(;;);
	strcpy(buf, header_app);
	strcat(buf, PARAMS_TIMESTAMP);
	strcat(buf, "\r\n");
	xQueueSendToBack(app->logger->queue, &buf, 0);

	buf = pvPortMalloc(strlen(header_app) + strlen(GIT_COMMIT_HASH) + 3);
	if (!buf)
		for(;;);
	strcpy(buf, header_app);
	strcat(buf, GIT_COMMIT_HASH);
	strcat(buf, "\r\n");
	xQueueSendToBack(app->logger->queue, &buf, 0);

	buf = pvPortMalloc(strlen(header_app) + strlen(PARAMS_DEVICE_NAME) + 3);
	if (!buf)
		for(;;);
	strcpy(buf, header_app);
	strcat(buf, PARAMS_DEVICE_NAME);
	strcat(buf, "\r\n");
	xQueueSendToBack(app->logger->queue, &buf, 0);

	buf = pvPortMalloc(strlen(header_app) + 32 + 3);
	if (!buf)
		for(;;);
	strcpy(buf, header_app);
	itoa(PARAMS_FW_VERSION, &buf[strlen(buf)], 10);
	strcat(buf, "\r\n");
	xQueueSendToBack(app->logger->queue, &buf, 0);
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

	info(app);

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
