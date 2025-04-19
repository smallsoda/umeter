/*
 * Blink task
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024-2025
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
	struct system *sys = argument;

	for(;;)
	{
		// 40kH / 128 / 4095 -> 13,104 seconds
		HAL_IWDG_Refresh(sys->wdg);

		// External watchdog (CBM706TAS8, ADM708TARZ) -> 1,6 seconds
		HAL_GPIO_TogglePin(sys->ext_port, sys->ext_pin);

		osDelay(1000);
	}
}

/******************************************************************************/
void task_system(struct system *sys)
{
	handle = osThreadNew(task, sys, &attributes);
}

