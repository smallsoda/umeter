/*
 * Over-the-air (OTA) firmware update
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#ifndef OTA_H_
#define OTA_H_

#include "cmsis_os.h"
#include "task.h"

#include "sim800l.h"
#include "hmac.h"
#include "w25q.h"

#define OTA_URL_SIZE 64

struct ota
{
	uint8_t secret[HMAC_SECRET_SIZE];
	char url[OTA_URL_SIZE];
	struct sim800l *mod;
	struct w25q *mem;

	TaskHandle_t task;
};

void ota_init(struct ota *ota, struct sim800l *mod, struct w25q *mem,
		const uint8_t *secret, const char *url);
void ota_task(struct ota *ota);

#endif /* OTA_H_ */
