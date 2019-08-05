
/*
 * Greybus Simulator
 *
 * Copyright 2014-2016 Google Inc.
 * Copyright 2014-2016 Linaro Ltd.
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#include <fcntl.h>
#include <pthread.h>
#include <libsoc_gpio.h>
#include <linux/fs.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "gbsim.h"

struct gb_gpio {
	uint8_t activated;
	uint8_t direction;
	uint8_t value;
	uint8_t irq_type;
	uint8_t irq_unmasked;
};

static struct gb_gpio gb_gpios[12];
static gpio *gpios[12];

uint8_t gpio_count=5;

uint8_t current_which;
uint16_t current_hd_cport_id;

unsigned int mikrobus_gpios[18] = {89,23,50,45,26,110,60,48,50,49,116,51,26,65,22,46,27,23};

int int_event_callback(void *args)
{
	size_t payload_size;	  
	uint16_t message_size;
	struct op_msg *op_req=malloc(sizeof(struct op_msg));

	payload_size = sizeof(struct gb_gpio_irq_event_request);
	op_req->gpio_irq_event_req.which = current_which;
	message_size = sizeof(struct gb_operation_msg_hdr) + payload_size;
	send_request(current_hd_cport_id, op_req, message_size, 0, GB_GPIO_TYPE_IRQ_EVENT);
	return EXIT_SUCCESS;
}

int gpio_handler(struct gbsim_connection *connection, void *rbuf,
		 size_t rsize, void *tbuf, size_t tsize)
{
	struct gb_operation_msg_hdr *oph;
	struct op_msg *op_req = rbuf;
	struct op_msg *op_rsp;
	size_t payload_size;
	ssize_t nbytes;
	uint16_t message_size;
	uint16_t hd_cport_id = connection->hd_cport_id;
	uint8_t which = 0;

	op_rsp = (struct op_msg *)tbuf;
	oph = (struct gb_operation_msg_hdr *)&op_req->header;

	switch (oph->type) {
	case GB_GPIO_TYPE_LINE_COUNT:
		payload_size = sizeof(struct gb_gpio_line_count_response);
		op_rsp->gpio_lc_rsp.count = gpio_count;
		break;
	case GB_GPIO_TYPE_ACTIVATE:
		payload_size = 0;
		which = op_req->gpio_act_req.which;
		gbsim_debug("GPIO %d activate request\n", which);
		gb_gpios[which].activated = 1;
		break;
	case GB_GPIO_TYPE_DEACTIVATE:
		payload_size = 0;
		which = op_req->gpio_deact_req.which;
		gbsim_debug("GPIO %d deactivate request\n", which);
		gb_gpios[which].activated = 0;
		break;
	case GB_GPIO_TYPE_GET_DIRECTION:
		payload_size = sizeof(struct gb_gpio_get_direction_response);
		which = op_req->gpio_get_dir_req.which;
		if (bbb_backend)
			op_rsp->gpio_get_dir_rsp.direction = libsoc_gpio_get_direction(gpios[which]);
		else
			op_rsp->gpio_get_dir_rsp.direction = gb_gpios[which].direction;
		gbsim_debug("GPIO %d get direction (%d) response\n",
			    which, op_rsp->gpio_get_dir_rsp.direction);
		break;
	case GB_GPIO_TYPE_DIRECTION_IN:
		payload_size = 0;
		which = op_req->gpio_dir_input_req.which;
		gbsim_debug("GPIO %d direction input request\n", which);
		if (bbb_backend)
			libsoc_gpio_set_direction(gpios[which], INPUT);
		else
			gb_gpios[which].direction = 1;
		break;
	case GB_GPIO_TYPE_DIRECTION_OUT:
		payload_size = 0;
		which = op_req->gpio_dir_output_req.which;
		gbsim_debug("GPIO %d direction output request\n", which);
		if (bbb_backend)
			libsoc_gpio_set_direction(gpios[which], OUTPUT);
		else
			gb_gpios[which].direction = 0;
		break;
	case GB_GPIO_TYPE_GET_VALUE:
		payload_size = sizeof(struct gb_gpio_get_value_response);
		which = op_req->gpio_get_val_req.which;
		if (bbb_backend)
			op_rsp->gpio_get_val_rsp.value = libsoc_gpio_get_level(gpios[which]);
		else
			op_rsp->gpio_get_val_rsp.value = gb_gpios[which].value;
		gbsim_debug("GPIO %d get value (%d) response\n  ",
			    which, op_rsp->gpio_get_val_rsp.value);
		break;
	case GB_GPIO_TYPE_SET_VALUE:
		payload_size = 0;
		which = op_req->gpio_set_val_req.which;
		gbsim_debug("GPIO %d set value (%d) request\n  ",
			    which, op_req->gpio_set_val_req.value);
		if (bbb_backend)
			libsoc_gpio_set_level(gpios[which], op_req->gpio_set_val_req.value);
		break;
	case GB_GPIO_TYPE_SET_DEBOUNCE:
		payload_size = 0;
		gbsim_debug("GPIO %d set debounce (%d us) request\n  ",
			    op_req->gpio_set_db_req.which, op_req->gpio_set_db_req.usec);
		break;
	case GB_GPIO_TYPE_IRQ_TYPE:
		payload_size = 0;
		which = op_req->gpio_irq_type_req.which;
		gbsim_debug("GPIO %d set IRQ type %d request\n  ",
			    which, op_req->gpio_irq_type_req.type);
		gb_gpios[which].irq_type = op_req->gpio_irq_type_req.type;
		if(bbb_backend) {
			libsoc_gpio_set_direction(gpios[which], INPUT);
			if(gb_gpios[which].irq_type == GB_GPIO_IRQ_TYPE_EDGE_RISING)
				libsoc_gpio_set_edge(gpios[which], RISING);
			else if(gb_gpios[which].irq_type == GB_GPIO_IRQ_TYPE_EDGE_FALLING)
				libsoc_gpio_set_edge(gpios[which], FALLING);
			else if(gb_gpios[which].irq_type == GB_GPIO_IRQ_TYPE_EDGE_BOTH)
				libsoc_gpio_set_edge(gpios[which], BOTH);
			current_which=which;
			current_hd_cport_id=hd_cport_id;
			libsoc_gpio_callback_interrupt(gpios[which], &int_event_callback, NULL);
		}
		break;
	case GB_GPIO_TYPE_IRQ_MASK:
		payload_size = 0;
		which = op_req->gpio_irq_mask_req.which;
		gb_gpios[which].irq_unmasked = 0;
		break;
	case GB_GPIO_TYPE_IRQ_UNMASK:
		payload_size = 0;
		which = op_req->gpio_irq_unmask_req.which;
		gb_gpios[which].irq_unmasked = 1;
		break;
	case GB_REQUEST_TYPE_CPORT_SHUTDOWN:
		payload_size = 0;
		break;
	default:
		return -EINVAL;
	}

	message_size = sizeof(struct gb_operation_msg_hdr) + payload_size;
	nbytes = send_response(hd_cport_id, op_rsp, message_size,
				oph->operation_id, oph->type,
				PROTOCOL_STATUS_SUCCESS);
	if (nbytes)
		return nbytes;
	return 0;
}

char *gpio_get_operation(uint8_t type)
{
	switch (type) {
	case GB_REQUEST_TYPE_INVALID:
		return "GB_GPIO_TYPE_INVALID";
	case GB_REQUEST_TYPE_CPORT_SHUTDOWN:
		return "GB_REQUEST_TYPE_CPORT_SHUTDOWN";
	case GB_GPIO_TYPE_LINE_COUNT:
		return "GB_GPIO_TYPE_LINE_COUNT";
	case GB_GPIO_TYPE_ACTIVATE:
		return "GB_GPIO_TYPE_ACTIVATE";
	case GB_GPIO_TYPE_DEACTIVATE:
		return "GB_GPIO_TYPE_DEACTIVATE";
	case GB_GPIO_TYPE_GET_DIRECTION:
		return "GB_GPIO_TYPE_GET_DIRECTION";
	case GB_GPIO_TYPE_DIRECTION_IN:
		return "GB_GPIO_TYPE_DIRECTION_IN";
	case GB_GPIO_TYPE_DIRECTION_OUT:
		return "GB_GPIO_TYPE_DIRECTION_OUT";
	case GB_GPIO_TYPE_GET_VALUE:
		return "GB_GPIO_TYPE_GET_VALUE";
	case GB_GPIO_TYPE_SET_VALUE:
		return "GB_GPIO_TYPE_SET_VALUE";
	case GB_GPIO_TYPE_SET_DEBOUNCE:
		return "GB_GPIO_TYPE_SET_DEBOUNCE";
	case GB_GPIO_TYPE_IRQ_TYPE:
		return "GB_GPIO_TYPE_IRQ_TYPE";
	case GB_GPIO_TYPE_IRQ_MASK:
		return "GB_GPIO_TYPE_IRQ_MASK";
	case GB_GPIO_TYPE_IRQ_UNMASK:
		return "GB_GPIO_TYPE_IRQ_UNMASK";
	case GB_GPIO_TYPE_IRQ_EVENT:
		return "GB_GPIO_TYPE_IRQ_EVENT";
	default:
		return "(Unknown operation)";
	}
}

void gpio_cleanup(void)
{
	int i;

	for(i=0;i<12;i++)
		if(gpios[i])
			libsoc_gpio_free(gpios[i]);
}

void gpio_init(void)
{
	int i;
	char platform[60];

	if (bbb_backend) {
		FILE* modelfile;
		modelfile = fopen("/proc/device-tree/model","r");		 
		fscanf (modelfile,"%s",platform);
		if(strcmp(platform,"TI AM335x PocketBeagle")!=0)
		{	
			gbsim_debug("Initalizing PocketBeagle GPIOs \n");
			for (i=0; i<6; i++)
			gpios[i] = libsoc_gpio_request(mikrobus_gpios[i], LS_GPIO_SHARED);
		}
		else
		{
			gbsim_debug("Initalizing Beaglebone Black GPIOs \n");
			gpio_count=11;
			for (i=6; i<18; i++)
			gpios[i-6] = libsoc_gpio_request(mikrobus_gpios[i], LS_GPIO_SHARED);
		}
	}
}