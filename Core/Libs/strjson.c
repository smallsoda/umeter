/*
 * String JSON
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "strjson.h"

#include <string.h>


/******************************************************************************/
void strjson_init(char *json)
{
	strcpy(json, "{}");
}

void strjson_str(char *json, const char *name, const char *value)
{
	size_t len = strlen(json);

	if (!len)
		return;

	json = &json[len - 1];

	if (len > 2)
	{
		strcpy(json, ",");
		json++;
	}

	strcpy(json, "\"");
	strcat(json, name);
	strcat(json, "\":\"");
	strcat(json, value);
	strcat(json, "\"}");
}

void strjson_int(char *json, const char *name, int value)
{
	size_t len = strlen(json);

	if (!len)
		return;

	json = &json[len - 1];

	if (len > 2)
	{
		strcpy(json, ",");
		json++;
	}

	strcpy(json, "\"");
	strcat(json, name);
	strcat(json, "\":");
	itoa(value, &json[strlen(json)], 10);
	strcat(json, "}");
}
