/*
 * Greybus Simulator
 *
 * Copyright 2014 Google Inc.
 * Copyright 2014 Linaro Ltd.
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#include <errno.h>
#include <stdio.h>
#include <usbg/usbg.h>

#include "gbsim.h"
#include "gbsim_usb.h"

#define VENDOR		0x18d1
#define PRODUCT		0x1eaf

int gadget_create(usbg_state **s, usbg_gadget **g)
{
	usbg_config *c;
	usbg_function *f;
	int ret = -EINVAL;
	int usbg_ret;
	char gbsim_name[10];
	char gadget_name[5];

	snprintf(gbsim_name, 9, "gbsim%d", gbsim_id);
	snprintf(gadget_name, 4, "g%d", gbsim_id);

	struct usbg_gadget_attrs g_attrs = {
			0x0200, /* bcdUSB */
			0x00, /* Defined at interface level */
			0x00, /* subclass */
			0x00, /* device protocol */
			0x0040, /* Max allowed packet size */
			VENDOR,
			PRODUCT,
			0x0001, /* Verson of device */
	};

	struct usbg_gadget_strs g_strs = {
			"0123456789", /* Serial number */
			"Toshiba", /* Manufacturer */
			"AP Bridge" /* Product string */
	};

	struct usbg_config_strs c_strs = {
			"AP Bridge"
	};

	usbg_ret = usbg_init("/sys/kernel/config", s);
	if (usbg_ret != USBG_SUCCESS) {
		gbsim_error("Error on USB gadget init\n");
		gbsim_error("Error: %s : %s\n", usbg_error_name(usbg_ret),
			    usbg_strerror(usbg_ret));
		goto out1;
	}

	usbg_ret = usbg_create_gadget(*s, gadget_name, &g_attrs, &g_strs, g);
	if (usbg_ret != USBG_SUCCESS) {
		gbsim_error("Error on create gadget\n");
		gbsim_error("Error: %s : %s\n", usbg_error_name(usbg_ret),
			    usbg_strerror(usbg_ret));
		goto out2;
	}

	usbg_ret = usbg_create_function(*g, USBG_F_FFS, gbsim_name, NULL, &f);
	if (usbg_ret != USBG_SUCCESS) {
		gbsim_error("Error creating gbsim function\n");
		gbsim_error("Error: %s : %s\n", usbg_error_name(usbg_ret),
			    usbg_strerror(usbg_ret));
		goto out2;
	}

	usbg_ret = usbg_create_config(*g, 1, NULL, NULL, &c_strs, &c);
	if (usbg_ret != USBG_SUCCESS) {
		gbsim_error("Error creating config\n");
		gbsim_error("Error: %s : %s\n", usbg_error_name(usbg_ret),
			    usbg_strerror(usbg_ret));
		goto out2;
	}

	usbg_ret = usbg_add_config_function(c, gbsim_name, f);
	if (usbg_ret != USBG_SUCCESS) {
		gbsim_error("Error adding gbsim configuration\n");
		gbsim_error("Error: %s : %s\n", usbg_error_name(usbg_ret),
			    usbg_strerror(usbg_ret));
		goto out2;
	}

	gbsim_info("USB gadget created\n");

	return 0;

out2:
	gadget_cleanup(*s, *g);

out1:
	return ret;
}

int gadget_enable(usbg_state *s,usbg_gadget *g)
{
	usbg_udc *udc;
	char dummy_udc_name[20];

	snprintf(dummy_udc_name, 19, "dummy_udc.%d", gbsim_id);
	udc=usbg_get_udc(s, dummy_udc_name);
	return usbg_enable_gadget(g, udc);
}

void gadget_cleanup(usbg_state *s, usbg_gadget *g)
{
	gbsim_debug("gadget_cleanup\n");

	if (g) {
		usbg_disable_gadget(g);
		usbg_rm_gadget(g, USBG_RM_RECURSE);
	}

	usbg_cleanup(s);
}
