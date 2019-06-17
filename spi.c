/*
 * Greybus Simulator
 *
 * Copyright 2015 Google Inc.
 * Copyright 2015 Linaro Ltd.
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/spi/spidev.h>

#include "gbsim.h"

#define SPI_BPW_MASK(bits) BIT((bits) - 1)
#define SPIDEV_TYPE	0x00
#define SPINOR_TYPE	0x01

#define SPI_NUM_CS	1

#define SPI_NOR_IDLE	0
#define SPI_NOR_CMD	1
#define SPI_NOR_PP	2
#define SPI_NOR_READ	3

static int ifd1;
int spiflag=1;

struct gb_spi_dev_config {
	uint16_t	mode;
	uint32_t	bits_per_word;
	uint32_t	max_speed_hz;
	uint8_t		device_type;
	uint8_t		name[32];
};

struct gb_spi_dev {
	uint8_t	cs;
	uint8_t	*buf;
	size_t	buf_size;
	uint8_t	*buf_resp;
	uint8_t	cmd_resp[6];
	size_t	resp_size;
	int	state;
	int	cmd;
	int	(*xfer_req_recv)(struct gb_spi_dev *dev,
				 struct gb_spi_transfer *xfer,
				 uint8_t *xfer_data);
	struct gb_spi_dev_config *conf;
};

struct gb_spi_master {
	uint16_t		mode;
	uint8_t			flags;
	uint32_t		bpwm;
	uint32_t		min_speed_hz;
	uint32_t		max_speed_hz;
	uint8_t			num_chipselect;
	struct gb_spi_dev	*devices;
};

static struct gb_spi_master *master;

static struct gb_spi_dev_config spidev_config = {
	.mode		= GB_SPI_MODE_MODE_0,
	.bits_per_word	= 8,
	.max_speed_hz	=  1000000,
	.name		= "mmc_spi",
	.device_type	= GB_SPI_SPI_MODALIAS,
};

static int spidev_xfer_req_recv(struct gb_spi_dev *dev,
				struct gb_spi_transfer *xfer,
				uint8_t *xfer_data)
{
	int ret;

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)xfer_data,
		.rx_buf = (unsigned long)dev->buf_resp,
		.len = xfer->len,
		.delay_usecs = xfer->delay_usecs,
		.speed_hz = xfer->speed_hz,
		.bits_per_word = xfer->bits_per_word,
	};
	ret = ioctl(ifd1, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		gbsim_error("can't send spi message");
	if (xfer->xfer_flags & GB_SPI_XFER_READ)
		dev->resp_size = xfer->len;
	return 0;
}

static int spi_set_device(uint8_t cs, int spi_type)
{
	struct gb_spi_dev *spi_dev = &master->devices[cs];
	spi_dev->xfer_req_recv = spidev_xfer_req_recv;
	spi_dev->buf_size = 0;
	spi_dev->conf = &spidev_config;
	spi_dev->state = SPI_NOR_IDLE;
	spi_dev->cs = cs;

	if (!spi_dev->buf_size)
		return 0;

	spi_dev->buf = calloc(1, spi_dev->buf_size);
	if (!spi_dev->buf) {
		free(spi_dev);
		return 0;
	}

	return 0;
}

static int spi_master_setup(void)
{
	master = calloc(1, sizeof(struct gb_spi_master));
 
	master->mode = GB_SPI_MODE_MODE_0;
	master->flags = 0;
	master->bpwm = SPI_BPW_MASK(8) | SPI_BPW_MASK(16) | SPI_BPW_MASK(32);
	master->min_speed_hz = 400000;
	master->max_speed_hz = 48000000;
	master->num_chipselect = SPI_NUM_CS;

	master->devices = calloc(master->num_chipselect,
				 sizeof(struct gb_spi_dev));
	spi_set_device(0, SPIDEV_TYPE);

	return 0;
}

int spi_handler(struct gbsim_connection *connection, void *rbuf,
		   size_t rsize, void *tbuf, size_t tsize)
{
	struct gb_operation_msg_hdr *oph;
	struct op_msg *op_req = rbuf;
	struct op_msg *op_rsp;
	size_t payload_size = 0;
	uint16_t message_size;
	uint16_t hd_cport_id = connection->hd_cport_id;
	struct gb_spi_transfer *xfer;
	struct gb_spi_dev *spi_dev;
	struct gb_spi_dev_config *conf;
	void *xfer_data;
	int xfer_cs, cs;
	int xfer_count;
	int xfer_rx = 0;
	int ret;
	int i;

	op_rsp = (struct op_msg *)tbuf;
	oph = (struct gb_operation_msg_hdr *)&op_req->header;

	switch (oph->type) {
	case GB_SPI_TYPE_MASTER_CONFIG:
		spi_master_setup();
		payload_size = sizeof(struct gb_spi_master_config_response);
		op_rsp->spi_mc_rsp.mode = htole16(master->mode);
		op_rsp->spi_mc_rsp.flags = htole16(master->flags);
		op_rsp->spi_mc_rsp.bits_per_word_mask = htole32(master->bpwm);
		op_rsp->spi_mc_rsp.num_chipselect = htole16(master->num_chipselect);
		op_rsp->spi_mc_rsp.min_speed_hz = htole32(master->min_speed_hz);
		op_rsp->spi_mc_rsp.max_speed_hz = htole32(master->max_speed_hz);
		break;
	case GB_SPI_TYPE_DEVICE_CONFIG:
		payload_size = sizeof(struct gb_spi_device_config_response);
		cs = op_req->spi_dc_req.chip_select;
		spi_dev = &master->devices[cs];
		conf = spi_dev->conf;
		op_rsp->spi_dc_rsp.mode = htole16(conf->mode);
		op_rsp->spi_dc_rsp.bits_per_word = conf->bits_per_word;
		op_rsp->spi_dc_rsp.max_speed_hz = htole32(conf->max_speed_hz);
		op_rsp->spi_dc_rsp.device_type = conf->device_type;
		char channelfilename[60];
		char devicename[20];
		FILE* channelfile;
		snprintf(channelfilename, 59, "/sys/bus/greybus/devices/%d-%d.%d.ctrl/product_string", connection->cport_id,connection->intf->interface_id,connection->intf->interface_id);
		channelfile = fopen(channelfilename,"r");		 
		fscanf (channelfile,"%s",devicename);	
	        memcpy(op_rsp->spi_dc_rsp.name, devicename, sizeof(devicename));
		break;
	case GB_SPI_TYPE_TRANSFER:
		xfer_cs = op_req->spi_xfer_req.chip_select;
		xfer_count = op_req->spi_xfer_req.count;
		xfer = &op_req->spi_xfer_req.transfers[0];
		xfer_data = xfer + xfer_count;
		spi_dev = &master->devices[xfer_cs];

		spi_dev->buf_resp = op_rsp->spi_xfer_rsp.data;

		for (i = 0; i < xfer_count; i++, xfer++) {
			spi_dev->xfer_req_recv(spi_dev, xfer, xfer_data);
			if (xfer->xfer_flags & GB_SPI_XFER_WRITE)
				xfer_data += xfer->len;
			if (xfer->xfer_flags & GB_SPI_XFER_READ)
				xfer_rx += xfer->len;
		}

		payload_size = sizeof(struct gb_spi_transfer_response) + xfer_rx;
		break;
	}

	message_size = sizeof(struct gb_operation_msg_hdr) + payload_size;
	ret = send_response(hd_cport_id, op_rsp, message_size,
			    oph->operation_id, oph->type,
			    PROTOCOL_STATUS_SUCCESS);
	return ret;
}

char *spi_get_operation(uint8_t type)
{
	switch (type) {
	case GB_REQUEST_TYPE_INVALID:
		return "GB_SPI_TYPE_INVALID";
	case GB_SPI_TYPE_MASTER_CONFIG:
		return "GB_SPI_TYPE_MASTER_CONFIG";
	case GB_SPI_TYPE_DEVICE_CONFIG:
		return "GB_SPI_TYPE_DEVICE_CONFIG";
	case GB_SPI_TYPE_TRANSFER:
		return "GB_SPI_TYPE_TRANSFER";
	default:
		return "(Unknown operation)";
	}
}

void spi_init(void)	
{	
	if(bbb_backend)
	{
		ifd1 = open("/dev/spidev1.0", O_RDWR);	//needs fixing
		if (ifd1 < 0)	
			gbsim_error("failed opening spi node read/write\n");
	}	
}
