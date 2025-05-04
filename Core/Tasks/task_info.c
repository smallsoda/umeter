/*
 * System information task
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024-2025
 */

#include "ptasks.h"

#include <stdlib.h>
#include <string.h>
#include "stm32f1xx_hal.h"
#include "task.h"

#include "params.h"
#include "fws.h"

#include "logger.h"
#define TAG "INFO"
extern struct logger logger;

static osThreadId_t handle;
static const osThreadAttr_t attributes = {
  .name = "info",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};


static void print_info_str(struct logger *logger, const char *header,
		const char *sub, const char *data)
{
	char *buf = pvPortMalloc(strlen(header) + 1 + strlen(sub) + 1 +
			strlen(data) + 2 + 1);
	if (!buf)
		return;

	strcpy(buf, header);
	strcat(buf, " ");
	strcat(buf, sub);
	strcat(buf, " ");
	strcat(buf, data);

	logger_add_str(logger, TAG, false, buf);
	vPortFree(buf);
}

static void info(struct app *app)
{
	char temp[32];

	print_info_str(&logger, "BL", "git", (char *) app->bl->hash);
	utoa(app->bl->status, temp, 10);
	print_info_str(&logger, "BL", "status", temp);

	print_info_str(&logger, "APP", "git", GIT_COMMIT_HASH);
	print_info_str(&logger, "APP", "name", PARAMS_DEVICE_NAME);
	utoa(PARAMS_FW_VERSION, temp, 10);
	print_info_str(&logger, "APP", "ver", temp);
	print_info_str(&logger, "APP", "MCU", app->params->mcu_uid);
}

static void task(void *argument)
{
	const char *t_names[] = {"system", "def", "app", "siface", "info", "ota",
			"sim800l", "sensors", "ecounter", NULL};
	TaskHandle_t t_handle;
	TaskStatus_t details;
	char temp[16];

	struct app *app = argument;

	info(app);

	for (;;)
	{
		osDelay(20000);

		utoa(xPortGetMinimumEverFreeHeapSize(), temp, 10);
		print_info_str(&logger, "HEAP", "heap", temp);

		int idx = 0;
		while (t_names[idx])
		{
			t_handle = xTaskGetHandle(t_names[idx]);
			if (t_handle)
			{
				vTaskGetInfo(t_handle, &details, pdTRUE, eInvalid);
				utoa(details.usStackHighWaterMark * sizeof(StackType_t),
						temp, 10);
				print_info_str(&logger, "STACK", t_names[idx], temp);
			}
			idx++;
		}
	}
}

/******************************************************************************/
void task_info(struct app *app)
{
	handle = osThreadNew(task, app, &attributes);
}
