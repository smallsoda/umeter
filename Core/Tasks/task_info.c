/*
 * System information task
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "ptasks.h"

#include <stdlib.h>
#include <string.h>
#include "stm32f1xx_hal.h"
#include "task.h"

#include "params.h"
#include "fws.h"

static osThreadId_t handle;
static const osThreadAttr_t attributes = {
  .name = "info",
  .stack_size = 96 * 4,
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
	strcat(buf, "\r\n");

	xQueueSendToBack(logger->queue, &buf, 0);
}

static void uid_string(char *uid)
{
	char temp[16];

	itoa(HAL_GetUIDw0(), temp, 16);
	strcpy(uid, temp);
	itoa(HAL_GetUIDw1(), temp, 16);
	strcat(uid, temp);
	itoa(HAL_GetUIDw2(), temp, 16);
	strcat(uid, temp);
}

static void info(struct app *app)
{
	char temp[32];

	print_info_str(app->logger, "\r\n\r\ninfo:", "", "");

	print_info_str(app->logger, "  BL", "-", (char *) app->bl->datetime);
	print_info_str(app->logger, "  BL", "-", (char *) app->bl->hash);
	itoa(app->bl->status, temp, 10);
	print_info_str(app->logger, "  BL", "-", temp);

	print_info_str(app->logger, " APP", "-", PARAMS_TIMESTAMP);
	print_info_str(app->logger, " APP", "-", GIT_COMMIT_HASH);
	print_info_str(app->logger, " APP", "-", PARAMS_DEVICE_NAME);
	itoa(PARAMS_FW_VERSION, temp, 10);
	print_info_str(app->logger, " APP", "-", temp);
	uid_string(temp);
	print_info_str(app->logger, " APP", "-", temp);
}

static void task(void *argument)
{
	const char *t_names[] = {"defaultTask", "app", "blink", "info", "logger",
			"ota", "sim800l", NULL};
	TaskHandle_t t_handle;
	TaskStatus_t details;
	char temp[16];

	struct app *app = argument;

	info(app);

	for (;;)
	{
		osDelay(20000);

		itoa(xPortGetMinimumEverFreeHeapSize(), temp, 10);
		print_info_str(app->logger, "HEAP", "heap", temp);

		int idx = 0;
		while (t_names[idx])
		{
			t_handle = xTaskGetHandle(t_names[idx]);
			if (t_handle)
			{
				vTaskGetInfo(t_handle, &details, pdTRUE, eInvalid);
				itoa(details.usStackHighWaterMark * sizeof(StackType_t),
						temp, 10);
				print_info_str(app->logger, "STACK", t_names[idx], temp);
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
