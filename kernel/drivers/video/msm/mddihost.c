/* Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 * Copyright (C) 2010 Sony Ericsson Mobile Communications AB.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>

#include "msm_fb.h"
#include "mddihost.h"
#include "mddihosti.h"

#include <linux/clk.h>
#include <mach/clk.h>

struct semaphore mddi_host_mutex;

struct clk *mddi_io_clk;
static boolean mddi_host_powered = FALSE;
static boolean mddi_host_initialized = FALSE;
extern uint32 *mddi_reg_read_value_ptr;

mddi_lcd_func_type mddi_lcd;

extern mddi_client_capability_type mddi_client_capability_pkt;

#ifdef FEATURE_MDDI_HITACHI
extern void mddi_hitachi_window_adjust(uint16 x1,
				       uint16 x2, uint16 y1, uint16 y2);
#endif

extern void mddi_toshiba_lcd_init(void);

#ifdef FEATURE_MDDI_S6D0142
extern void mddi_s6d0142_lcd_init(void);
extern void mddi_s6d0142_window_adjust(uint16 x1,
				       uint16 x2,
				       uint16 y1,
				       uint16 y2,
				       mddi_llist_done_cb_type done_cb);
#endif

void mddi_init(void)
{
	if (mddi_host_initialized)
		return;

	mddi_host_initialized = TRUE;

	init_MUTEX(&mddi_host_mutex);

	if (!mddi_host_powered) {
		down(&mddi_host_mutex);
		mddi_host_init(MDDI_HOST_PRIM);
		mddi_host_powered = TRUE;
		up(&mddi_host_mutex);
		mdelay(10);
	}
}

int mddi_host_register_read(uint32 reg_addr,
     uint32 *reg_value_ptr, boolean wait, mddi_host_type host) {
	mddi_linked_list_type *curr_llist_ptr;
	mddi_register_access_packet_type *regacc_pkt_ptr;
	uint16 curr_llist_idx;
	int ret = 0;

	if (in_interrupt())
		MDDI_MSG_CRIT("Called from ISR context\n");

	if (!mddi_host_powered) {
		MDDI_MSG_ERR("MDDI powered down!\n");
		mddi_init();
	}

	down(&mddi_host_mutex);

	mddi_reg_read_value_ptr = reg_value_ptr;
	curr_llist_idx = mddi_get_reg_read_llist_item(host, TRUE);
	if (curr_llist_idx == UNASSIGNED_INDEX) {
		up(&mddi_host_mutex);

		/* need to change this to some sort of wait */
		MDDI_MSG_ERR("Attempting to queue up more than 1 reg read\n");
		return -EINVAL;
	}

	curr_llist_ptr = &llist_extern[host][curr_llist_idx];
	curr_llist_ptr->link_controller_flags = 0x11;
	curr_llist_ptr->packet_header_count = 14;
	curr_llist_ptr->packet_data_count = 0;

	curr_llist_ptr->next_packet_pointer = NULL;
	curr_llist_ptr->packet_data_pointer = NULL;
	curr_llist_ptr->reserved = 0;

	regacc_pkt_ptr = &curr_llist_ptr->packet_header.register_pkt;

	regacc_pkt_ptr->packet_length = curr_llist_ptr->packet_header_count;
	regacc_pkt_ptr->packet_type = 146;	/* register access packet */
	regacc_pkt_ptr->bClient_ID = 0;
	regacc_pkt_ptr->read_write_info = 0x8001;
	regacc_pkt_ptr->register_address = reg_addr;

	/* now adjust pointers */
	mddi_queue_forward_packets(curr_llist_idx, curr_llist_idx, wait,
				   NULL, host);
	/* need to check if we can write the pointer or not */

	up(&mddi_host_mutex);

	if (wait) {
		int wait_ret;

		mddi_linked_list_notify_type *llist_notify_ptr;
		llist_notify_ptr = &llist_extern_notify[host][curr_llist_idx];
		wait_ret = wait_for_completion_timeout(
					&(llist_notify_ptr->done_comp), 5 * HZ);

		if (wait_ret <= 0)
			ret = -EBUSY;

		if (wait_ret < 0)
			printk(KERN_ERR "%s: failed to wait for completion!\n",
				__func__);
		else if (!wait_ret)
			printk(KERN_ERR "%s: Timed out waiting!\n", __func__);

		if (!ret && (mddi_reg_read_value_ptr == reg_value_ptr) &&
			(*reg_value_ptr == -EBUSY)) {
			printk(KERN_ERR "%s - failed to get data from client",
				   __func__);
			mddi_reg_read_value_ptr = NULL;
			ret = -EBUSY;
		}
	}

	MDDI_MSG_DEBUG("Reg Read value=0x%x\n", *reg_value_ptr);

	return ret;
}				/* mddi_host_register_read */
EXPORT_SYMBOL(mddi_host_register_read);

int mddi_host_register_write_xl
		(uint32 reg_addr,
		 uint32 *reg_val_ext,	/* array of register data */
		 uint32 reg_nbrs,
		 boolean wait, mddi_llist_done_cb_type done_cb, mddi_host_type host) {
	mddi_linked_list_type *curr_llist_ptr;
	mddi_linked_list_type *curr_llist_dma_ptr;
	mddi_register_access_packet_xl_type *regacc_pkt_ptr;
	uint16 curr_llist_idx;
	uint16 idx_values = 0;
	int ret = 0;

	if (in_interrupt())
		MDDI_MSG_CRIT("Called from ISR context\n");

	if (!mddi_host_powered) {
		MDDI_MSG_ERR("MDDI powered down!\n");
		mddi_init();
	}

	down(&mddi_host_mutex);

	curr_llist_idx = mddi_get_next_free_llist_item(host, TRUE);
	curr_llist_ptr = &llist_extern[host][curr_llist_idx];
	curr_llist_dma_ptr = &llist_dma_extern[host][curr_llist_idx];

	curr_llist_ptr->link_controller_flags = 1;
	curr_llist_ptr->packet_header_count = 14;
	curr_llist_ptr->packet_data_count = reg_nbrs*4;

	curr_llist_ptr->next_packet_pointer = NULL;
	curr_llist_ptr->reserved = 0;

	regacc_pkt_ptr = &curr_llist_ptr->packet_header.register_xl_pkt;

	regacc_pkt_ptr->packet_length = curr_llist_ptr->packet_header_count + curr_llist_ptr->packet_data_count; //4;
	regacc_pkt_ptr->packet_type = 146;	/* register access packet */
	regacc_pkt_ptr->bClient_ID = 0;
	regacc_pkt_ptr->read_write_info = reg_nbrs;
	regacc_pkt_ptr->register_address = reg_addr;
	regacc_pkt_ptr->register_data_list = reg_val_ext[0];

	/* Handle multiple parameter writes to a single register */
	if ( (reg_nbrs > 1) && (reg_nbrs <= MDDI_ACCESS_PKT_REG_DATA_EXT) ){
		for (idx_values = 1; idx_values < reg_nbrs; idx_values++) {
			regacc_pkt_ptr->register_data_list_ext[idx_values - 1] = reg_val_ext[idx_values];
		}
	}

	MDDI_MSG_DEBUG("Reg Access write reg=0x%x, value=0x%x\n",
		       regacc_pkt_ptr->register_address,
		       regacc_pkt_ptr->register_data_list);

	regacc_pkt_ptr = &curr_llist_dma_ptr->packet_header.register_xl_pkt;
	curr_llist_ptr->packet_data_pointer =
	    (void *)(&regacc_pkt_ptr->register_data_list);

	/* now adjust pointers */
	mddi_queue_forward_packets(curr_llist_idx, curr_llist_idx, wait,
				   done_cb, host);

	up(&mddi_host_mutex);

	if (wait) {
		int wait_ret;

		mddi_linked_list_notify_type *llist_notify_ptr;
		llist_notify_ptr = &llist_extern_notify[host][curr_llist_idx];
		wait_ret = wait_for_completion_timeout(
					&(llist_notify_ptr->done_comp), 5 * HZ);

		if (wait_ret <= 0)
			ret = -EBUSY;

		if (wait_ret < 0)
			printk(KERN_ERR "%s: failed to wait for completion!\n",
				__func__);
		else if (!wait_ret)
			printk(KERN_ERR "%s: Timed out waiting!\n", __func__);
	}

	return ret;
}
EXPORT_SYMBOL(mddi_host_register_write_xl);

int mddi_host_register_write16
    (uint32 reg_addr,
     uint32 reg_val0,
     uint32 reg_val1,
     uint32 reg_val2,
     uint32 reg_val3,
     uint32 reg_nbrs,
     boolean wait, mddi_llist_done_cb_type done_cb, mddi_host_type host)
{
	mddi_linked_list_type *curr_llist_ptr;
	mddi_linked_list_type *curr_llist_dma_ptr;
	mddi_register_access_packet_type *regacc_pkt_ptr;
	uint16 curr_llist_idx;
	int ret = 0;

	if (in_interrupt())
		MDDI_MSG_CRIT("Called from ISR context\n");

	if (!mddi_host_powered) {
		MDDI_MSG_ERR("MDDI powered down!\n");
		mddi_init();
	}

	down(&mddi_host_mutex);

	curr_llist_idx = mddi_get_next_free_llist_item(host, TRUE);
	curr_llist_ptr = &llist_extern[host][curr_llist_idx];
	curr_llist_dma_ptr = &llist_dma_extern[host][curr_llist_idx];

	curr_llist_ptr->link_controller_flags = 1;
	curr_llist_ptr->packet_header_count = 14;
	curr_llist_ptr->packet_data_count = reg_nbrs*4;

	curr_llist_ptr->next_packet_pointer = NULL;
	curr_llist_ptr->reserved = 0;

	regacc_pkt_ptr = &curr_llist_ptr->packet_header.register_pkt;

	regacc_pkt_ptr->packet_length = curr_llist_ptr->packet_header_count + curr_llist_ptr->packet_data_count; //4;
	regacc_pkt_ptr->packet_type = 146;	/* register access packet */
	regacc_pkt_ptr->bClient_ID = 0;
	regacc_pkt_ptr->read_write_info = reg_nbrs;
	regacc_pkt_ptr->register_address = reg_addr;
	regacc_pkt_ptr->register_data_list = reg_val0;
	regacc_pkt_ptr->register_data_list_ext[0] = reg_val1;
	regacc_pkt_ptr->register_data_list_ext[1] = reg_val2;
	regacc_pkt_ptr->register_data_list_ext[2] = reg_val3;

	MDDI_MSG_DEBUG("Reg Access write reg=0x%x, value=0x%x\n",
		       regacc_pkt_ptr->register_address,
		       regacc_pkt_ptr->register_data_list);

	regacc_pkt_ptr = &curr_llist_dma_ptr->packet_header.register_pkt;
	curr_llist_ptr->packet_data_pointer =
	    (void *)(&regacc_pkt_ptr->register_data_list);

	/* now adjust pointers */
	mddi_queue_forward_packets(curr_llist_idx, curr_llist_idx, wait,
				   done_cb, host);

	up(&mddi_host_mutex);

	if (wait) {
		int wait_ret;

		mddi_linked_list_notify_type *llist_notify_ptr;
		llist_notify_ptr = &llist_extern_notify[host][curr_llist_idx];
		wait_ret = wait_for_completion_timeout(
					&(llist_notify_ptr->done_comp), 5 * HZ);

		if (wait_ret <= 0)
			ret = -EBUSY;

		if (wait_ret < 0)
			printk(KERN_ERR "%s: failed to wait for completion!\n",
				__func__);
		else if (!wait_ret)
			printk(KERN_ERR "%s: Timed out waiting!\n", __func__);
	}

	return ret;
}
EXPORT_SYMBOL(mddi_host_register_write16);

int mddi_host_register_write(uint32 reg_addr,
     uint32 reg_val, enum mddi_data_packet_size_type packet_size,
     boolean wait, mddi_llist_done_cb_type done_cb, mddi_host_type host) {
	mddi_linked_list_type *curr_llist_ptr;
	mddi_linked_list_type *curr_llist_dma_ptr;
	mddi_register_access_packet_type *regacc_pkt_ptr;
	uint16 curr_llist_idx;
	int ret = 0;

	if (in_interrupt())
		MDDI_MSG_CRIT("Called from ISR context\n");

	if (!mddi_host_powered) {
		MDDI_MSG_ERR("MDDI powered down!\n");
		mddi_init();
	}

	down(&mddi_host_mutex);

	curr_llist_idx = mddi_get_next_free_llist_item(host, TRUE);
	curr_llist_ptr = &llist_extern[host][curr_llist_idx];
	curr_llist_dma_ptr = &llist_dma_extern[host][curr_llist_idx];

	curr_llist_ptr->link_controller_flags = 1;
	curr_llist_ptr->packet_header_count = 14;
	curr_llist_ptr->packet_data_count = 4;

	curr_llist_ptr->next_packet_pointer = NULL;
	curr_llist_ptr->reserved = 0;

	regacc_pkt_ptr = &curr_llist_ptr->packet_header.register_pkt;

	regacc_pkt_ptr->packet_length = curr_llist_ptr->packet_header_count +
					(uint16)packet_size;
	regacc_pkt_ptr->packet_type = 146;	/* register access packet */
	regacc_pkt_ptr->bClient_ID = 0;
	regacc_pkt_ptr->read_write_info = 0x0001;
	regacc_pkt_ptr->register_address = reg_addr;
	regacc_pkt_ptr->register_data_list = reg_val;

	MDDI_MSG_DEBUG("Reg Access write reg=0x%x, value=0x%x\n",
		       regacc_pkt_ptr->register_address,
		       regacc_pkt_ptr->register_data_list);

	regacc_pkt_ptr = &curr_llist_dma_ptr->packet_header.register_pkt;
	curr_llist_ptr->packet_data_pointer =
	    (void *)(&regacc_pkt_ptr->register_data_list);

	/* now adjust pointers */
	mddi_queue_forward_packets(curr_llist_idx, curr_llist_idx, wait,
				   done_cb, host);

	up(&mddi_host_mutex);

	if (wait) {
		int wait_ret;

		mddi_linked_list_notify_type *llist_notify_ptr;
		llist_notify_ptr = &llist_extern_notify[host][curr_llist_idx];
		wait_ret = wait_for_completion_timeout(
					&(llist_notify_ptr->done_comp), 5 * HZ);

		if (wait_ret <= 0)
			ret = -EBUSY;

		if (wait_ret < 0)
			printk(KERN_ERR "%s: failed to wait for completion!\n",
				__func__);
		else if (!wait_ret)
			printk(KERN_ERR "%s: Timed out waiting!\n", __func__);
	}

	return ret;
}				/* mddi_host_register_write */

boolean mddi_host_register_read_int
    (uint32 reg_addr, uint32 *reg_value_ptr, mddi_host_type host) {
	mddi_linked_list_type *curr_llist_ptr;
	mddi_register_access_packet_type *regacc_pkt_ptr;
	uint16 curr_llist_idx;

	if (!in_interrupt())
		MDDI_MSG_CRIT("Called from TASK context\n");

	if (!mddi_host_powered) {
		MDDI_MSG_ERR("MDDI powered down!\n");
		return FALSE;
	}

	if (down_trylock(&mddi_host_mutex) != 0)
		return FALSE;

	mddi_reg_read_value_ptr = reg_value_ptr;
	curr_llist_idx = mddi_get_reg_read_llist_item(host, FALSE);
	if (curr_llist_idx == UNASSIGNED_INDEX) {
		up(&mddi_host_mutex);
		return FALSE;
	}

	curr_llist_ptr = &llist_extern[host][curr_llist_idx];
	curr_llist_ptr->link_controller_flags = 0x11;
	curr_llist_ptr->packet_header_count = 14;
	curr_llist_ptr->packet_data_count = 0;

	curr_llist_ptr->next_packet_pointer = NULL;
	curr_llist_ptr->packet_data_pointer = NULL;
	curr_llist_ptr->reserved = 0;

	regacc_pkt_ptr = &curr_llist_ptr->packet_header.register_pkt;

	regacc_pkt_ptr->packet_length = curr_llist_ptr->packet_header_count;
	regacc_pkt_ptr->packet_type = 146;	/* register access packet */
	regacc_pkt_ptr->bClient_ID = 0;
	regacc_pkt_ptr->read_write_info = 0x8001;
	regacc_pkt_ptr->register_address = reg_addr;

	/* now adjust pointers */
	mddi_queue_forward_packets(curr_llist_idx, curr_llist_idx, FALSE,
				   NULL, host);
	/* need to check if we can write the pointer or not */

	up(&mddi_host_mutex);

	return TRUE;

}				/* mddi_host_register_read */

boolean mddi_host_register_write_int
    (uint32 reg_addr,
     uint32 reg_val, mddi_llist_done_cb_type done_cb, mddi_host_type host) {
	mddi_linked_list_type *curr_llist_ptr;
	mddi_linked_list_type *curr_llist_dma_ptr;
	mddi_register_access_packet_type *regacc_pkt_ptr;
	uint16 curr_llist_idx;

	if (!in_interrupt())
		MDDI_MSG_CRIT("Called from TASK context\n");

	if (!mddi_host_powered) {
		MDDI_MSG_ERR("MDDI powered down!\n");
		return FALSE;
	}

	if (down_trylock(&mddi_host_mutex) != 0)
		return FALSE;

	curr_llist_idx = mddi_get_next_free_llist_item(host, FALSE);
	if (curr_llist_idx == UNASSIGNED_INDEX) {
		up(&mddi_host_mutex);
		return FALSE;
	}

	curr_llist_ptr = &llist_extern[host][curr_llist_idx];
	curr_llist_dma_ptr = &llist_dma_extern[host][curr_llist_idx];

	curr_llist_ptr->link_controller_flags = 1;
	curr_llist_ptr->packet_header_count = 14;
	curr_llist_ptr->packet_data_count = 4;

	curr_llist_ptr->next_packet_pointer = NULL;
	curr_llist_ptr->reserved = 0;

	regacc_pkt_ptr = &curr_llist_ptr->packet_header.register_pkt;

	regacc_pkt_ptr->packet_length = curr_llist_ptr->packet_header_count + 4;
	regacc_pkt_ptr->packet_type = 146;	/* register access packet */
	regacc_pkt_ptr->bClient_ID = 0;
	regacc_pkt_ptr->read_write_info = 0x0001;
	regacc_pkt_ptr->register_address = reg_addr;
	regacc_pkt_ptr->register_data_list = reg_val;

	regacc_pkt_ptr = &curr_llist_dma_ptr->packet_header.register_pkt;
	curr_llist_ptr->packet_data_pointer =
	    (void *)(&(regacc_pkt_ptr->register_data_list));

	/* now adjust pointers */
	mddi_queue_forward_packets(curr_llist_idx, curr_llist_idx, FALSE,
				   done_cb, host);
	up(&mddi_host_mutex);

	return TRUE;

}				/* mddi_host_register_write */

void mddi_wait(uint16 time_ms)
{
	mdelay(time_ms);
}
EXPORT_SYMBOL(mddi_wait);

void mddi_client_lcd_vsync_detected(boolean detected)
{
	if (mddi_lcd.vsync_detected)
		(*mddi_lcd.vsync_detected) (detected);
}

/* extended version of function includes done callback */
void mddi_window_adjust_ext(struct msm_fb_data_type *mfd,
			    uint16 x1,
			    uint16 x2,
			    uint16 y1,
			    uint16 y2, mddi_llist_done_cb_type done_cb)
{
#ifdef FEATURE_MDDI_HITACHI
	if (mfd->panel.id == HITACHI)
		mddi_hitachi_window_adjust(x1, x2, y1, y2);
#elif defined(FEATURE_MDDI_S6D0142)
	if (mfd->panel.id == MDDI_LCD_S6D0142)
		mddi_s6d0142_window_adjust(x1, x2, y1, y2, done_cb);
#else
	struct msm_fb_panel_data *pdata;
	pdata = (struct msm_fb_panel_data *)mfd->pdev->dev.platform_data;

	if (pdata && pdata->panel_ext && pdata->panel_ext->window_adjust)
		pdata->panel_ext->window_adjust(x1, x2, y1, y2);
#endif
}

void mddi_window_adjust(struct msm_fb_data_type *mfd,
			uint16 x1, uint16 x2, uint16 y1, uint16 y2)
{
	mddi_window_adjust_ext(mfd, x1, x2, y1, y2, NULL);
}

boolean mddi_video_stream_black_display(uint32 x0, uint32 y0,
			uint32 width, uint32 height, mddi_host_type host)
{
	uint32 row = 0;
	uint32 depth = 16;
	uint32 rows_per_block = 16;
	size_t pixelbytes_per_row = width * (depth / 8);
	dma_addr_t  pixeldata_dma_addr;
	void *pixeldata_ptr;

	if (in_interrupt())
		return FALSE;
	if (!mddi_host_powered)
		return FALSE;

	pixeldata_ptr = dma_alloc_coherent(NULL,
					pixelbytes_per_row * rows_per_block,
					&pixeldata_dma_addr, GFP_KERNEL);

	if (pixeldata_ptr != NULL) {
		int wait_ret = 0;
		boolean wait = FALSE;
		uint16 curr_llist_idx = UNASSIGNED_INDEX;
		mddi_linked_list_notify_type *llist_notify_ptr;

		memset(pixeldata_ptr, 0 /* black */,
			pixelbytes_per_row * rows_per_block);

		MDDI_MSG_DEBUG("%s: mutex down\n", __func__);
		down(&mddi_host_mutex);

		while (height > row) {
			mddi_linked_list_type *curr_llist_ptr;
			mddi_linked_list_type *curr_llist_dma_ptr;
			mddi_video_stream_packet_type *video_pkt_ptr;

			/* always wait for a new available llist item */
			curr_llist_idx =
				mddi_get_next_free_llist_item(host, TRUE);
			curr_llist_ptr = &llist_extern[host][curr_llist_idx];
			curr_llist_dma_ptr =
				&llist_dma_extern[host][curr_llist_idx];
			curr_llist_ptr->link_controller_flags = 1;
			curr_llist_ptr->packet_header_count = 26;
			curr_llist_ptr->packet_data_count =
				pixelbytes_per_row * rows_per_block;
			curr_llist_ptr->next_packet_pointer = NULL;
			curr_llist_ptr->reserved = 0;

			/* video stream packet */
			video_pkt_ptr =
				&curr_llist_ptr->packet_header.video_pkt;
			video_pkt_ptr->packet_length =
				curr_llist_ptr->packet_header_count +
				curr_llist_ptr->packet_data_count;
			video_pkt_ptr->packet_type = 0x10;
			video_pkt_ptr->bClient_ID = 0;
			video_pkt_ptr->video_data_format_descriptor = 0x5565;
			video_pkt_ptr->pixel_data_attributes = 0x00C3;
			video_pkt_ptr->x_left_edge = x0;
			video_pkt_ptr->y_top_edge = y0;
			video_pkt_ptr->x_right_edge = x0 + width - 1;
			video_pkt_ptr->y_bottom_edge = y0 + height - 1;
			video_pkt_ptr->x_start = 0;
			video_pkt_ptr->y_start = row;
			video_pkt_ptr->pixel_count = width * rows_per_block;
			video_pkt_ptr->parameter_CRC = 0;
			curr_llist_ptr->packet_data_pointer =
				(void *)pixeldata_dma_addr;
			MDDI_MSG_DEBUG("row=%d, idx=%d\n",
				 row, curr_llist_idx);

			row += rows_per_block;

			/* need to wait for the last packet to finish */
			if (row >= height)
				wait = TRUE;

			/* add a packet to the queue */
			mddi_queue_forward_packets(curr_llist_idx,
				curr_llist_idx, wait, NULL, host);
		}
		up(&mddi_host_mutex);
		MDDI_MSG_DEBUG("%s: mutex up\n", __func__);

		llist_notify_ptr =
			&llist_extern_notify[host][curr_llist_idx];
		wait_ret = wait_for_completion_timeout(
			&(llist_notify_ptr->done_comp), 5 * HZ);

		dma_free_coherent(NULL,
			pixelbytes_per_row * rows_per_block,
			pixeldata_ptr,
			pixeldata_dma_addr);

		if (wait_ret <= 0)
			return FALSE;

		MDDI_MSG_DEBUG("%s: finished ok!\n", __func__);
		return TRUE;
	} else {
		return FALSE;
	}
}
EXPORT_SYMBOL(mddi_video_stream_black_display);
