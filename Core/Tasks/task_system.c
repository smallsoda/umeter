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


static void blink(void)
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
}

static void task(void *argument)
{
	IWDG_HandleTypeDef *wdg = argument;

	blink();

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

