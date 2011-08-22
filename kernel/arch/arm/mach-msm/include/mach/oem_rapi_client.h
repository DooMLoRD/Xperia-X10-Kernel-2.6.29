/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 * Copyright (C) 2010 Sony Ericsson Mobile Communications AB.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __ASM__ARCH_OEM_RAPI_CLIENT_H
#define __ASM__ARCH_OEM_RAPI_CLIENT_H

/*
 * OEM RAPI CLIENT Driver header file
 */

#include <linux/types.h>
#include <mach/msm_rpcrouter.h>

#define OEM_RAPI_MAX_CLIENT_INPUT_BUFFER_SIZE	128
#define OEM_RAPI_MAX_CLIENT_OUTPUT_BUFFER_SIZE	128
#define OEM_RAPI_MAX_SERVER_INPUT_BUFFER_SIZE	128
#define OEM_RAPI_MAX_SERVER_OUTPUT_BUFFER_SIZE	128

enum {
	OEM_RAPI_CLIENT_EVENT_NONE = 0,

	/*
	 * list of oem rapi client events
	 */

	OEM_RAPI_CLIENT_EVENT_BATT_MV_GET,
	OEM_RAPI_CLIENT_EVENT_BATT_MA_GET,
	OEM_RAPI_CLIENT_EVENT_BATT_ID_GET,
	OEM_RAPI_CLIENT_EVENT_BATT_ID_TYPE1_GET,
	OEM_RAPI_CLIENT_EVENT_BATT_ID_TYPE2_GET,
	OEM_RAPI_CLIENT_EVENT_CUTOFF_LEVEL_CB_REGISTER,
	OEM_RAPI_CLIENT_EVENT_CUTOFF_LEVEL_CB_UNREGISTER_SET,
	OEM_RAPI_CLIENT_EVENT_NOTIFY_PLATFORM_SET,
	OEM_RAPI_CLIENT_EVENT_NOTIFY_BDATA_CB_REGISTER_SET,
	OEM_RAPI_CLIENT_EVENT_NOTIFY_BDATA_CB_UNREGISTER_SET,
	OEM_RAPI_CLIENT_EVENT_NOTIFY_BOOT_CHARGING_INFO,
	OEM_RAPI_CLIENT_EVENT_PM_BATT_FET_SWITCH_SET,

	OEM_RAPI_CLIENT_EVENT_PROXIMITY_THRESHOLD_SET,
	OEM_RAPI_CLIENT_EVENT_PROXIMITY_HYSTERESIS_SET,
	OEM_RAPI_CLIENT_EVENT_PROXIMITY_ON_TIME_SET,
	OEM_RAPI_CLIENT_EVENT_PROXIMITY_OFF_TIME_SET,
	OEM_RAPI_CLIENT_EVENT_PROXIMITY_ACTIVATE,
	OEM_RAPI_CLIENT_EVENT_PROXIMITY_DEACTIVATE,
	OEM_RAPI_CLIENT_EVENT_PROXIMITY_USE_DOUT_SET,
	OEM_RAPI_CLIENT_EVENT_PROXIMITY_VALUE_CB_REGISTER,

	/* Constants used by SEport */
	OEM_RAPI_CLIENT_EVENT_SEPORT_HSSD_MAX_SET,
	OEM_RAPI_CLIENT_EVENT_SEPORT_HSSD_MIN_SET,
	OEM_RAPI_CLIENT_EVENT_SEPORT_HSSD_VAL_GET,
	/* End of constants used by SEport */

	OEM_RAPI_CLIENT_EVENT_MAX

};

/*
 * This enum lists the events that the server can notify the client of.
 */
enum {
	OEM_RAPI_SERVER_EVENT_NONE = 0,

	/*
	 * list of oem rapi server events
	 */

	OEM_RAPI_SERVER_EVENT_CUTOFF_CB_EVENT,
	OEM_RAPI_SERVER_EVENT_NOTIFY_BDATA_CB_EVENT,
	OEM_RAPI_SERVER_EVENT_PROXIMITY_VALUE_CB,

	OEM_RAPI_SERVER_EVENT_MAX
};

struct oem_rapi_client_streaming_func_cb_arg {
	uint32_t  event;
	void      *handle;
	uint32_t  in_len;
	char      *input;
	uint32_t out_len_valid;
	uint32_t output_valid;
	uint32_t output_size;
};

struct oem_rapi_client_streaming_func_cb_ret {
	uint32_t *out_len;
	char *output;
};

struct oem_rapi_client_streaming_func_arg {
	uint32_t event;
	int (*cb_func)(struct oem_rapi_client_streaming_func_cb_arg *,
		       struct oem_rapi_client_streaming_func_cb_ret *);
	void *handle;
	uint32_t in_len;
	char *input;
	uint32_t out_len_valid;
	uint32_t output_valid;
	uint32_t output_size;
};

struct oem_rapi_client_streaming_func_ret {
	uint32_t *out_len;
	char *output;
};

int oem_rapi_client_streaming_function(
	struct msm_rpc_client *client,
	struct oem_rapi_client_streaming_func_arg *arg,
	struct oem_rapi_client_streaming_func_ret *ret);

int oem_rapi_client_close(void);

struct msm_rpc_client *oem_rapi_client_init(void);

#endif
