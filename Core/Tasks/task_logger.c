/*
 * Logger task
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "umeter_tasks.h"

static osThreadId_t handle;
static const osThreadAttr_t attributes = {
  .name = "logger",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static void task(void *argument)
{
	struct logger *logger = argument;
	logger_task(logger); // <- Infinite loop
}

/******************************************************************************/
void task_logger(struct logger *logger)
{
	handle = osThreadNew(task, logger, &attributes);
}
