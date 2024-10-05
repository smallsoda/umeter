/*
 * Parameters (FLASH storage)
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#ifndef PARAMS_H_
#define PARAMS_H_

#include "sim800l.h"
#include "ota.h"

#if __has_include("gitcommit.h")
#include "gitcommit.h"
#else
#define GIT_COMMIT_HASH "-"
#endif

#define PARAMS_DEVICE_NAME "umeter-a1"
#define PARAMS_FW_VERSION 3

#define PARAMS_MAGIC_EMPTY 0xFFFFFFFF
#define PARAMS_MAGIC_VALID 0xAA550003

typedef struct
{
	uint32_t magic;
	uint32_t id;
	/* Do not edit items above */

	char apn[SIM800L_APN_SIZE];
	char url_ota[OTA_URL_SIZE];
} __attribute__((aligned(8))) params_t;

void params_get(params_t *params);
void params_set(params_t *params);
void params_init(void);

#endif /* PARAMS_H_ */
