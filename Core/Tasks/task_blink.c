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
  .name = "blink",
  .stack_size = 64 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static void task(void *argument)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);

	for(int i = 0; i < 10; i++)
	{
		osDelay(200);
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_4);
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);
	}

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);

	for(;;)
	{
		osDelay(1);
	}
}

/******************************************************************************/
void task_blink(void)
{
	handle = osThreadNew(task, NULL, &attributes);
}

