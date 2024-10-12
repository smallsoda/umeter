/*
 * Blink task
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "ptasks.h"
#include "main.h"

static osThreadId_t handle;
static const osThreadAttr_t attributes = {
  .name = "system",
  .stack_size = 64 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};

static void task(void *argument)
{
	IWDG_HandleTypeDef *wdg = argument;

	for(;;)
	{
		// 40kH / 128 / 4095 -> 13,104 seconds
		HAL_IWDG_Refresh(wdg);
		osDelay(5000);
	}
}

/******************************************************************************/
void task_system(IWDG_HandleTypeDef *wdg)
{
	handle = osThreadNew(task, wdg, &attributes);
}

