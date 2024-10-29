/*
 * Application interface
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#ifndef APPIFACE_H_
#define APPIFACE_H_

#include "params.h"

struct appiface
{
	volatile uint32_t *timestamp;
	volatile struct bl_params *bl;
	params_t *params;

	params_t uparams;
};


int appiface(void *iface);

#endif /* APPIFACE_H_ */
