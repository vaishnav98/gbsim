/*
 * Greybus Simulator: SVC CPort protocol
 *
 * Copyright 2015 Google Inc.
 * Copyright 2015 Linaro Ltd.
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#include <fcntl.h>
#include <pthread.h>
#include <linux/fs.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "gbsim.h"

struct gbsim_svc *svc;

int svc_get_next_intf_id(struct gbsim_svc *s)
{
	struct gbsim_interface *intf;
	int intf_id = 1;

start_again:
	TAILQ_FOREACH(intf, &s->intfs, intf_node) {
		if (intf_id == intf->interface_id) {
			intf_id++;
			goto start_again;
		}
	}

	return intf_id;
}

static int svc_handler_request(uint16_t cport_id, uint16_t hd_cport_id,
			       void *rbuf, size_t rsize, void *tbuf,
			       size_t tsize)
{
	struct op_msg *op_req = rbuf;
	struct op_msg *op_rsp = tbuf;
	struct gb_operation_msg_hdr *oph = &op_req->header;
	struct gb_svc_intf_device_id_request *svc_dev_id;
	struct gb_svc_conn_create_request *svc_conn_create;
	struct gb_svc_conn_destroy_request *svc_conn_destroy;
	struct gb_svc_dme_peer_get_request *dme_get_request;
	struct gb_svc_dme_peer_get_response *dme_get_response;
	struct gb_svc_dme_peer_set_request *dme_set_request;
	struct gb_svc_dme_peer_set_response *dme_set_response;
	struct gb_svc_route_create_request *svc_route_create;
	struct gb_svc_route_destroy_request *svc_route_destroy;
	struct gb_svc_intf_set_pwrm_request *svc_intf_set_pwrm;
	struct gb_svc_pwrmon_rail_count_get_response *svc_pwrmon_rail_count_get_response;
	struct gb_svc_intf_vsys_response *svc_intf_vsys_response;
	struct gb_svc_intf_refclk_response *svc_intf_refclk_response;
	struct gb_svc_intf_unipro_response *svc_intf_unipro_response;
	struct gb_svc_intf_activate_response *svc_intf_activate_response;
	struct gb_svc_intf_resume_response *svc_intf_resume_response;
	struct gb_svc_intf_set_pwrm_response *svc_intf_set_pwrm_response;
	struct gbsim_interface *intf;
	struct gbsim_connection *connection;
	uint16_t ap_intf_id, ap_cport_id, mod_intf_id, mod_cport_id;
	uint16_t message_size = sizeof(*oph);
	uint8_t intf_id = 0;
	size_t payload_size = 0;

	switch (oph->type) {
	case GB_SVC_TYPE_PROTOCOL_VERSION:
		gbsim_error("%s: Protocol Version request not supported\n",
			    __func__);
		break;
	case GB_SVC_TYPE_INTF_DEVICE_ID:
		svc_dev_id = &op_req->svc_intf_device_id_request;

		gbsim_debug("SVC assign device id (%hhu %hhu) response\n",
			    svc_dev_id->intf_id, svc_dev_id->device_id);
		break;
	case GB_SVC_TYPE_CONN_CREATE:
		svc_conn_create = &op_req->svc_conn_create_request;

		ap_intf_id = le16toh(svc_conn_create->intf1_id);
		ap_cport_id = le16toh(svc_conn_create->cport1_id);
		mod_intf_id = le16toh(svc_conn_create->intf2_id);
		mod_cport_id = le16toh(svc_conn_create->cport2_id);

		gbsim_debug("SVC connection create request (%hu %hu):(%hu %hu) response\n",
			    ap_intf_id, ap_cport_id, mod_intf_id, mod_cport_id);

		intf = interface_get_by_id(svc, mod_intf_id);
		if (!intf) {
			gbsim_error("SVC No interface: %hu\n", mod_intf_id);
			break;
		}

		connection = allocate_connection(intf, mod_cport_id,
						 ap_cport_id);
		if (!connection) {
			gbsim_error("Failed to allocate connection: (%hu %hu):(%hu %hu)\n",
				    ap_intf_id, ap_cport_id, mod_intf_id,
				    mod_cport_id);
			return -ENOMEM;
		}

		connection_set_protocol(connection, mod_cport_id);

		break;
	case GB_SVC_TYPE_CONN_DESTROY:
		svc_conn_destroy = &op_req->svc_conn_destroy_request;

		ap_intf_id = le16toh(svc_conn_destroy->intf1_id);
		ap_cport_id = le16toh(svc_conn_destroy->cport1_id);
		mod_intf_id = le16toh(svc_conn_destroy->intf2_id);
		mod_cport_id = le16toh(svc_conn_destroy->cport2_id);

		gbsim_debug("SVC connection destroy request (%hu %hu):(%hu %hu) response\n",
			    ap_intf_id, ap_cport_id, mod_intf_id, mod_cport_id);

		connection = connection_find(ap_cport_id);

		free_connection(connection);
		break;
	case GB_SVC_TYPE_DME_PEER_GET:
		payload_size = sizeof(*dme_get_response);
		dme_get_request = &op_req->svc_dme_peer_get_request;
		dme_get_response = &op_rsp->svc_dme_peer_get_response;
		dme_get_response->result_code = 0;
		dme_get_response->attr_value = 0x0126;

		gbsim_debug("SVC dme peer get (%hhu %hu %hu) request\n",
			    dme_get_request->intf_id, dme_get_request->attr,
			    dme_get_request->selector);
		gbsim_debug("SVC dme peer get (%hu %u) response\n",
			    dme_get_response->result_code,
			    dme_get_response->attr_value);
		break;
	case GB_SVC_TYPE_DME_PEER_SET:
		payload_size = sizeof(*dme_set_response);
		dme_set_request = &op_req->svc_dme_peer_set_request;
		dme_set_response = &op_rsp->svc_dme_peer_set_response;
		dme_set_response->result_code = 0;

		gbsim_debug("SVC dme peer set (%hhu %hu %hu %u) request\n",
			    dme_set_request->intf_id, dme_set_request->attr,
			    dme_set_request->selector, dme_set_request->value);
		gbsim_debug("SVC dme peer set (%hu) response\n",
			    dme_set_response->result_code);
		break;
	case GB_SVC_TYPE_ROUTE_CREATE:
		svc_route_create = &op_req->svc_route_create_request;

		gbsim_debug("SVC route create request (%hhu %u):(%hhu %u) response\n",
			    svc_route_create->intf1_id, svc_route_create->dev1_id,
			    svc_route_create->intf2_id, svc_route_create->dev2_id);
		break;
	case GB_SVC_TYPE_ROUTE_DESTROY:
		svc_route_destroy = &op_req->svc_route_destroy_request;

		gbsim_debug("SVC route destroy request (%hhu:%hhu) response\n",
			    svc_route_destroy->intf1_id,
			    svc_route_destroy->intf2_id);
		break;
	case GB_SVC_TYPE_PING:
		gbsim_debug("SVC ping request response\n");
		break;
	case GB_SVC_TYPE_PWRMON_RAIL_COUNT_GET:
		payload_size = sizeof(*svc_pwrmon_rail_count_get_response);
		svc_pwrmon_rail_count_get_response = &op_rsp->svc_pwrmon_rail_count_get_response;
		svc_pwrmon_rail_count_get_response->rail_count = 0;
		break;
	case GB_SVC_TYPE_INTF_VSYS_ENABLE:
		payload_size = sizeof(*svc_intf_vsys_response);
		svc_intf_vsys_response = &op_rsp->svc_intf_vsys_response;
		svc_intf_vsys_response->result_code = 0;
		break;
	case GB_SVC_TYPE_INTF_VSYS_DISABLE:
		payload_size = sizeof(*svc_intf_vsys_response);
		svc_intf_vsys_response = &op_rsp->svc_intf_vsys_response;
		svc_intf_vsys_response->result_code = 0;

		intf_id = op_req->svc_intf_vsys_request.intf_id;

		intf = interface_get_by_id(svc, intf_id);
		if (!intf)
			return -ENODEV;
		interface_free(svc, intf);

		break;
	case GB_SVC_TYPE_INTF_REFCLK_ENABLE:
		payload_size = sizeof(*svc_intf_refclk_response);
		svc_intf_refclk_response = &op_rsp->svc_intf_refclk_response;
		svc_intf_refclk_response->result_code = 0;
		break;
	case GB_SVC_TYPE_INTF_REFCLK_DISABLE:
		payload_size = sizeof(*svc_intf_refclk_response);
		svc_intf_refclk_response = &op_rsp->svc_intf_refclk_response;
		svc_intf_refclk_response->result_code = 0;
		break;
	case GB_SVC_TYPE_INTF_UNIPRO_ENABLE:
		payload_size = sizeof(*svc_intf_unipro_response);
		svc_intf_unipro_response = &op_rsp->svc_intf_unipro_response;
		svc_intf_unipro_response->result_code = 0;
		break;
	case GB_SVC_TYPE_INTF_UNIPRO_DISABLE:
		payload_size = sizeof(*svc_intf_unipro_response);
		svc_intf_unipro_response = &op_rsp->svc_intf_unipro_response;
		svc_intf_unipro_response->result_code = 0;
		break;
	case GB_SVC_TYPE_INTF_ACTIVATE:
		payload_size = sizeof(*svc_intf_activate_response);
		svc_intf_activate_response = &op_rsp->svc_intf_activate_response;
		svc_intf_activate_response->intf_type = GB_SVC_INTF_TYPE_GREYBUS;
		break;
	case GB_SVC_TYPE_INTF_RESUME:
		payload_size = sizeof(*svc_intf_resume_response);
		svc_intf_resume_response = &op_rsp->svc_intf_resume_response;
		svc_intf_resume_response->status = GB_SVC_OP_SUCCESS;
		break;
	case GB_SVC_TYPE_INTF_MAILBOX_EVENT:
		break;
	case GB_SVC_TYPE_INTF_SET_PWRM:
		payload_size = sizeof(*svc_intf_set_pwrm_response);
		svc_intf_set_pwrm = &op_req->svc_intf_set_pwrm_request;
		svc_intf_set_pwrm_response = &op_rsp->svc_intf_set_pwrm_response;
		if (svc_intf_set_pwrm->tx_mode != GB_SVC_UNIPRO_HIBERNATE_MODE)
        svc_intf_set_pwrm_response->result_code = GB_SVC_SETPWRM_PWR_LOCAL;
        else
        svc_intf_set_pwrm_response->result_code = GB_SVC_SETPWRM_PWR_OK;

		break;
	case GB_SVC_TYPE_MODULE_INSERTED:
	case GB_SVC_TYPE_MODULE_REMOVED:
	case GB_SVC_TYPE_INTF_RESET:
	default:
		gbsim_error("%s: Request not supported (%d)\n", __func__,
			    oph->type);
		return -EINVAL;
	}

	message_size += payload_size;
	return send_response(hd_cport_id, op_rsp, message_size,
				oph->operation_id, oph->type,
				PROTOCOL_STATUS_SUCCESS);
}

static int svc_handler_response(uint16_t cport_id, uint16_t hd_cport_id,
				void *rbuf, size_t rsize)
{
	struct op_msg *op_rsp = rbuf;
	struct gb_operation_msg_hdr *oph = &op_rsp->header;
	int ret;

	/* Must be AP's svc protocol's cport */
	if (cport_id != GB_SVC_CPORT_ID || cport_id != hd_cport_id) {
		gbsim_error("%s: Error: cport-id-mismatch (%d %d %d)", __func__,
			    cport_id, hd_cport_id, GB_SVC_CPORT_ID);
		return -EINVAL;
	}

	switch (oph->type & ~OP_RESPONSE) {
	case GB_SVC_TYPE_PROTOCOL_VERSION:
		gbsim_debug("%s: Version major-%d minor-%d\n", __func__,
			    op_rsp->svc_version_response.major,
			    op_rsp->svc_version_response.minor);

		/* Version request successful, send hello msg */
		ret = svc_request_send(GB_SVC_TYPE_SVC_HELLO, AP_INTF_ID);
		if (ret) {
			gbsim_error("%s: Failed to send svc hello request (%d)\n",
				    __func__, ret);
			return ret;
		}
		break;
	case GB_SVC_TYPE_SVC_HELLO:
		/*
		 * AP's SVC cport is ready now, start scanning for module
		 * hotplug.
		 */
		ret = inotify_start(svc, hotplug_basedir);
		if (ret < 0)
			gbsim_error("Failed to start inotify thread\n");
		break;
	case GB_SVC_TYPE_MODULE_REMOVED:
		break;
	case GB_SVC_TYPE_MODULE_INSERTED:
	case GB_SVC_TYPE_INTF_RESET:
		break;
	default:
		gbsim_error("%s: Response not supported (%d)\n", __func__,
			    oph->type & ~OP_RESPONSE);
		return -EINVAL;
	}

	return 0;
}

int svc_handler(struct gbsim_connection *connection, void *rbuf,
		    size_t rsize, void *tbuf, size_t tsize)
{
	struct op_msg *op = rbuf;
	struct gb_operation_msg_hdr *oph = &op->header;
	uint16_t cport_id = connection->cport_id;
	uint16_t hd_cport_id = connection->hd_cport_id;

	if (oph->type & OP_RESPONSE)
		return svc_handler_response(cport_id, hd_cport_id, rbuf, rsize);
	else
		return svc_handler_request(cport_id, hd_cport_id, rbuf, rsize,
					   tbuf, tsize);
}

char *svc_get_operation(uint8_t type)
{
	switch (type) {
	case GB_REQUEST_TYPE_INVALID:
		return "GB_SVC_TYPE_INVALID";
	case GB_SVC_TYPE_PROTOCOL_VERSION:
		return "GB_SVC_TYPE_PROTOCOL_VERSION";
	case GB_SVC_TYPE_SVC_HELLO:
		return "GB_SVC_TYPE_SVC_HELLO";
	case GB_SVC_TYPE_INTF_DEVICE_ID:
		return "GB_SVC_TYPE_INTF_DEVICE_ID";
	case GB_SVC_TYPE_MODULE_INSERTED:
		return "GB_SVC_TYPE_MODULE_INSERTED";
	case GB_SVC_TYPE_MODULE_REMOVED:
		return "GB_SVC_TYPE_MODULE_REMOVED";
	case GB_SVC_TYPE_INTF_RESET:
		return "GB_SVC_TYPE_INTF_RESET";
	case GB_SVC_TYPE_CONN_CREATE:
		return "GB_SVC_TYPE_CONN_CREATE";
	case GB_SVC_TYPE_CONN_DESTROY:
		return "GB_SVC_TYPE_CONN_DESTROY";
	case GB_SVC_TYPE_DME_PEER_GET:
		return "GB_SVC_TYPE_DME_PEER_GET";
	case GB_SVC_TYPE_DME_PEER_SET:
		return "GB_SVC_TYPE_DME_PEER_SET";
	case GB_SVC_TYPE_ROUTE_CREATE:
		return "GB_SVC_TYPE_ROUTE_CREATE";
	case GB_SVC_TYPE_ROUTE_DESTROY:
		return "GB_SVC_TYPE_ROUTE_DESTROY";
	case GB_SVC_TYPE_PING:
		return "GB_SVC_TYPE_PING";
	case GB_SVC_TYPE_TIMESYNC_ENABLE:
		return "GB_SVC_TYPE_TIMESYNC_ENABLE";
	case GB_SVC_TYPE_TIMESYNC_DISABLE:
		return "GB_SVC_TYPE_TIMESYNC_DISABLE";
	case GB_SVC_TYPE_TIMESYNC_AUTHORITATIVE:
		return "GB_SVC_TYPE_TIMESYNC_AUTHORITATIVE";
	case GB_SVC_TYPE_INTF_SET_PWRM:
		return "GB_SVC_TYPE_INTF_SET_PWRM";
	case GB_SVC_TYPE_INTF_EJECT:
		return "GB_SVC_TYPE_INTF_EJECT";
	case GB_SVC_TYPE_PWRMON_RAIL_COUNT_GET:
		return "GB_SVC_TYPE_PWRMON_RAIL_COUNT_GET";
	case GB_SVC_TYPE_PWRMON_RAIL_NAMES_GET:
		return "GB_SVC_TYPE_PWRMON_RAIL_NAMES_GET";
	case GB_SVC_TYPE_PWRMON_SAMPLE_GET:
		return "GB_SVC_TYPE_PWRMON_SAMPLE_GET";
	case GB_SVC_TYPE_PWRMON_INTF_SAMPLE_GET:
		return "GB_SVC_TYPE_PWRMON_INTF_SAMPLE_GET";
	case GB_SVC_TYPE_TIMESYNC_WAKE_PINS_ACQUIRE:
		return "GB_SVC_TYPE_TIMESYNC_WAKE_PINS_ACQUIRE";
	case GB_SVC_TYPE_TIMESYNC_WAKE_PINS_RELEASE:
		return "GB_SVC_TYPE_TIMESYNC_WAKE_PINS_RELEASE";
	case GB_SVC_TYPE_TIMESYNC_PING:
		return "GB_SVC_TYPE_TIMESYNC_PING";
	case GB_SVC_TYPE_INTF_VSYS_ENABLE:
		return "GB_SVC_TYPE_INTF_VSYS_ENABLE";
	case GB_SVC_TYPE_INTF_VSYS_DISABLE:
		return "GB_SVC_TYPE_INTF_VSYS_DISABLE";
	case GB_SVC_TYPE_INTF_REFCLK_ENABLE:
		return "GB_SVC_TYPE_INTF_REFCLK_ENABLE";
	case GB_SVC_TYPE_INTF_REFCLK_DISABLE:
		return "GB_SVC_TYPE_INTF_REFCLK_DISABLE";
	case GB_SVC_TYPE_INTF_UNIPRO_ENABLE:
		return "GB_SVC_TYPE_INTF_UNIPRO_ENABLE";
	case GB_SVC_TYPE_INTF_UNIPRO_DISABLE:
		return "GB_SVC_TYPE_INTF_UNIPRO_DISABLE";
	case GB_SVC_TYPE_INTF_ACTIVATE:
		return "GB_SVC_TYPE_INTF_ACTIVATE";
	case GB_SVC_TYPE_INTF_RESUME:
		return "GB_SVC_TYPE_INTF_RESUME";
	case GB_SVC_TYPE_INTF_MAILBOX_EVENT:
		return "GB_SVC_TYPE_INTF_MAILBOX_EVENT";
	default:
		return "(Unknown operation)";
	}
}

int svc_request_send(uint8_t type, uint8_t intf_id)
{
	struct op_msg msg = { };
	struct gb_operation_msg_hdr *oph = &msg.header;
	struct gb_svc_version_request *version_request;
	struct gb_svc_hello_request *hello_request;
	struct gb_svc_module_inserted_request *inserted;
	struct gb_svc_module_removed_request *removed;
	struct gb_svc_intf_reset_request *reset;
	uint16_t message_size = sizeof(*oph);
	size_t payload_size;

	switch (type) {
	case GB_SVC_TYPE_PROTOCOL_VERSION:
		payload_size = sizeof(*version_request);
		version_request = &msg.svc_version_request;
		version_request->major = GB_SVC_VERSION_MAJOR;
		version_request->minor = GB_SVC_VERSION_MINOR;
		break;
	case GB_SVC_TYPE_SVC_HELLO:
		payload_size = sizeof(*hello_request);
		hello_request = &msg.hello_request;

		hello_request->endo_id = htole16(ENDO_ID);
		hello_request->interface_id = AP_INTF_ID;
		break;
	case GB_SVC_TYPE_MODULE_INSERTED:
		payload_size = sizeof(*inserted);
		inserted = &msg.svc_module_inserted_request;

		inserted->primary_intf_id = intf_id;
		inserted->intf_count = 1;
		inserted->flags = 0;

		break;
	case GB_SVC_TYPE_MODULE_REMOVED:
		payload_size = sizeof(*removed);
		removed = &msg.svc_module_removed_request;
		removed->primary_intf_id = intf_id;
		break;
	case GB_SVC_TYPE_INTF_RESET:
		payload_size = sizeof(*reset);
		reset = &msg.svc_intf_reset_request;
		reset->intf_id = intf_id;

		break;
	default:
		gbsim_error("svc operation type %02x not supported\n", type);
		return -EINVAL;
	}

	message_size += payload_size;
	return send_request(GB_SVC_CPORT_ID, &msg, message_size, 1, type);
}

int svc_init(void)
{
	struct gbsim_connection *connection;

	svc = calloc(1, sizeof(*svc));
	if (!svc)
		return -ENOMEM;

	TAILQ_INIT(&svc->intfs);

	/* init svc->ap interface */
	svc->intf = interface_alloc(svc, 0);
	if (!svc->intf)
		return -ENOMEM;

	connection = allocate_connection(svc->intf, GB_SVC_CPORT_ID,
					 GB_SVC_CPORT_ID);
	if (!connection)
		return -ENOMEM;

	connection_set_protocol(connection, GB_SVC_CPORT_ID);

	return 0;
}

void svc_exit(void)
{
	if (svc && svc->intf)
		interface_free(svc, svc->intf);
	free(svc);
}
