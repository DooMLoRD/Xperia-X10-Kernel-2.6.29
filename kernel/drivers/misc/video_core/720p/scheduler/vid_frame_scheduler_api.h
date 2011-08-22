/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
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
#ifndef _SCHEDULER_API_H_
#define _SCHEDULER_API_H_

enum sched_status_type {
	SCHED_S_OK = 0x0,
	SCHED_S_NOPTKN,
	SCHED_S_NOOTKN,
	SCHED_S_SLEEP,
	SCHED_S_QEMPTY,
	SCHED_S_QFULL,
	SCHED_S_EOF,
	SCHED_S_EFAIL = 0x64,
	SCHED_S_ENOMEM,
	SCHED_S_EBADPARM,
	SCHED_S_EINVALOP,
	SCHED_S_ENOTIMPL,
	SCHED_S_ENORES,
	SCHED_S_EINVALST,
	SCHED_S_MAX = 0x7fffffff
};

enum sched_index_type {
	SCHED_I_START_UNUSED = 0x0,
	SCHED_I_PERFLEVEL,
	SCHED_I_CLNT_START_UNUSED = 0x63,
	SCHED_I_CLNT_CURRQLEN,
	SCHED_I_CLNT_PTKNRATE,
	SCHED_I_CLNT_PTKNPERFRM,
	SCHED_I_CLNT_FRAMERATE,
	SCHED_I_CLNT_OTKNMAX,
	SCHED_I_CLNT_OTKNPERIPFRM,
	SCHED_I_CLNT_OTKNCURRENT,
	SCHED_I_MAX = 0x7fffffff
};

struct sched_client_frm_rate_type {
	u32 n_numer;
	u32 n_denom;

};

union sched_value_type {
	u32 un_value;
	struct sched_client_frm_rate_type frm_rate;

};

struct sched_init_param_type {
	u32 n_perf_lvl;

};

enum sched_client_ctgy_type {
	SCHED_CLNT_RT_BUFF = 0,
	SCHED_CLNT_RT_NOBUFF,
	SCHED_CLNT_NONRT,
	SCHED_CLNT_MAX = 0x7fffffff
};

struct sched_client_init_param_type {
	enum sched_client_ctgy_type client_ctgy;
	u32 n_max_queue_len;
	struct sched_client_frm_rate_type frm_rate;
	u32 n_p_tkn_per_frm;
	u32 n_alloc_p_tkn_rate;
	u32 n_o_tkn_max;
	u32 n_o_tkn_per_ip_frm;
	u32 n_o_tkn_init;

	void *p_client_data;

};

enum sched_status_type sched_create
    (struct sched_init_param_type *init_param, void **p_handle);

enum sched_status_type sched_destroy(void *handle);

enum sched_status_type sched_get_param
    (void *handle,
     enum sched_index_type param_index, union sched_value_type *p_param_value);

enum sched_status_type sched_set_param
    (void *handle,
     enum sched_index_type param_index, union sched_value_type *p_param_value);

enum sched_status_type sched_add_client
    (void *handle,
     struct sched_client_init_param_type *init_param, void **p_client_hdl);

enum sched_status_type sched_remove_client(void *handle, void *client_hdl);

enum sched_status_type sched_flush_client_buffer
    (void *handle, void *client_hdl, void **pp_frm_data);

enum sched_status_type sched_mark_client_eof(void *handle, void *client_hdl);

enum sched_status_type sched_update_client_o_tkn
    (void *handle, void *client_hdl, u32 b_type, u32 n_o_tkn);

enum sched_status_type sched_queue_frame
    (void *handle, void *client_hdl, void *p_frm_data);
enum sched_status_type sched_re_queue_frame
(void *handle, void *client_hdl, void *p_frm_data);

enum sched_status_type sched_de_queue_frame
    (void *handle, void **pp_frm_data, void **pp_client_data);

enum sched_status_type sched_get_client_param
    (void *handle,
     void *client_hdl,
     enum sched_index_type param_index, union sched_value_type *p_param_value);

enum sched_status_type sched_set_client_param
    (void *handle,
     void *client_hdl,
     enum sched_index_type param_index, union sched_value_type *p_param_value);

enum sched_status_type sched_suspend_resume_client
    (void *handle, void *client_hdl, u32 b_state);

#endif
