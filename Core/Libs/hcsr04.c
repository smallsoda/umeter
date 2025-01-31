/*
 * HC-SR04 ultrasonic sensor
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2025
 */

#include "hcsr04.h"

#include <string.h>

#define CLOCK_US (SystemCoreClock / 1000000)
#define COEFF_D1 10
#define COEFF_D2 58

/******************************************************************************/
void hcsr04_init(struct hcsr04 *sen, DWT_Type *dwt, GPIO_TypeDef *trig_port,
		uint16_t trig_pin)
{
	memset(sen, 0, sizeof(*sen));
	sen->sema = xSemaphoreCreateBinary();
	sen->dwt = dwt;
	sen->trig_port = trig_port;
	sen->trig_pin = trig_pin;

	xSemaphoreTake(sen->sema, 0);
}

/******************************************************************************/
void hcsr04_irq(struct hcsr04 *sen)
{
	BaseType_t woken = pdFALSE;

	if (!sen->sstatus)
	{
		sen->dwt->CYCCNT = 0;
		sen->sstatus = true;
	}
	else
	{
		sen->count = sen->dwt->CYCCNT;
		xSemaphoreGiveFromISR(sen->sema, &woken);
	}
}

static void delay(DWT_Type *dwt, int us)
{
	int ticks = us * CLOCK_US;

	dwt->CYCCNT = 0;
	while (dwt->CYCCNT < ticks);
}

/******************************************************************************/
int hcsr04_read(struct hcsr04 *sen)
{
	BaseType_t status;

	sen->sstatus = false;

	taskENTER_CRITICAL();

	HAL_GPIO_WritePin(sen->trig_port, sen->trig_pin, GPIO_PIN_SET);
	delay(sen->dwt, 10);
	HAL_GPIO_WritePin(sen->trig_port, sen->trig_pin, GPIO_PIN_RESET);

	taskEXIT_CRITICAL();

	status = xSemaphoreTake(sen->sema, pdMS_TO_TICKS(50));
	if (!status)
		return -1;

	return sen->count * COEFF_D1 / CLOCK_US / COEFF_D2;
}
