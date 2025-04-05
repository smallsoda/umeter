/*
 * UART firmware updater
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "updater.h"

#include <string.h>
#include "fws.h"


/******************************************************************************/
void updater_init(struct updater *upd, struct w25q_s *mem,
		UART_HandleTypeDef *uart)
{
	memset(upd, 0, sizeof(*upd));
	upd->mem = mem;
	upd->uart = uart;
	upd->stream = xStreamBufferCreate(sizeof(struct updater_packet), 1);
}

/******************************************************************************/
void updater_irq(struct updater *upd, const char *buf, size_t len)
{
	BaseType_t woken = pdFALSE;
	xStreamBufferSendFromISR(upd->stream, buf, len, &woken);
}

inline static void transmit(struct updater *upd, int32_t ans)
{
	// Clear stream buffer to receive data after transmission again
	xStreamBufferReset(upd->stream);

	while (HAL_UART_Transmit(upd->uart, (uint8_t *) &ans, sizeof(int32_t),
			HAL_MAX_DELAY) == HAL_BUSY);
}

/******************************************************************************/
void updater_task(struct updater *upd)
{
	struct updater_packet packet;
	uint32_t address = 0;
	uint32_t sum;
	size_t rec;

	for(;;)
	{
		rec = xStreamBufferReceive(upd->stream, &packet, sizeof(packet),
				portMAX_DELAY);

		if (rec < sizeof(uint32_t))
		{
			transmit(upd, -1);
			continue;
		}

		switch ((enum updater_cmd) packet.cmd)
		{
		case UPDATER_CMD_START:
			address = 0;
			transmit(upd, 0);
			continue;
			break;

		case UPDATER_CMD_DATA:
			break;

		case UPDATER_CMD_RESET:
			transmit(upd, 0);
			NVIC_SystemReset(); // --> BOOTLOADER
			break;

		default:
			transmit(upd, -1);
			continue;
			break;
		}

		// UPDATER_CMD_DATA
		// -->

		if (rec != sizeof(packet))
		{
			transmit(upd, -1);
			continue;
		}

		sum = FWS_CHECKSUM_INIT;
		for (int i = 0; i < UPDATER_PAYLOAD_SIZE; i++)
			sum += packet.payload[i];

		if (sum != packet.checksum)
		{
			transmit(upd, -1);
			continue;
		}

		if ((address & (W25Q_SECTOR_SIZE - 1)) == 0)
		{
			w25q_s_sector_erase(upd->mem, address);
		}

		w25q_s_write_data(upd->mem, address, (uint8_t *) packet.payload,
				UPDATER_PAYLOAD_SIZE * sizeof(uint32_t));

		transmit(upd, address);

		address += UPDATER_PAYLOAD_SIZE * sizeof(uint32_t);
	}
}
