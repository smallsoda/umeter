/*
 * Application interface
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "appiface.h"

#include <string.h>
#include "cmsis_os.h"
#include "main.h"

#include "strjson.h"
#include "siface.h"
#include "fws.h"


int appiface(void *iface)
{
	struct siface *siface = iface;
	struct appiface *appif = siface->context;
	char *request;
	int ret;

	// TODO: Parse data

	HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_4); // TODO: Remove

	request = pvPortMalloc(256); // NOTE: ?
	if (!request)
		return -1;

	/* strjson_init(request); */
	strcpy(request, "@,,{}");
	strjson_str(&request[3], "uid", appif->params->mcu_uid);
	strjson_uint(request, "ts", *appif->timestamp);
	strjson_str(request, "name", PARAMS_DEVICE_NAME);
	strjson_str(request, "bl_git", (char *) appif->bl->hash);
	strjson_str(request, "app_git", GIT_COMMIT_HASH);
	strcat(request, "\r\n");

	ret = siface_add(siface, request);
	if (ret < 0)
	{
		vPortFree(request);
		return -1;
	}

	return 0;
}
