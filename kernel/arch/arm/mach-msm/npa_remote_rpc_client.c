/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, and instead of the terms immediately above, this
 * software may be relicensed by the recipient at their option under the
 * terms of the GNU General Public License version 2 ("GPL") and only
 * version 2.  If the recipient chooses to relicense the software under
 * the GPL, then the recipient shall replace all of the text immediately
 * above and including this paragraph with the text immediately below
 * and between the words START OF ALTERNATE GPL TERMS and END OF
 * ALTERNATE GPL TERMS and such notices and license terms shall apply
 * INSTEAD OF the notices and licensing terms given above.
 *
 * START OF ALTERNATE GPL TERMS
 *
 * Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * This software was originally licensed under the Code Aurora Forum
 * Inc. Dual BSD/GPL License version 1.1 and relicensed as permitted
 * under the terms thereof by a recipient under the General Public
 * License Version 2.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * END OF ALTERNATE GPL TERMS
 *
 */

/*
 * Node Power Architecture (NPA) ONCRPC protocol support.
 */

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/npa_remote.h>
#include <mach/msm_rpcrouter.h>

#define NPA_REMOTEPROG			0x300000A4
#define NPA_REMOTEVERS			0x00010001
#define NPA_REMOTE_NULL_VERS		0x00010001

#define NPA_REMOTE_NULL_PROC				0
#define NPA_REMOTE_RPC_GLUE_CODE_INFO_REMOTE_PROC	1
#define NPA_REMOTE_INIT_PROC				2
#define NPA_REMOTE_CREATE_SYNC_CLIENT_PROC		3
#define NPA_REMOTE_DESTROY_CLIENT_PROC			5
#define NPA_REMOTE_ISSUE_REQUIRED_REQUEST_PROC		6
#define NPA_REMOTE_RESOURCE_AVAILABLE_PROC		22

#define NPA_REMOTE_API_VERSIONS_PROC		0xFFFFFFFF

#define NPA_REMOTE_CALLBACK_TYPE_PROC		1

struct npa_remote_cb_data {
	uint32_t cb_id;
	uint32_t context;
	uint32_t type;
	int32_t *buffer;
	uint32_t size;
};

struct npa_remote_init_arg {
	uint32_t major;
	uint32_t minor;
	uint32_t build;
	void *callback;
	uint32_t context;
};

struct npa_remote_init_ret {
	int32_t result;
};

struct npa_remote_resource_available_arg {
	const char *resource_name;
	void *callback;
	uint32_t context;
};

struct npa_remote_resource_available_ret {
	int32_t result;
};

struct npa_remote_create_sync_client_arg {
	const char *resource_name;
	const char *client_name;
	uint32_t client_type;
	uint32_t handle_is_valid;
};

struct npa_remote_create_sync_client_ret {
	int32_t result;
	uint32_t handle_is_valid;
	uint32_t handle;
};

struct npa_remote_destroy_client_arg {
	uint32_t handle;
};

struct npa_remote_destroy_client_ret {
	int32_t result;
};

struct npa_remote_issue_required_request_arg {
	uint32_t handle;
	uint32_t state;
	uint32_t new_state_valid;
};

struct npa_remote_issue_required_request_ret {
	int32_t result;
	uint32_t new_state_is_valid;
	uint32_t new_state;
};

static struct msm_rpc_client *npa_rpc_client;
static struct workqueue_struct *npa_rpc_wq;

static int npa_remote_cb(struct msm_rpc_client *client, struct msm_rpc_xdr *xdr)
{
	int err = 0;
	int array_length = 0;
	unsigned int status = RPC_ACCEPTSTAT_SYSTEM_ERR;
	struct npa_remote_cb_data arg;
	npa_remote_callback cb_fn = NULL;

	xdr_recv_uint32(xdr, &arg.cb_id);
	xdr_recv_uint32(xdr, &arg.context);
	xdr_recv_uint32(xdr, &arg.type);
	xdr_recv_array(xdr, (void **)&arg.buffer, &array_length, INT_MAX,
			sizeof(int32_t), (void *)xdr_recv_int32);
	xdr_recv_uint32(xdr, &arg.size);

	cb_fn = (npa_remote_callback) msm_rpc_get_cb_func(client, arg.cb_id);
	if (cb_fn) {
		cb_fn((void *)arg.context, arg.type, arg.buffer, arg.size);
		status = RPC_ACCEPTSTAT_SUCCESS;
	}

	xdr_start_accepted_reply(xdr, status);
	err = xdr_send_msg(xdr);
	if (err) {
		pr_err("NPA Remote callback %s: send accepted reply failed: "
				"%d\n", __func__, err);
		BUG();
	}

	kfree(arg.buffer);

	return 0;
}

static int npa_remote_cb_fn(struct msm_rpc_client *client,
		struct rpc_request_hdr *req, struct msm_rpc_xdr *xdr)
{
	int ret = 0;

	switch (req->procedure) {
	case NPA_REMOTE_CALLBACK_TYPE_PROC:
		ret = npa_remote_cb(client, xdr);
		break;
	default:
		pr_err("NPA RPC: callback procedure not supported %d\n",
				req->procedure);
		xdr_start_accepted_reply(xdr, RPC_ACCEPTSTAT_PROC_UNAVAIL);
		ret = xdr_send_msg(xdr);
		if (ret)
			pr_err("NPA RPC: %s error sending reply %d\n",
					__func__, ret);
		break;
	}

	return ret;
}

int npa_remote_null(void)
{
	return msm_rpc_client_req2(npa_rpc_client, NPA_REMOTE_NULL_PROC,
			NULL, NULL, NULL, NULL, -1);
}

static int npa_remote_init_arg_fn(struct msm_rpc_client *client,
		struct msm_rpc_xdr *xdr, void *data)
{
	struct npa_remote_init_arg *arg = data;
	unsigned int cb_id = 0;

	cb_id = msm_rpc_add_cb_func(client, (void *)arg->callback);
	if (cb_id < 0 && (cb_id != MSM_RPC_CLIENT_NULL_CB_ID))
		return cb_id;

	xdr_send_uint32(xdr, &arg->major);
	xdr_send_uint32(xdr, &arg->minor);
	xdr_send_uint32(xdr, &arg->build);
	xdr_send_uint32(xdr, &cb_id);
	xdr_send_uint32(xdr, &arg->context);

	return 0;
}

static int npa_remote_init_ret_fn(struct msm_rpc_client *client,
		struct msm_rpc_xdr *xdr, void *data)
{
	struct npa_remote_init_ret *ret = data;

	xdr_recv_int32(xdr, &ret->result);

	return 0;
}

int npa_remote_init(unsigned int major, unsigned int minor, unsigned int build,
		npa_remote_callback callback, void *context)
{
	int err = 0;
	struct npa_remote_init_arg arg;
	struct npa_remote_init_ret ret;

	arg.major = major;
	arg.minor = minor;
	arg.build = build;
	arg.callback = (void *)callback;
	arg.context = (uint32_t)context;

	err = msm_rpc_client_req2(npa_rpc_client,
			NPA_REMOTE_INIT_PROC,
			npa_remote_init_arg_fn, &arg,
			npa_remote_init_ret_fn, &ret, -1);
	if (err) {
		pr_err("NPA Remote func %s returned error %d\n", __func__, err);
		BUG();
	}

	return ret.result;
}
EXPORT_SYMBOL(npa_remote_init);

static int npa_remote_resource_available_arg_fn(struct msm_rpc_client *client,
		struct msm_rpc_xdr *xdr, void *data)
{
	struct npa_remote_resource_available_arg *arg = data;
	unsigned int cb_id = 0;
	int len = 0;

	cb_id = msm_rpc_add_cb_func(client, (void *)arg->callback);
	if (cb_id < 0 && (cb_id != MSM_RPC_CLIENT_NULL_CB_ID))
		return cb_id;

	if (arg->resource_name) {
		len = strlen(arg->resource_name) + 1;
		xdr_send_bytes(xdr, (const void **)&arg->resource_name, &len);
	} else {
		xdr_send_uint32(xdr, &len);
	}
	xdr_send_uint32(xdr, &cb_id);
	xdr_send_uint32(xdr, &arg->context);

	return 0;
}

static int npa_remote_resource_available_ret_fn(struct msm_rpc_client *client,
		struct msm_rpc_xdr *xdr, void *data)
{
	struct npa_remote_resource_available_ret *ret = data;

	xdr_recv_uint32(xdr, &ret->result);

	return 0;
}

int npa_remote_resource_available(const char *resource_name,
		npa_remote_callback callback, void *context)
{
	int err = 0;
	struct npa_remote_resource_available_arg arg;
	struct npa_remote_resource_available_ret ret;

	arg.resource_name = resource_name;
	arg.callback = (void *)callback;
	arg.context = (uint32_t)context;

	err = msm_rpc_client_req2(npa_rpc_client,
			NPA_REMOTE_RESOURCE_AVAILABLE_PROC,
			npa_remote_resource_available_arg_fn, &arg,
			npa_remote_resource_available_ret_fn, &ret, -1);
	if (err) {
		pr_err("NPA Remote func %s returned error %d\n", __func__, err);
		BUG();
	}

	return ret.result;
}
EXPORT_SYMBOL(npa_remote_resource_available);

static int npa_remote_create_sync_client_arg_fn(struct msm_rpc_client *client,
		struct msm_rpc_xdr *xdr, void *data)
{
	struct npa_remote_create_sync_client_arg *arg = data;
	int len = 0;

	if (arg->resource_name) {
		len = strlen(arg->resource_name) + 1;
		xdr_send_bytes(xdr, (const void **)&arg->resource_name, &len);
	} else {
		xdr_send_uint32(xdr, &len);
	}
	if (arg->client_name) {
		len = strlen(arg->client_name) + 1;
		xdr_send_bytes(xdr, (const void **)&arg->client_name, &len);
	} else {
		len = 0;
		xdr_send_uint32(xdr, &len);
	}
	xdr_send_uint32(xdr, &arg->client_type);
	xdr_send_uint32(xdr, &arg->handle_is_valid);

	return 0;
}

static int npa_remote_create_sync_client_ret_fn(struct msm_rpc_client *client,
		struct msm_rpc_xdr *xdr, void *data)
{
	struct npa_remote_create_sync_client_ret *ret = data;

	xdr_recv_int32(xdr, &ret->result);
	xdr_recv_uint32(xdr, &ret->handle_is_valid);
	if (ret->handle_is_valid)
		xdr_recv_uint32(xdr, &ret->handle);

	return 0;
}

int npa_remote_create_sync_client(const char *resource_name,
		const char *client_name,
		enum npa_remote_client_type client_type,
		void **handle)
{
	int err = 0;
	struct npa_remote_create_sync_client_arg arg;
	struct npa_remote_create_sync_client_ret ret;

	arg.resource_name = resource_name;
	arg.client_name = client_name;
	arg.client_type = client_type;
	arg.handle_is_valid = handle != NULL;

	err = msm_rpc_client_req2(npa_rpc_client,
			NPA_REMOTE_CREATE_SYNC_CLIENT_PROC,
			npa_remote_create_sync_client_arg_fn, &arg,
			npa_remote_create_sync_client_ret_fn, &ret, -1);
	if (err) {
		pr_err("NPA Remote func %s returned error %d\n", __func__, err);
		BUG();
	}

	if (ret.handle_is_valid)
		*handle = (void *)ret.handle;

	return ret.result;
}
EXPORT_SYMBOL(npa_remote_create_sync_client);

static int npa_remote_destroy_client_arg_fn(struct msm_rpc_client *client,
		struct msm_rpc_xdr *xdr, void *data)
{
	struct npa_remote_destroy_client_arg *arg = data;

	xdr_send_uint32(xdr, &arg->handle);

	return 0;
}

static int npa_remote_destroy_client_ret_fn(struct msm_rpc_client *client,
		struct msm_rpc_xdr *xdr, void *data)
{
	struct npa_remote_destroy_client_ret *ret = data;

	xdr_recv_int32(xdr, &ret->result);

	return 0;
}

int npa_remote_destroy_client(void *handle)
{
	int err = 0;
	struct npa_remote_destroy_client_arg arg;
	struct npa_remote_destroy_client_ret ret;

	arg.handle = (uint32_t)handle;

	err = msm_rpc_client_req2(npa_rpc_client,
			NPA_REMOTE_DESTROY_CLIENT_PROC,
			npa_remote_destroy_client_arg_fn, &arg,
			npa_remote_destroy_client_ret_fn, &ret, -1);
	if (err) {
		pr_err("NPA Remote func %s returned error %d\n", __func__, err);
		BUG();
	}

	return ret.result;
}
EXPORT_SYMBOL(npa_remote_destroy_client);

static int npa_remote_issue_required_request_arg_fn(
		struct msm_rpc_client *client,
		struct msm_rpc_xdr *xdr, void *data)
{
	struct npa_remote_issue_required_request_arg *arg = data;

	xdr_send_uint32(xdr, &arg->handle);
	xdr_send_uint32(xdr, &arg->state);
	xdr_send_uint32(xdr, &arg->new_state_valid);

	return 0;
}

static int npa_remote_issue_required_request_ret_fn(
		struct msm_rpc_client *client,
		struct msm_rpc_xdr *xdr, void *data)
{
	struct npa_remote_issue_required_request_ret *ret = data;

	xdr_recv_int32(xdr, &ret->result);
	xdr_recv_uint32(xdr, &ret->new_state_is_valid);
	if (ret->new_state_is_valid)
		xdr_recv_uint32(xdr, &ret->new_state);

	return 0;
}

int npa_remote_issue_required_request(void *handle, unsigned int state,
		unsigned int *new_state)
{
	int err = 0;
	struct npa_remote_issue_required_request_arg arg;
	struct npa_remote_issue_required_request_ret ret;

	arg.handle = (uint32_t)handle;
	arg.state = state;
	arg.new_state_valid = new_state != NULL;

	err = msm_rpc_client_req2(npa_rpc_client,
			NPA_REMOTE_ISSUE_REQUIRED_REQUEST_PROC,
			npa_remote_issue_required_request_arg_fn, &arg,
			npa_remote_issue_required_request_ret_fn, &ret, -1);
	if (err) {
		pr_err("NPA Remote func %s returned error %d\n", __func__, err);
		BUG();
	}

	if (ret.new_state_is_valid)
		*new_state = ret.new_state;

	return ret.result;
}
EXPORT_SYMBOL(npa_remote_issue_required_request);

#define NPA_PROTOCOL_ONCRPC "/protocols/modem/oncrpc/1.0.0"

unsigned int npa_oncrpc_driver_fn(struct npa_resource *resource,
		struct npa_client *client, unsigned int state)
{
	/* Do nothing */
	return 0;
}

static struct npa_resource_definition npa_oncrpc_resource = {
	.name	= NPA_PROTOCOL_ONCRPC,
	.plugin	= &npa_always_on_plugin,
};

static struct npa_node_definition npa_oncrpc_node = {
	.name			= NPA_PROTOCOL_ONCRPC,
	.driver_fn		= npa_oncrpc_driver_fn,
	.dependencies		= NULL,
	.dependency_count	= 0,
	.resources		= &npa_oncrpc_resource,
	.resource_count		= 1,
};

static void npa_create_rpc_resource(struct work_struct *work)
{
	unsigned int init_state[] = {0};

	/* Define the resource as available */
	/* This will end up defining all the resources that were waiting
	 * on this protocol to be ready.
	 */
	npa_define_node(&npa_oncrpc_node, init_state, NULL, NULL);

	kfree(work);
}

static int npa_remote_verify_cb(void *context, unsigned int size,
		int *data, unsigned int data_size)
{
	/* Schedule a work and release the RPC callback thread */
	struct work_struct *work =
		kzalloc(sizeof(struct work_struct), GFP_KERNEL);
	if (!work)
		return -ENOMEM;

	INIT_WORK(work, npa_create_rpc_resource);
	queue_work(npa_rpc_wq, work);

	return 0;
}

static void npa_remote_verify(struct work_struct *work)
{
	int err = npa_remote_init(NPA_REMOTE_VERSION_MAJOR,
				NPA_REMOTE_VERSION_MINOR,
				NPA_REMOTE_VERSION_BUILD,
				npa_remote_verify_cb, NULL);
	BUG_ON(err);
	kfree(work);
}

/* Registration with the platform driver for notification on the availability
 * of the NPA remote server
 */

static int __devinit npa_rpc_init_probe(struct platform_device *pdev)
{
	struct work_struct *work = NULL;

	if (pdev->id != (NPA_REMOTEVERS & RPC_VERSION_MAJOR_MASK))
		return -EINVAL;

	npa_rpc_client = msm_rpc_register_client2("npa-remote-client",
						NPA_REMOTEPROG,
						NPA_REMOTEVERS,
						1, npa_remote_cb_fn);

	if (IS_ERR(npa_rpc_client)) {
		pr_err("NPA REMOTE: RPC client creation failed\n");
		return -ENODEV;
	}

	/* Schedule a work to initialize NPA remoting.
	 */
	work = kzalloc(sizeof(struct work_struct), GFP_KERNEL);
	if (!work)
		return -ENOMEM;

	INIT_WORK(work, npa_remote_verify);
	queue_work(npa_rpc_wq, work);

	return 0;
}

static char npa_rpc_driver_name[] = "rs00000000:00000000";

static struct platform_driver npa_rpc_init_driver = {
	.probe = npa_rpc_init_probe,
	.driver = {
		.owner = THIS_MODULE,
	},
};

static int __init npa_rpc_init(void)
{
	int err = 0;

	snprintf(npa_rpc_driver_name, sizeof(npa_rpc_driver_name),
		"rs%08x", NPA_REMOTEPROG);
	npa_rpc_init_driver.driver.name = npa_rpc_driver_name;

	npa_rpc_wq = create_workqueue("npa-rpc");
	BUG_ON(!npa_rpc_wq);

	err = platform_driver_register(&npa_rpc_init_driver);

	return err;
}
arch_initcall(npa_rpc_init);
