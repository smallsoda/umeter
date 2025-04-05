/*
 * "mfifo" based FLASH storage queue
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2025
 */

#include "mqueue.h"

#include <string.h>

#include "cmsis_os.h"
#include "semphr.h"

#include "fws.h"

extern const uint32_t *_app_len;
#define APP_LENGTH ((uint32_t) &_app_len)

#define APP_END_ADDR     (FWS_PAYLOAD_ADDR + APP_LENGTH)
#define APP_END_SEC_ADDR ((APP_END_ADDR / W25Q_SECTOR_SIZE) * W25Q_SECTOR_SIZE)
//#define FIFO_ADDR        (APP_END_SEC + ((APP_LENGTH % W25Q_SECTOR_SIZE) && 1)) // TODO


static SemaphoreHandle_t mutex = NULL;
static struct w25q_s *memory = NULL;
static size_t address = 0;
static size_t msize = 0;


static int read(uint32_t addr, uint8_t *data, size_t len)
{
	w25q_s_read_data(memory, addr, data, len);
	return 0;
}

static int write(uint32_t addr, const uint8_t *data, size_t len)
{
	w25q_s_write_data(memory, addr, (uint8_t *) data, len);
	return 0;
}

static int erase(uint32_t addr)
{
	w25q_s_sector_erase(memory, addr);
	return 0;
}

/******************************************************************************/
void mqueue_init(struct w25q_s *mem)
{
	memory = mem;
	mutex = xSemaphoreCreateMutex();
	msize = w25q_s_get_capacity(memory);

	address = APP_END_SEC_ADDR;
	if (APP_LENGTH % W25Q_SECTOR_SIZE)
		address += W25Q_SECTOR_SIZE;
}

inline static mqueue_t *create(size_t secnum)
{
	size_t len = secnum * W25Q_SECTOR_SIZE;
	mqueue_t *q;
	int ret;

	if ((address + len) > msize)
		return NULL;

	q = pvPortMalloc(sizeof(mqueue_t));
	if (!q)
		return NULL;

	ret = mfifo_init(&q->mfifo, address, secnum, read, write, erase);
	if (ret)
	{
		vPortFree(q);
		return NULL;
	}

	address += len;
	return q;
}

/******************************************************************************/
mqueue_t *mqueue_create(size_t secnum)
{
	mqueue_t *q;

	xSemaphoreTake(mutex, portMAX_DELAY);
	q = create(secnum);
	xSemaphoreGive(mutex);

	return q;
}

/******************************************************************************/
int mqueue_set(mqueue_t *queue, const void *element)
{
	if (!queue)
		return -MFIFO_ERR_INVALID_ARGUMENT;
	return mfifo_set(&queue->mfifo, element);
}

/******************************************************************************/
int mqueue_get(mqueue_t *queue, void *element)
{
	if (!queue)
		return -MFIFO_ERR_INVALID_ARGUMENT;
	return mfifo_get(&queue->mfifo, element);
}

/******************************************************************************/
int mqueue_is_empty(mqueue_t *queue)
{
	if (!queue)
		return -MFIFO_ERR_INVALID_ARGUMENT;
	return mfifo_is_empty(&queue->mfifo);
}
