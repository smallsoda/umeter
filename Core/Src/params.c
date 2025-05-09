/*
 * Parameters (FLASH storage)
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024-2025
 */

#include <params.h>
#include <string.h>
#include <stdlib.h>
#include "stm32f1xx_hal.h"

extern const uint32_t *_storage;
//#define PARAMS_ADDRESS ((uint32_t) &_storage)
static uint32_t PARAMS_ADDRESS = ((uint32_t) &_storage);


inline static void flash_get(uint32_t address, size_t size, uint8_t *buffer)
{
	memcpy(buffer, (uint8_t *) address, size);
}

static void flash_set(uint32_t address, size_t size, uint8_t *buffer)
{
	// Alignment: 8
	if (size & 0x07)
		size = size + 0x08 - (size & 0x07);

	HAL_FLASH_Unlock();
	while (size)
	{
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, *((uint64_t *) buffer));
		size -= 8;
		address += 8;
		buffer += 8;
	}
	HAL_FLASH_Lock();
}

static void flash_erase(uint32_t address)
{
	uint32_t page_error;
	FLASH_EraseInitTypeDef erase_init;
	erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
	erase_init.Banks = FLASH_BANK_1;
	erase_init.PageAddress = address;
	erase_init.NbPages = 1;

	HAL_FLASH_Unlock();
	HAL_FLASHEx_Erase(&erase_init, &page_error);
	HAL_FLASH_Lock();
}

void params_get(params_t *params)
{
	flash_get(PARAMS_ADDRESS, sizeof(params_t), (uint8_t *) params);
}

void params_set(params_t *params)
{
	flash_erase(PARAMS_ADDRESS);
	flash_set(PARAMS_ADDRESS, sizeof(params_t), (uint8_t *) params);
}

static void uid_string(char *uid)
{
	char temp[16];

	utoa(HAL_GetUIDw0(), temp, 16);
	strcpy(uid, temp);
	utoa(HAL_GetUIDw1(), temp, 16);
	strcat(uid, temp);
	utoa(HAL_GetUIDw2(), temp, 16);
	strcat(uid, temp);
}

// Do not edit magic and id
static void set_default(params_t *params)
{
	memset(params, 0, sizeof(params_t));
	strcpy(params->apn, "internet");
	strcpy(params->url_ota, "ota.proshutinsky.ru");
	strcpy(params->url_app, "app.proshutinsky.ru");
	uid_string(params->mcu_uid);
	memset(params->secret, 0xAB, sizeof(params->secret));
	params->period_app = 60 * 5;
	params->period_sen = 60;
	params->mtime_count = 10;
	params->offset_angle = 0;
}

void params_init(void)
{
	params_t params;
	uint32_t id;

	params_get(&params);

	// First initialization
	if (params.magic != PARAMS_MAGIC_VALID)
	{
		// Empty - set id to 0
		if (params.magic == PARAMS_MAGIC_EMPTY)
			id = 0;
		// Not empty - use saved id
		else
			id = params.id;

		set_default(&params);
		params.magic = PARAMS_MAGIC_VALID;
		params.id = id;

		params_set(&params);
	}
}
