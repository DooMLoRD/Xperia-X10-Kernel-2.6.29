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

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include <mach/camera.h>
#include "vx6953.h"

/*=============================================================
	SENSOR REGISTER DEFINES
==============================================================*/

#define REG_GROUPED_PARAMETER_HOLD			0x0104
#define GROUPED_PARAMETER_HOLD_OFF			0x00
#define GROUPED_PARAMETER_HOLD				0x01
#define REG_MODE_SELECT						0x0100
#define MODE_SELECT_STANDBY_MODE			0x00
#define MODE_SELECT_STREAM				0x01
/* Integration Time */
#define REG_COARSE_INTEGRATION_TIME_HI		0x0202
#define REG_COARSE_INTEGRATION_TIME_LO	0x0203
/* Gain */
#define REG_ANALOGUE_GAIN_CODE_GLOBAL_HI	0x0204
#define REG_ANALOGUE_GAIN_CODE_GLOBAL_LO	0x0205
/* Digital Gain */
#define REG_DIGITAL_GAIN_GREEN_R_HI		0x020E
#define REG_DIGITAL_GAIN_GREEN_R_LO		0x020F
#define REG_DIGITAL_GAIN_RED_HI			0x0210
#define REG_DIGITAL_GAIN_RED_LO			0x0211
#define REG_DIGITAL_GAIN_BLUE_HI		0x0212
#define REG_DIGITAL_GAIN_BLUE_LO		0x0213
#define REG_DIGITAL_GAIN_GREEN_B_HI		0x0214
#define REG_DIGITAL_GAIN_GREEN_B_LO		0x0215
/* output bits setting */
#define REG_0x0112					0x0112
#define REG_0x0113					0x0113
/* PLL registers */
#define REG_VT_PIX_CLK_DIV			0x0301
#define REG_PRE_PLL_CLK_DIV			0x0305
#define REG_PLL_MULTIPLIER			0x0307
#define REG_OP_PIX_CLK_DIV			0x0309
#define REG_0x034c					0x034c
#define REG_0x034d					0x034d
#define REG_0x034e					0x034e
#define REG_0x034f					0x034f
#define REG_0x0387					0x0387
#define REG_0x0383					0x0383
#define REG_FRAME_LENGTH_LINES_HI		0x0340
#define REG_FRAME_LENGTH_LINES_LO		0x0341
#define REG_LINE_LENGTH_PCK_HI		0x0342
#define REG_LINE_LENGTH_PCK_LO		0x0343
#define REG_0x3030					0x3030
#define REG_0x0111					0x0111
#define REG_0x0b00					0x0b00
#define REG_0x3001					0x3001
#define REG_0x3004					0x3004
#define REG_0x3007					0x3007
#define REG_0x301a					0x301a
#define REG_0x3101					0x3101
#define REG_0x3364					0x3364
#define REG_0x3365					0x3365
#define REG_0x0b83					0x0b83
#define REG_0x0b84					0x0b84
#define REG_0x0b8a					0x0b8a
#define REG_0x3005					0x3005
#define REG_0x3036					0x3036
#define REG_0x3041					0x3041
#define REG_0x0b80					0x0b80
#define REG_0x0900					0x0900
#define REG_0x0901					0x0901
#define REG_0x0902					0x0902
#define REG_0x3016					0x3016
#define REG_0x301d					0x301d
#define REG_0x317e					0x317e
#define REG_0x317f					0x317f
#define REG_0x3400					0x3400
#define REG_0x303a					0x303a
#define REG_0x1716					0x1716
#define REG_0x1717					0x1717
#define REG_0x1718					0x1718
#define REG_0x1719					0x1719
#define REG_0x3011					0x3011
#define REG_0x3035					0x3035
#define REG_0x3045					0x3045
/* Test Pattern */
#define REG_TEST_PATTERN_MODE		0x0601

/*============================================================================
							 TYPE DECLARATIONS
============================================================================*/

/* 16bit address - 8 bit context register structure */
#define	VX6953_STM5M0EDOF_OFFSET	9
#define	Q8		0x00000100
#define	VX6953_STM5M0EDOF_MAX_SNAPSHOT_EXPOSURE_LINE_COUNT	2922
#define	VX6953_STM5M0EDOF_DEFAULT_MASTER_CLK_RATE	24000000
#define	VX6953_STM5M0EDOF_OP_PIXEL_CLOCK_RATE	79800000
#define	VX6953_STM5M0EDOF_VT_PIXEL_CLOCK_RATE	88670000
/* Full	Size */
#define	VX6953_FULL_SIZE_WIDTH	2608
#define	VX6953_FULL_SIZE_HEIGHT		1960
#define	VX6953_FULL_SIZE_DUMMY_PIXELS	1
#define	VX6953_FULL_SIZE_DUMMY_LINES	0
/* Quarter Size	*/
#define	VX6953_QTR_SIZE_WIDTH	1304
#define	VX6953_QTR_SIZE_HEIGHT		980
#define	VX6953_QTR_SIZE_DUMMY_PIXELS	1
#define	VX6953_QTR_SIZE_DUMMY_LINES		0
/* Blanking	as measured	on the scope */
/* Full	Size */
#define	VX6953_HRZ_FULL_BLK_PIXELS	348
#define	VX6953_VER_FULL_BLK_LINES	40
/* Quarter Size	*/
#define	VX6953_HRZ_QTR_BLK_PIXELS	1628
#define	VX6953_VER_QTR_BLK_LINES	28
#define	MAX_LINE_LENGTH_PCK		8190
#define	VX6953_REVISION_NUMBER	0x10/*revision number	for	Cut2.0*/
/* FIXME: Changes from here */
struct vx6953_work_t {
	struct work_struct work;
};

static struct vx6953_work_t *vx6953_sensorw;
static struct i2c_client *vx6953_client;

struct vx6953_ctrl_t {
	const struct  msm_camera_sensor_info *sensordata;

	uint32_t sensormode;
	uint32_t fps_divider;   	/* init to 1 * 0x00000400 */
	uint32_t pict_fps_divider;  /* init to 1 * 0x00000400 */
	uint16_t fps;

	int16_t curr_lens_pos;
	uint16_t curr_step_pos;
	uint16_t my_reg_gain;
	uint32_t my_reg_line_count;
	uint16_t total_lines_per_frame;

	enum vx6953_resolution_t prev_res;
	enum vx6953_resolution_t pict_res;
	enum vx6953_resolution_t curr_res;
	enum vx6953_test_mode_t  set_test;
	enum sensor_revision_t sensor_type;

	enum edof_mode_t edof_mode;

	unsigned short imgaddr;
};


static uint8_t vx6953_stm5m0edof_delay_msecs_stdby;
static uint16_t vx6953_stm5m0edof_delay_msecs_stream = 20;

static struct vx6953_ctrl_t *vx6953_ctrl;
static DECLARE_WAIT_QUEUE_HEAD(vx6953_wait_queue);
DEFINE_MUTEX(vx6953_mut);
static struct vx6953_i2c_reg_conf patch_tbl[] = {
	{0xFB94, 0},	/*intialise Data Xfer Status reg*/
	{0xFB95, 0},	/*gain 1	  (0x00)*/
	{0xFB96, 0},	/*gain 1.07   (0x10)*/
	{0xFB97, 0},	/*gain 1.14   (0x20)*/
	{0xFB98, 0},	/*gain 1.23   (0x30)*/
	{0xFB99, 0},	/*gain 1.33   (0x40)*/
	{0xFB9A, 0},	/*gain 1.45   (0x50)*/
	{0xFB9B, 0},	/*gain 1.6    (0x60)*/
	{0xFB9C, 0},	/*gain 1.78   (0x70)*/
	{0xFB9D, 2},	/*gain 2	  (0x80)*/
	{0xFB9E, 2},	/*gain 2.29   (0x90)*/
	{0xFB9F, 3},	/*gain 2.67   (0xA0)*/
	{0xFBA0, 3},	/*gain 3.2    (0xB0)*/
	{0xFBA1, 4},	/*gain 4	  (0xC0)*/
	{0xFBA2, 7},	/*gain 5.33   (0xD0)*/
	{0xFBA3, 10},	/*gain 8	  (0xE0)*/
	{0xFBA4, 11},	/*gain 9.14   (0xE4)*/
	{0xFBA5, 13},	/*gain 10.67  (0xE8)*/
	{0xFBA6, 15},	/*gain 12.8   (0xEC)*/
	{0xFBA7, 19},	/*gain 16     (0xF0)*/
	{0xF800, 0x12},
	{0xF801, 0x06},
	{0xF802, 0xf7},
	{0xF803, 0x90},
	{0xF804, 0x02},
	{0xF805, 0x05},
	{0xF806, 0xe0},
	{0xF807, 0xff},
	{0xF808, 0x65},
	{0xF809, 0x7d},
	{0xF80A, 0x70},
	{0xF80B, 0x03},
	{0xF80C, 0x02},
	{0xF80D, 0xf9},
	{0xF80E, 0x1c},
	{0xF80F, 0x8f},
	{0xF810, 0x7d},
	{0xF811, 0xe4},
	{0xF812, 0xf5},
	{0xF813, 0x7a},
	{0xF814, 0x75},
	{0xF815, 0x78},
	{0xF816, 0x30},
	{0xF817, 0x75},
	{0xF818, 0x79},
	{0xF819, 0x53},
	{0xF81A, 0x85},
	{0xF81B, 0x79},
	{0xF81C, 0x82},
	{0xF81D, 0x85},
	{0xF81E, 0x78},
	{0xF81F, 0x83},
	{0xF820, 0xe0},
	{0xF821, 0xc3},
	{0xF822, 0x95},
	{0xF823, 0x7b},
	{0xF824, 0xf0},
	{0xF825, 0x74},
	{0xF826, 0x02},
	{0xF827, 0x25},
	{0xF828, 0x79},
	{0xF829, 0xf5},
	{0xF82A, 0x79},
	{0xF82B, 0xe4},
	{0xF82C, 0x35},
	{0xF82D, 0x78},
	{0xF82E, 0xf5},
	{0xF82F, 0x78},
	{0xF830, 0x05},
	{0xF831, 0x7a},
	{0xF832, 0xe5},
	{0xF833, 0x7a},
	{0xF834, 0xb4},
	{0xF835, 0x08},
	{0xF836, 0xe3},
	{0xF837, 0xe5},
	{0xF838, 0x7d},
	{0xF839, 0x70},
	{0xF83A, 0x04},
	{0xF83B, 0xff},
	{0xF83C, 0x02},
	{0xF83D, 0xf8},
	{0xF83E, 0xe4},
	{0xF83F, 0xe5},
	{0xF840, 0x7d},
	{0xF841, 0xb4},
	{0xF842, 0x10},
	{0xF843, 0x05},
	{0xF844, 0x7f},
	{0xF845, 0x01},
	{0xF846, 0x02},
	{0xF847, 0xf8},
	{0xF848, 0xe4},
	{0xF849, 0xe5},
	{0xF84A, 0x7d},
	{0xF84B, 0xb4},
	{0xF84C, 0x20},
	{0xF84D, 0x05},
	{0xF84E, 0x7f},
	{0xF84F, 0x02},
	{0xF850, 0x02},
	{0xF851, 0xf8},
	{0xF852, 0xe4},
	{0xF853, 0xe5},
	{0xF854, 0x7d},
	{0xF855, 0xb4},
	{0xF856, 0x30},
	{0xF857, 0x05},
	{0xF858, 0x7f},
	{0xF859, 0x03},
	{0xF85A, 0x02},
	{0xF85B, 0xf8},
	{0xF85C, 0xe4},
	{0xF85D, 0xe5},
	{0xF85E, 0x7d},
	{0xF85F, 0xb4},
	{0xF860, 0x40},
	{0xF861, 0x04},
	{0xF862, 0x7f},
	{0xF863, 0x04},
	{0xF864, 0x80},
	{0xF865, 0x7e},
	{0xF866, 0xe5},
	{0xF867, 0x7d},
	{0xF868, 0xb4},
	{0xF869, 0x50},
	{0xF86A, 0x04},
	{0xF86B, 0x7f},
	{0xF86C, 0x05},
	{0xF86D, 0x80},
	{0xF86E, 0x75},
	{0xF86F, 0xe5},
	{0xF870, 0x7d},
	{0xF871, 0xb4},
	{0xF872, 0x60},
	{0xF873, 0x04},
	{0xF874, 0x7f},
	{0xF875, 0x06},
	{0xF876, 0x80},
	{0xF877, 0x6c},
	{0xF878, 0xe5},
	{0xF879, 0x7d},
	{0xF87A, 0xb4},
	{0xF87B, 0x70},
	{0xF87C, 0x04},
	{0xF87D, 0x7f},
	{0xF87E, 0x07},
	{0xF87F, 0x80},
	{0xF880, 0x63},
	{0xF881, 0xe5},
	{0xF882, 0x7d},
	{0xF883, 0xb4},
	{0xF884, 0x80},
	{0xF885, 0x04},
	{0xF886, 0x7f},
	{0xF887, 0x08},
	{0xF888, 0x80},
	{0xF889, 0x5a},
	{0xF88A, 0xe5},
	{0xF88B, 0x7d},
	{0xF88C, 0xb4},
	{0xF88D, 0x90},
	{0xF88E, 0x04},
	{0xF88F, 0x7f},
	{0xF890, 0x09},
	{0xF891, 0x80},
	{0xF892, 0x51},
	{0xF893, 0xe5},
	{0xF894, 0x7d},
	{0xF895, 0xb4},
	{0xF896, 0xa0},
	{0xF897, 0x04},
	{0xF898, 0x7f},
	{0xF899, 0x0a},
	{0xF89A, 0x80},
	{0xF89B, 0x48},
	{0xF89C, 0xe5},
	{0xF89D, 0x7d},
	{0xF89E, 0xb4},
	{0xF89F, 0xb0},
	{0xF8A0, 0x04},
	{0xF8A1, 0x7f},
	{0xF8A2, 0x0b},
	{0xF8A3, 0x80},
	{0xF8A4, 0x3f},
	{0xF8A5, 0xe5},
	{0xF8A6, 0x7d},
	{0xF8A7, 0xb4},
	{0xF8A8, 0xc0},
	{0xF8A9, 0x04},
	{0xF8AA, 0x7f},
	{0xF8AB, 0x0c},
	{0xF8AC, 0x80},
	{0xF8AD, 0x36},
	{0xF8AE, 0xe5},
	{0xF8AF, 0x7d},
	{0xF8B0, 0xb4},
	{0xF8B1, 0xd0},
	{0xF8B2, 0x04},
	{0xF8B3, 0x7f},
	{0xF8B4, 0x0d},
	{0xF8B5, 0x80},
	{0xF8B6, 0x2d},
	{0xF8B7, 0xe5},
	{0xF8B8, 0x7d},
	{0xF8B9, 0xb4},
	{0xF8BA, 0xe0},
	{0xF8BB, 0x04},
	{0xF8BC, 0x7f},
	{0xF8BD, 0x0e},
	{0xF8BE, 0x80},
	{0xF8BF, 0x24},
	{0xF8C0, 0xe5},
	{0xF8C1, 0x7d},
	{0xF8C2, 0xb4},
	{0xF8C3, 0xe4},
	{0xF8C4, 0x04},
	{0xF8C5, 0x7f},
	{0xF8C6, 0x0f},
	{0xF8C7, 0x80},
	{0xF8C8, 0x1b},
	{0xF8C9, 0xe5},
	{0xF8CA, 0x7d},
	{0xF8CB, 0xb4},
	{0xF8CC, 0xe8},
	{0xF8CD, 0x04},
	{0xF8CE, 0x7f},
	{0xF8CF, 0x10},
	{0xF8D0, 0x80},
	{0xF8D1, 0x12},
	{0xF8D2, 0xe5},
	{0xF8D3, 0x7d},
	{0xF8D4, 0xb4},
	{0xF8D5, 0xec},
	{0xF8D6, 0x04},
	{0xF8D7, 0x7f},
	{0xF8D8, 0x11},
	{0xF8D9, 0x80},
	{0xF8DA, 0x09},
	{0xF8DB, 0xe5},
	{0xF8DC, 0x7d},
	{0xF8DD, 0x7f},
	{0xF8DE, 0x00},
	{0xF8DF, 0xb4},
	{0xF8E0, 0xf0},
	{0xF8E1, 0x02},
	{0xF8E2, 0x7f},
	{0xF8E3, 0x12},
	{0xF8E4, 0x8f},
	{0xF8E5, 0x7c},
	{0xF8E6, 0xef},
	{0xF8E7, 0x24},
	{0xF8E8, 0x95},
	{0xF8E9, 0xff},
	{0xF8EA, 0xe4},
	{0xF8EB, 0x34},
	{0xF8EC, 0xfb},
	{0xF8ED, 0x8f},
	{0xF8EE, 0x82},
	{0xF8EF, 0xf5},
	{0xF8F0, 0x83},
	{0xF8F1, 0xe4},
	{0xF8F2, 0x93},
	{0xF8F3, 0xf5},
	{0xF8F4, 0x7c},
	{0xF8F5, 0xf5},
	{0xF8F6, 0x7b},
	{0xF8F7, 0xe4},
	{0xF8F8, 0xf5},
	{0xF8F9, 0x7a},
	{0xF8FA, 0x75},
	{0xF8FB, 0x78},
	{0xF8FC, 0x30},
	{0xF8FD, 0x75},
	{0xF8FE, 0x79},
	{0xF8FF, 0x53},
	{0xF900, 0x85},
	{0xF901, 0x79},
	{0xF902, 0x82},
	{0xF903, 0x85},
	{0xF904, 0x78},
	{0xF905, 0x83},
	{0xF906, 0xe0},
	{0xF907, 0x25},
	{0xF908, 0x7c},
	{0xF909, 0xf0},
	{0xF90A, 0x74},
	{0xF90B, 0x02},
	{0xF90C, 0x25},
	{0xF90D, 0x79},
	{0xF90E, 0xf5},
	{0xF90F, 0x79},
	{0xF910, 0xe4},
	{0xF911, 0x35},
	{0xF912, 0x78},
	{0xF913, 0xf5},
	{0xF914, 0x78},
	{0xF915, 0x05},
	{0xF916, 0x7a},
	{0xF917, 0xe5},
	{0xF918, 0x7a},
	{0xF919, 0xb4},
	{0xF91A, 0x08},
	{0xF91B, 0xe4},
	{0xF91C, 0x02},
	{0xF91D, 0x18},
	{0xF91E, 0x32},
	{0xF91F, 0x22},
	{0xF920, 0xf0},
	{0xF921, 0x90},
	{0xF922, 0xa0},
	{0xF923, 0xf8},
	{0xF924, 0xe0},
	{0xF925, 0x70},
	{0xF926, 0x02},
	{0xF927, 0xa3},
	{0xF928, 0xe0},
	{0xF929, 0x70},
	{0xF92A, 0x0a},
	{0xF92B, 0x90},
	{0xF92C, 0xa1},
	{0xF92D, 0x10},
	{0xF92E, 0xe0},
	{0xF92F, 0xfe},
	{0xF930, 0xa3},
	{0xF931, 0xe0},
	{0xF932, 0xff},
	{0xF933, 0x80},
	{0xF934, 0x04},
	{0xF935, 0x7e},
	{0xF936, 0x00},
	{0xF937, 0x7f},
	{0xF938, 0x00},
	{0xF939, 0x8e},
	{0xF93A, 0x7e},
	{0xF93B, 0x8f},
	{0xF93C, 0x7f},
	{0xF93D, 0x90},
	{0xF93E, 0x36},
	{0xF93F, 0x0d},
	{0xF940, 0xe0},
	{0xF941, 0x44},
	{0xF942, 0x02},
	{0xF943, 0xf0},
	{0xF944, 0x90},
	{0xF945, 0x36},
	{0xF946, 0x0e},
	{0xF947, 0xe5},
	{0xF948, 0x7e},
	{0xF949, 0xf0},
	{0xF94A, 0xa3},
	{0xF94B, 0xe5},
	{0xF94C, 0x7f},
	{0xF94D, 0xf0},
	{0xF94E, 0xe5},
	{0xF94F, 0x3a},
	{0xF950, 0x60},
	{0xF951, 0x0c},
	{0xF952, 0x90},
	{0xF953, 0x36},
	{0xF954, 0x09},
	{0xF955, 0xe0},
	{0xF956, 0x70},
	{0xF957, 0x06},
	{0xF958, 0x90},
	{0xF959, 0x36},
	{0xF95A, 0x08},
	{0xF95B, 0xf0},
	{0xF95C, 0xf5},
	{0xF95D, 0x3a},
	{0xF95E, 0x02},
	{0xF95F, 0x03},
	{0xF960, 0x94},
	{0xF961, 0x22},
	{0xF962, 0x78},
	{0xF963, 0x07},
	{0xF964, 0xe6},
	{0xF965, 0xd3},
	{0xF966, 0x94},
	{0xF967, 0x00},
	{0xF968, 0x40},
	{0xF969, 0x16},
	{0xF96A, 0x16},
	{0xF96B, 0xe6},
	{0xF96C, 0x90},
	{0xF96D, 0x30},
	{0xF96E, 0xa1},
	{0xF96F, 0xf0},
	{0xF970, 0x90},
	{0xF971, 0x43},
	{0xF972, 0x83},
	{0xF973, 0xe0},
	{0xF974, 0xb4},
	{0xF975, 0x01},
	{0xF976, 0x0f},
	{0xF977, 0x90},
	{0xF978, 0x43},
	{0xF979, 0x87},
	{0xF97A, 0xe0},
	{0xF97B, 0xb4},
	{0xF97C, 0x01},
	{0xF97D, 0x08},
	{0xF97E, 0x80},
	{0xF97F, 0x00},
	{0xF980, 0x90},
	{0xF981, 0x30},
	{0xF982, 0xa0},
	{0xF983, 0x74},
	{0xF984, 0x01},
	{0xF985, 0xf0},
	{0xF986, 0x22},
	{0xF987, 0xf0},
	{0xF988, 0x90},
	{0xF989, 0x35},
	{0xF98A, 0xba},
	{0xF98B, 0xe0},
	{0xF98C, 0xb4},
	{0xF98D, 0x0a},
	{0xF98E, 0x0d},
	{0xF98F, 0xa3},
	{0xF990, 0xe0},
	{0xF991, 0xb4},
	{0xF992, 0x01},
	{0xF993, 0x08},
	{0xF994, 0x90},
	{0xF995, 0xfb},
	{0xF996, 0x94},
	{0xF997, 0xe0},
	{0xF998, 0x90},
	{0xF999, 0x35},
	{0xF99A, 0xb8},
	{0xF99B, 0xf0},
	{0xF99C, 0xd0},
	{0xF99D, 0xd0},
	{0xF99E, 0xd0},
	{0xF99F, 0x82},
	{0xF9A0, 0xd0},
	{0xF9A1, 0x83},
	{0xF9A2, 0xd0},
	{0xF9A3, 0xe0},
	{0xF9A4, 0x32},
	{0xF9A5, 0x22},
	{0xF9A6, 0xe5},
	{0xF9A7, 0x7f},
	{0xF9A8, 0x45},
	{0xF9A9, 0x7e},
	{0xF9AA, 0x60},
	{0xF9AB, 0x15},
	{0xF9AC, 0x90},
	{0xF9AD, 0x01},
	{0xF9AE, 0x00},
	{0xF9AF, 0xe0},
	{0xF9B0, 0x70},
	{0xF9B1, 0x0f},
	{0xF9B2, 0x90},
	{0xF9B3, 0xa0},
	{0xF9B4, 0xf8},
	{0xF9B5, 0xe5},
	{0xF9B6, 0x7e},
	{0xF9B7, 0xf0},
	{0xF9B8, 0xa3},
	{0xF9B9, 0xe5},
	{0xF9BA, 0x7f},
	{0xF9BB, 0xf0},
	{0xF9BC, 0xe4},
	{0xF9BD, 0xf5},
	{0xF9BE, 0x7e},
	{0xF9BF, 0xf5},
	{0xF9C0, 0x7f},
	{0xF9C1, 0x22},
	{0xF9C2, 0x02},
	{0xF9C3, 0x0e},
	{0xF9C4, 0x79},
	{0xF9C5, 0x22},
	/* Offsets:*/
	{0x35C6, 0x00},/* FIDDLEDARKCAL*/
	{0x35C7, 0x00},
	{0x35C8, 0x01},/* STOREDISTANCEATSTOPSTREAMING*/
	{0x35C9, 0x20},
	{0x35CA, 0x01},/*BRUCEFIX*/
	{0x35CB, 0x62},
	{0x35CC, 0x01},/*FIXDATAXFERSTATUSREG*/
	{0x35CD, 0x87},
	{0x35CE, 0x01},/*FOCUSDISTANCEUPDATE*/
	{0x35CF, 0xA6},
	{0x35D0, 0x01},/*SKIPEDOFRESET*/
	{0x35D1, 0xC2},
	{0x35D2, 0x00},
	{0x35D3, 0xFB},
	{0x35D4, 0x00},
	{0x35D5, 0x94},
	{0x35D6, 0x00},
	{0x35D7, 0xFB},
	{0x35D8, 0x00},
	{0x35D9, 0x94},
	{0x35DA, 0x00},
	{0x35DB, 0xFB},
	{0x35DC, 0x00},
	{0x35DD, 0x94},
	{0x35DE, 0x00},
	{0x35DF, 0xFB},
	{0x35E0, 0x00},
	{0x35E1, 0x94},
	{0x35E6, 0x18},/* FIDDLEDARKCAL*/
	{0x35E7, 0x2F},
	{0x35E8, 0x03},/* STOREDISTANCEATSTOPSTREAMING*/
	{0x35E9, 0x93},
	{0x35EA, 0x18},/* BRUCEFIX*/
	{0x35EB, 0x99},
	{0x35EC, 0x00},/* FIXDATAXFERSTATUSREG*/
	{0x35ED, 0xA3},
	{0x35EE, 0x21},/* FOCUSDISTANCEUPDATE*/
	{0x35EF, 0x5B},
	{0x35F0, 0x0E},/* SKIPEDOFRESET*/
	{0x35F1, 0x74},
	{0x35F2, 0x04},
	{0x35F3, 0x64},
	{0x35F4, 0x04},
	{0x35F5, 0x65},
	{0x35F6, 0x04},
	{0x35F7, 0x7B},
	{0x35F8, 0x04},
	{0x35F9, 0x7C},
	{0x35FA, 0x04},
	{0x35FB, 0xDD},
	{0x35FC, 0x04},
	{0x35FD, 0xDE},
	{0x35FE, 0x04},
	{0x35FF, 0xEF},
	{0x3600, 0x04},
	{0x3601, 0xF0},
	/*Jump/Data:*/
	{0x35C2, 0x3F},/* Jump Reg*/
	{0x35C3, 0xFF},/* Jump Reg*/
	{0x35C4, 0x3F},/* Data Reg*/
	{0x35C5, 0xC0},/* Data Reg*/
	{0x35C0, 0x01},/* Enable*/

};
/*=============================================================*/

static int vx6953_i2c_rxdata(unsigned short saddr,
	unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
		{
			.addr  = saddr,
			.flags = 0,
			.len   = 2,
			.buf   = rxdata,
		},
		{
			.addr  = saddr,
			.flags = I2C_M_RD,
			.len   = 2,
			.buf   = rxdata,
		},
	};
	if (i2c_transfer(vx6953_client->adapter, msgs, 2) < 0) {
		CDBG("vx6953_i2c_rxdata failed!\n");
		return -EIO;
	}
	return 0;
}
static int32_t vx6953_i2c_txdata(unsigned short saddr,
				unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = saddr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		 },
	};
	if (i2c_transfer(vx6953_client->adapter, msg, 1) < 0) {
		CDBG("vx6953_i2c_txdata faild 0x%x\n", vx6953_client->addr);
		return -EIO;
	}

	return 0;
}


static int32_t vx6953_i2c_read(unsigned short raddr,
	unsigned short *rdata, int rlen)
{
	int32_t rc = 0;
	unsigned char buf[2];
	if (!rdata)
		return -EIO;
	memset(buf, 0, sizeof(buf));
	buf[0] = (raddr & 0xFF00) >> 8;
	buf[1] = (raddr & 0x00FF);
	rc = vx6953_i2c_rxdata(vx6953_client->addr>>1, buf, rlen);
	if (rc < 0) {
		CDBG("vx6953_i2c_read 0x%x failed!\n", raddr);
		return rc;
	}
	*rdata = (rlen == 2 ? buf[0] << 8 | buf[1] : buf[0]);
	return rc;
}
static int32_t vx6953_i2c_write_b_sensor(unsigned short waddr, uint8_t bdata)
{
	int32_t rc = -EFAULT;
	unsigned char buf[3];
	memset(buf, 0, sizeof(buf));
	buf[0] = (waddr & 0xFF00) >> 8;
	buf[1] = (waddr & 0x00FF);
	buf[2] = bdata;
	CDBG("i2c_write_b addr = 0x%x, val = 0x%x\n", waddr, bdata);
	rc = vx6953_i2c_txdata(vx6953_client->addr>>1, buf, 3);
	if (rc < 0) {
		CDBG("i2c_write_b failed, addr = 0x%x, val = 0x%x!\n",
			waddr, bdata);
	}
	udelay(250);
	return rc;
}
static int32_t vx6953_i2c_write_seq_sensor(unsigned short waddr,
	uint8_t *bdata, uint16_t len)
{
	int32_t rc = -EFAULT;
	unsigned char buf[len+2];
	int i;
	memset(buf, 0, sizeof(buf));
	buf[0] = (waddr & 0xFF00) >> 8;
	buf[1] = (waddr & 0x00FF);
	for (i = 2; i < len+2; i++)
		buf[i] = *bdata++;
	rc = vx6953_i2c_txdata(vx6953_client->addr>>1, buf, len+2);
	if (rc < 0) {
		CDBG("i2c_write_b failed, addr = 0x%x, val = 0x%x!\n",
			 waddr, bdata[0]);
	}
	mdelay(1);
	return rc;
}

static int32_t vx6953_i2c_write_w_table(struct vx6953_i2c_reg_conf const
					 *reg_conf_tbl, int num)
{
	int i;
	int32_t rc = -EIO;
	for (i = 0; i < num; i++) {
		rc = vx6953_i2c_write_b_sensor(reg_conf_tbl->waddr,
			reg_conf_tbl->wdata);
		if (rc < 0)
			break;
		reg_conf_tbl++;
	}
	return rc;
}

static void vx6953_get_pict_fps(uint16_t fps, uint16_t *pfps)
{
	/* input fps is preview fps in Q8 format */
	uint16_t preview_frame_length_lines, snapshot_frame_length_lines;
	uint16_t preview_line_length_pck, snapshot_line_length_pck;
	uint32_t divider, d1, d2;
	/* Total frame_length_lines and line_length_pck for preview */
	preview_frame_length_lines = VX6953_QTR_SIZE_HEIGHT +
		VX6953_VER_QTR_BLK_LINES;
	preview_line_length_pck = VX6953_QTR_SIZE_WIDTH +
		VX6953_HRZ_QTR_BLK_PIXELS;
	/* Total frame_length_lines and line_length_pck for snapshot */
	snapshot_frame_length_lines = VX6953_FULL_SIZE_HEIGHT +
		VX6953_VER_FULL_BLK_LINES;
	snapshot_line_length_pck = VX6953_FULL_SIZE_WIDTH +
		VX6953_HRZ_FULL_BLK_PIXELS;
	d1 = preview_frame_length_lines * 0x00000400/
		snapshot_frame_length_lines;
	d2 = preview_line_length_pck * 0x00000400/
		snapshot_line_length_pck;
	divider = d1 * d2 / 0x400;
	/*Verify PCLK settings and frame sizes.*/
	*pfps = (uint16_t) (fps * divider / 0x400);
	/* 2 is the ratio of no.of snapshot channels
	to number of preview channels */

}

static uint16_t vx6953_get_prev_lines_pf(void)
{
	if (vx6953_ctrl->prev_res == QTR_SIZE)
		return VX6953_QTR_SIZE_HEIGHT + VX6953_VER_QTR_BLK_LINES;
	else
		return VX6953_FULL_SIZE_HEIGHT + VX6953_VER_FULL_BLK_LINES;

}

static uint16_t vx6953_get_prev_pixels_pl(void)
{
	if (vx6953_ctrl->prev_res == QTR_SIZE)
		return VX6953_QTR_SIZE_WIDTH + VX6953_HRZ_QTR_BLK_PIXELS;
	else
		return VX6953_FULL_SIZE_WIDTH + VX6953_HRZ_FULL_BLK_PIXELS;
}

static uint16_t vx6953_get_pict_lines_pf(void)
{
		if (vx6953_ctrl->pict_res == QTR_SIZE)
			return VX6953_QTR_SIZE_HEIGHT +
				VX6953_VER_QTR_BLK_LINES;
		else
			return VX6953_FULL_SIZE_HEIGHT +
				VX6953_VER_FULL_BLK_LINES;
}

static uint16_t vx6953_get_pict_pixels_pl(void)
{
	if (vx6953_ctrl->pict_res == QTR_SIZE)
		return VX6953_QTR_SIZE_WIDTH +
			VX6953_HRZ_QTR_BLK_PIXELS;
	else
		return VX6953_FULL_SIZE_WIDTH +
			VX6953_HRZ_FULL_BLK_PIXELS;
}

static uint32_t vx6953_get_pict_max_exp_lc(void)
{
	if (vx6953_ctrl->pict_res == QTR_SIZE)
		return (VX6953_QTR_SIZE_HEIGHT +
			VX6953_VER_QTR_BLK_LINES)*24;
	else
		return (VX6953_FULL_SIZE_HEIGHT +
			VX6953_VER_FULL_BLK_LINES)*24;
}

static int32_t vx6953_set_fps(struct fps_cfg	*fps)
{
	uint16_t total_lines_per_frame;
	int32_t rc = 0;
	total_lines_per_frame = (uint16_t)((VX6953_QTR_SIZE_HEIGHT +
		VX6953_VER_QTR_BLK_LINES) * vx6953_ctrl->fps_divider/0x400);
	if (vx6953_i2c_write_b_sensor(REG_FRAME_LENGTH_LINES_HI,
		((total_lines_per_frame & 0xFF00) >> 8)) < 0)
		return rc;
	if (vx6953_i2c_write_b_sensor(REG_FRAME_LENGTH_LINES_LO,
		(total_lines_per_frame & 0x00FF)) < 0)
		return rc;
	return rc;
}

static int32_t vx6953_write_exp_gain(uint16_t gain, uint32_t line)
{
	static uint16_t stored_line_length_ratio = 1*Q8;
	static uint16_t stored_gain = 0x0;
	static uint16_t stored_line = 0x0;
	uint16_t line_length_pck, frame_length_lines;
	uint8_t gain_hi, gain_lo;
	uint8_t intg_time_hi, intg_time_lo;
	uint8_t line_length_pck_hi = 0, line_length_pck_lo = 0;
	uint16_t line_length_ratio = 1 * Q8;
	int32_t rc = 0;
	if (gain != stored_gain || line != stored_line) {
		if (vx6953_ctrl->sensormode != SENSOR_SNAPSHOT_MODE) {
			frame_length_lines = VX6953_QTR_SIZE_HEIGHT +
			VX6953_VER_QTR_BLK_LINES;
			line_length_pck = VX6953_QTR_SIZE_WIDTH +
				VX6953_HRZ_QTR_BLK_PIXELS;
			if (line > (frame_length_lines -
				VX6953_STM5M0EDOF_OFFSET)) {
				vx6953_ctrl->fps = (uint16_t) (30 * Q8 *
				(frame_length_lines - VX6953_STM5M0EDOF_OFFSET)/
				line);
			} else {
				vx6953_ctrl->fps = (uint16_t) (30 * Q8);
			}
		} else {
			frame_length_lines = VX6953_FULL_SIZE_HEIGHT +
					VX6953_VER_FULL_BLK_LINES;
			line_length_pck = VX6953_FULL_SIZE_WIDTH +
					VX6953_HRZ_FULL_BLK_PIXELS;
		}
		/* calculate line_length_ratio */
		if ((frame_length_lines - VX6953_STM5M0EDOF_OFFSET) < line) {
			line_length_ratio = (line*Q8) /
				(frame_length_lines - VX6953_STM5M0EDOF_OFFSET);
			line = frame_length_lines - VX6953_STM5M0EDOF_OFFSET;
		} else {
			line_length_ratio = 1*Q8;
		}
		vx6953_i2c_write_b_sensor(REG_GROUPED_PARAMETER_HOLD,
			GROUPED_PARAMETER_HOLD);
		if (line_length_ratio != stored_line_length_ratio) {
			line_length_pck = (line_length_pck >
				MAX_LINE_LENGTH_PCK) ?
				MAX_LINE_LENGTH_PCK : line_length_pck;
			line_length_pck = (uint16_t) (line_length_pck *
				line_length_ratio/Q8);
			line_length_pck_hi = (uint8_t) ((line_length_pck &
				0xFF00) >> 8);
			line_length_pck_lo = (uint8_t) (line_length_pck &
				0x00FF);
			vx6953_i2c_write_b_sensor(REG_LINE_LENGTH_PCK_HI,
				line_length_pck_hi);
			vx6953_i2c_write_b_sensor(REG_LINE_LENGTH_PCK_LO,
				line_length_pck_lo);
			stored_line_length_ratio = line_length_ratio;
		}
		/* update analogue gain registers */
		gain_hi = (uint8_t) ((gain & 0xFF00) >> 8);
		gain_lo = (uint8_t) (gain & 0x00FF);
		vx6953_i2c_write_b_sensor(REG_ANALOGUE_GAIN_CODE_GLOBAL_LO,
			gain_lo);
		vx6953_i2c_write_b_sensor(REG_DIGITAL_GAIN_GREEN_R_LO, gain_hi);
		vx6953_i2c_write_b_sensor(REG_DIGITAL_GAIN_RED_LO, gain_hi);
		vx6953_i2c_write_b_sensor(REG_DIGITAL_GAIN_BLUE_LO, gain_hi);
		vx6953_i2c_write_b_sensor(REG_DIGITAL_GAIN_GREEN_B_LO, gain_hi);
		CDBG("%s, gain_hi 0x%x, gain_lo 0x%x\n", __func__,
			gain_hi, gain_lo);
		/* update line count registers */
		intg_time_hi = (uint8_t) (((uint16_t)line & 0xFF00) >> 8);
		intg_time_lo = (uint8_t) ((uint16_t)line & 0x00FF);
		vx6953_i2c_write_b_sensor(REG_COARSE_INTEGRATION_TIME_HI,
			intg_time_hi);
		vx6953_i2c_write_b_sensor(REG_COARSE_INTEGRATION_TIME_LO,
			intg_time_lo);
		vx6953_i2c_write_b_sensor(REG_GROUPED_PARAMETER_HOLD,
			GROUPED_PARAMETER_HOLD_OFF);
		stored_gain = gain;
		stored_line = line;
	}
	return rc;
}

static int32_t vx6953_set_pict_exp_gain(uint16_t gain, uint32_t line)
{
	int32_t rc = 0;
	rc = vx6953_write_exp_gain(gain, line);
	return rc;
} /* endof vx6953_set_pict_exp_gain*/

static int32_t vx6953_move_focus(int direction,
	int32_t num_steps)
{
	return 0;
}


static int32_t vx6953_set_default_focus(uint8_t af_step)
{
	return 0;
}

static int32_t vx6953_test(enum vx6953_test_mode_t mo)
{
	int32_t rc = 0;
	if (mo == TEST_OFF)
		return rc;
	else {
		/* REG_0x30D8[4] is TESBYPEN: 0: Normal Operation,
		1: Bypass Signal Processing
		REG_0x30D8[5] is EBDMASK: 0:
		Output Embedded data, 1: No output embedded data */
		if (vx6953_i2c_write_b_sensor(REG_TEST_PATTERN_MODE,
			(uint8_t) mo) < 0) {
			return rc;
		}
	}
	return rc;
}

static int vx6953_enable_edof(enum edof_mode_t edof_mode)
{
	int rc = 0;
	if (edof_mode == VX6953_EDOF_ESTIMATION) {
		/* EDof Estimation mode for preview */
		if (vx6953_i2c_write_b_sensor(REG_0x0b80, 0x02) < 0)
			return rc;
		CDBG("VX6953_EDOF_ESTIMATION");
	} else if (edof_mode == VX6953_EDOF_APPLICATION) {
		/* EDof Application mode for Capture */
		if (vx6953_i2c_write_b_sensor(REG_0x0b80, 0x01) < 0)
			return rc;
		CDBG("VX6953_EDOF_APPLICATION");
	} else {
		/* EDOF disabled */
		if (vx6953_i2c_write_b_sensor(REG_0x0b80, 0x00) < 0)
			return rc;
		CDBG("VX6953_EDOF_DISABLE");
	}
	return rc;
}

static int32_t vx6953_patch_for_cut2(void)
{
	int32_t rc = 0;
	rc = vx6953_i2c_write_w_table(patch_tbl,
		ARRAY_SIZE(patch_tbl));
	if (rc < 0)
		return rc;

	return rc;
}
static int32_t vx6953_sensor_setting(int update_type, int rt)
{

	int32_t rc = 0;
	unsigned short frame_cnt;
	switch (update_type) {
	case REG_INIT:
		if (rt == RES_PREVIEW || rt == RES_CAPTURE) {
			struct vx6953_i2c_reg_conf init_tbl[] = {
				{REG_0x0112,
					vx6953_regs.reg_pat_init[0].reg_0x0112},
				{REG_0x0113,
					vx6953_regs.reg_pat_init[0].reg_0x0113},
				{REG_VT_PIX_CLK_DIV,
					vx6953_regs.reg_pat_init[0].
					vt_pix_clk_div},
				{REG_PRE_PLL_CLK_DIV,
					vx6953_regs.reg_pat_init[0].
					pre_pll_clk_div},
				{REG_PLL_MULTIPLIER,
					vx6953_regs.reg_pat_init[0].
					pll_multiplier},
				{REG_OP_PIX_CLK_DIV,
					vx6953_regs.reg_pat_init[0].
					op_pix_clk_div},
				{REG_COARSE_INTEGRATION_TIME_HI,
					vx6953_regs.reg_pat[rt].
					coarse_integration_time_hi},
				{REG_COARSE_INTEGRATION_TIME_LO,
					vx6953_regs.reg_pat[rt].
					coarse_integration_time_lo},
				{REG_ANALOGUE_GAIN_CODE_GLOBAL_LO,
					vx6953_regs.reg_pat[rt].
					analogue_gain_code_global},
				{REG_0x3030,
					vx6953_regs.reg_pat_init[0].reg_0x3030},
				/* 953 specific registers */
				{REG_0x0111,
					vx6953_regs.reg_pat_init[0].reg_0x0111},
				{REG_0x0b00,
					vx6953_regs.reg_pat_init[0].reg_0x0b00},
				{REG_0x3001,
					vx6953_regs.reg_pat_init[0].reg_0x3001},
				{REG_0x3004,
					vx6953_regs.reg_pat_init[0].reg_0x3004},
				{REG_0x3007,
					vx6953_regs.reg_pat_init[0].reg_0x3007},
				{REG_0x3016,
					vx6953_regs.reg_pat_init[0].reg_0x3016},
				{REG_0x301d,
					vx6953_regs.reg_pat_init[0].reg_0x301d},
				{REG_0x317e,
					vx6953_regs.reg_pat_init[0].reg_0x317e},
				{REG_0x317f,
					vx6953_regs.reg_pat_init[0].reg_0x317f},
				{REG_0x3400,
					vx6953_regs.reg_pat_init[0].reg_0x3400},
				/* DEFCOR settings */
				/*Single Defect Correction Weight DISABLE*/
				{0x0b06,
					vx6953_regs.reg_pat_init[0].reg_0x0b06},
				/*Single_defect_correct_weight = auto*/
				{0x0b07,
					vx6953_regs.reg_pat_init[0].reg_0x0b07},
				/*Dynamic couplet correction ENABLED*/
				{0x0b08,
					vx6953_regs.reg_pat_init[0].reg_0x0b08},
				/*Dynamic couplet correction weight*/
				{0x0b09,
					vx6953_regs.reg_pat_init[0].reg_0x0b09},
				/* Clock Setup */
				/* Tell sensor ext clk is 24MHz*/
				{0x0136,
					vx6953_regs.reg_pat_init[0].reg_0x0136},
				{0x0137,
					vx6953_regs.reg_pat_init[0].reg_0x0137},
				/* The white balance gains must be written
				to the sensor every frame. */
				/* Edof */
				{REG_0x0b83,
					vx6953_regs.reg_pat_init[0].reg_0x0b83},
				{REG_0x0b84,
					vx6953_regs.reg_pat_init[0].reg_0x0b84},
				{0x0b85,
					vx6953_regs.reg_pat_init[0].reg_0x0b85},
				{0x0b88,
					vx6953_regs.reg_pat_init[0].reg_0x0b88},
				{0x0b89,
					vx6953_regs.reg_pat_init[0].reg_0x0b89},
				{REG_0x0b8a,
					vx6953_regs.reg_pat_init[0].reg_0x0b8a},
				/* Mode specific regieters */
				{REG_FRAME_LENGTH_LINES_HI,
					vx6953_regs.reg_pat[rt].
					frame_length_lines_hi},
				{REG_FRAME_LENGTH_LINES_LO,
					vx6953_regs.reg_pat[rt].
					frame_length_lines_lo},
				{REG_LINE_LENGTH_PCK_HI,
					vx6953_regs.reg_pat[rt].
					line_length_pck_hi},
				{REG_LINE_LENGTH_PCK_LO,
					vx6953_regs.reg_pat[rt].
					line_length_pck_lo},
				{REG_0x3005,
					vx6953_regs.reg_pat[rt].reg_0x3005},
				{0x3010,
					vx6953_regs.reg_pat[rt].reg_0x3010},
				{REG_0x3011,
					vx6953_regs.reg_pat[rt].reg_0x3011},
				{REG_0x301a,
					vx6953_regs.reg_pat[rt].reg_0x301a},
				{REG_0x3035,
					vx6953_regs.reg_pat[rt].reg_0x3035},
				{REG_0x3036,
					vx6953_regs.reg_pat[rt].reg_0x3036},
				{REG_0x3041,
					vx6953_regs.reg_pat[rt].reg_0x3041},
				{0x3042,
					vx6953_regs.reg_pat[rt].reg_0x3042},
				{REG_0x3045,
					vx6953_regs.reg_pat[rt].reg_0x3045},
				/*EDOF: Estimation settings for Preview mode
				Application settings for capture mode
				(standard settings - Not tuned) */
				{REG_0x0b80,
					vx6953_regs.reg_pat[rt].reg_0x0b80},
				{REG_0x0900,
					vx6953_regs.reg_pat[rt].reg_0x0900},
				{REG_0x0901,
					vx6953_regs.reg_pat[rt].reg_0x0901},
				{REG_0x0902,
					vx6953_regs.reg_pat[rt].reg_0x0902},
				{REG_0x0383,
					vx6953_regs.reg_pat[rt].reg_0x0383},
				{REG_0x0387,
					vx6953_regs.reg_pat[rt].reg_0x0387},
				/* Change output size / frame rate */
				{REG_0x034c,
					vx6953_regs.reg_pat[rt].reg_0x034c},
				{REG_0x034d,
					vx6953_regs.reg_pat[rt].reg_0x034d},
				{REG_0x034e,
					vx6953_regs.reg_pat[rt].reg_0x034e},
				{REG_0x034f,
					vx6953_regs.reg_pat[rt].reg_0x034f},
				{REG_0x1716,
					vx6953_regs.reg_pat[rt].reg_0x1716},
				{REG_0x1717,
					vx6953_regs.reg_pat[rt].reg_0x1717},
				{REG_0x1718,
					vx6953_regs.reg_pat[rt].reg_0x1718},
				{REG_0x1719,
					vx6953_regs.reg_pat[rt].reg_0x1719},
			};
			/* reset fps_divider */
			vx6953_ctrl->fps = 30 * Q8;
			/* stop streaming */
			if (vx6953_i2c_write_b_sensor(REG_MODE_SELECT,
				MODE_SELECT_STANDBY_MODE) < 0)
				return rc;
				/*vx6953_stm5m0edof_delay_msecs_stdby*/
			mdelay(vx6953_stm5m0edof_delay_msecs_stdby);
			vx6953_patch_for_cut2();
			rc = vx6953_i2c_write_w_table(&init_tbl[0],
				ARRAY_SIZE(init_tbl));
			if (rc < 0)
				return rc;
			/* Start sensor streaming */
			if (vx6953_i2c_write_b_sensor(REG_MODE_SELECT,
				MODE_SELECT_STREAM) < 0)
				return rc;
			mdelay(vx6953_stm5m0edof_delay_msecs_stream);
			if (vx6953_i2c_read(0x0005, &frame_cnt, 1) < 0)
				return rc;
			while (frame_cnt == 0xFF) {
				if (vx6953_i2c_read(0x0005, &frame_cnt, 1) < 0)
					return rc;
				CDBG("frame_cnt=%d", frame_cnt);
			}
		}
		return rc;
	case UPDATE_PERIODIC:
		if (rt == RES_PREVIEW || rt == RES_CAPTURE) {
			struct vx6953_i2c_reg_conf mode_tbl[] = {
			{REG_VT_PIX_CLK_DIV,
				vx6953_regs.reg_pat_init[0].vt_pix_clk_div},
			{REG_PRE_PLL_CLK_DIV,
				vx6953_regs.reg_pat_init[0].pre_pll_clk_div},
			{REG_PLL_MULTIPLIER,
				vx6953_regs.reg_pat_init[0].pll_multiplier},
			{REG_OP_PIX_CLK_DIV,
				vx6953_regs.reg_pat_init[0].op_pix_clk_div},
			/* Mode specific regieters */
			{REG_FRAME_LENGTH_LINES_HI,
				vx6953_regs.reg_pat[rt].frame_length_lines_hi},
			{REG_FRAME_LENGTH_LINES_LO,
				vx6953_regs.reg_pat[rt].frame_length_lines_lo},
			{REG_LINE_LENGTH_PCK_HI,
				vx6953_regs.reg_pat[rt].line_length_pck_hi},
			{REG_LINE_LENGTH_PCK_LO,
				vx6953_regs.reg_pat[rt].line_length_pck_lo},
			{REG_0x3005, vx6953_regs.reg_pat[rt].reg_0x3005},
			{0x3010, vx6953_regs.reg_pat[rt].reg_0x3010},
			{REG_0x3011, vx6953_regs.reg_pat[rt].reg_0x3011},
			{REG_0x301a, vx6953_regs.reg_pat[rt].reg_0x301a},
			{REG_0x3035, vx6953_regs.reg_pat[rt].reg_0x3035},
			{REG_0x3036, vx6953_regs.reg_pat[rt].reg_0x3036},
			{REG_0x3041, vx6953_regs.reg_pat[rt].reg_0x3041},
			{0x3042, vx6953_regs.reg_pat[rt].reg_0x3042},
			{REG_0x3045, vx6953_regs.reg_pat[rt].reg_0x3045},
			/*EDOF: Estimation settings for Preview mode
			Application settings for capture
			mode(standard settings - Not tuned) */
			{REG_0x0b80, vx6953_regs.reg_pat[rt].reg_0x0b80},
			{REG_0x0900, vx6953_regs.reg_pat[rt].reg_0x0900},
			{REG_0x0901, vx6953_regs.reg_pat[rt].reg_0x0901},
			{REG_0x0902, vx6953_regs.reg_pat[rt].reg_0x0902},
			{REG_0x0383, vx6953_regs.reg_pat[rt].reg_0x0383},
			{REG_0x0387, vx6953_regs.reg_pat[rt].reg_0x0387},
			/* Change output size / frame rate */
			{REG_0x034c, vx6953_regs.reg_pat[rt].reg_0x034c},
			{REG_0x034d, vx6953_regs.reg_pat[rt].reg_0x034d},
			{REG_0x034e, vx6953_regs.reg_pat[rt].reg_0x034e},
			{REG_0x034f, vx6953_regs.reg_pat[rt].reg_0x034f},
			/*{0x200, vx6953_regs.reg_pat[rt].reg_0x0200},
			{0x201, vx6953_regs.reg_pat[rt].reg_0x0201},*/
			{REG_0x1716, vx6953_regs.reg_pat[rt].reg_0x1716},
			{REG_0x1717, vx6953_regs.reg_pat[rt].reg_0x1717},
			{REG_0x1718, vx6953_regs.reg_pat[rt].reg_0x1718},
			{REG_0x1719, vx6953_regs.reg_pat[rt].reg_0x1719},
		};
			/* stop streaming */
		mdelay(5);
		if (vx6953_i2c_write_b_sensor(REG_MODE_SELECT,
			MODE_SELECT_STANDBY_MODE) < 0)
			return rc;
		/*vx6953_stm5m0edof_delay_msecs_stdby*/
		mdelay(vx6953_stm5m0edof_delay_msecs_stdby);
		rc = vx6953_i2c_write_w_table(&mode_tbl[0],
			ARRAY_SIZE(mode_tbl));
		if (rc < 0)
			return rc;
		/* Start sensor streaming */
		if (vx6953_i2c_write_b_sensor(REG_MODE_SELECT,
			MODE_SELECT_STREAM) < 0)
			return rc;
		mdelay(vx6953_stm5m0edof_delay_msecs_stream);
		if (vx6953_i2c_read(0x0005, &frame_cnt, 1) < 0)
			return rc;
		while (frame_cnt == 0xFF) {
			if (vx6953_i2c_read(0x0005, &frame_cnt, 1) < 0)
				return rc;
				CDBG("frame_cnt=%d", frame_cnt);
			}
		}
		return rc;
	default:
		return rc;
	}

	return rc;
}


static int32_t vx6953_video_config(int mode)
{

	int32_t	rc = 0;
	int	rt;
	/* change sensor resolution	if needed */
	if (vx6953_ctrl->curr_res != vx6953_ctrl->prev_res)	{
		if (vx6953_ctrl->prev_res == QTR_SIZE) {
			rt = RES_PREVIEW;
			vx6953_stm5m0edof_delay_msecs_stdby	=
				((2 * 1000 * Q8 * vx6953_ctrl->fps_divider) /
				vx6953_ctrl->fps) + 1;
		} else {
			rt = RES_CAPTURE;
			vx6953_stm5m0edof_delay_msecs_stdby	=
				((1000 * Q8 * vx6953_ctrl->fps_divider) /
				vx6953_ctrl->fps) + 1;
		}
		if (vx6953_sensor_setting(UPDATE_PERIODIC, rt) < 0)
			return rc;
	}
	if (vx6953_ctrl->set_test) {
		if (vx6953_test(vx6953_ctrl->set_test) < 0)
			return	rc;
	}
	vx6953_ctrl->edof_mode = VX6953_EDOF_ESTIMATION;
	rc = vx6953_enable_edof(vx6953_ctrl->edof_mode);
	if (rc < 0)
		return rc;
	vx6953_ctrl->curr_res = vx6953_ctrl->prev_res;
	vx6953_ctrl->sensormode = mode;
	return rc;
}

static int32_t vx6953_snapshot_config(int mode)
{
	int32_t rc = 0;
	int rt;
	/*change sensor resolution if needed */
	if (vx6953_ctrl->curr_res != vx6953_ctrl->pict_res) {
		if (vx6953_ctrl->pict_res == QTR_SIZE) {
			rt = RES_PREVIEW;
			vx6953_stm5m0edof_delay_msecs_stdby =
				((2 * 1000 * Q8 * vx6953_ctrl->fps_divider)/
				vx6953_ctrl->fps) + 1;
		} else {
			rt = RES_CAPTURE;
			vx6953_stm5m0edof_delay_msecs_stdby =
				((1000 * Q8 * vx6953_ctrl->fps_divider)/
				vx6953_ctrl->fps) + 1;
		}
	if (vx6953_sensor_setting(UPDATE_PERIODIC, rt) < 0)
		return rc;
}

	vx6953_ctrl->edof_mode = VX6953_EDOF_APPLICATION;
	if (vx6953_enable_edof(vx6953_ctrl->edof_mode) < 0)
		return rc;
	vx6953_ctrl->curr_res = vx6953_ctrl->pict_res;
	vx6953_ctrl->sensormode = mode;
	return rc;
} /*end of vx6953_snapshot_config*/

static int32_t vx6953_raw_snapshot_config(int mode)
{
	int32_t rc = 0;
	int rt;
	/* change sensor resolution if needed */
	if (vx6953_ctrl->curr_res != vx6953_ctrl->pict_res) {
		if (vx6953_ctrl->pict_res == QTR_SIZE) {
			rt = RES_PREVIEW;
			vx6953_stm5m0edof_delay_msecs_stdby =
				((2 * 1000 * Q8 *
				vx6953_ctrl->fps_divider) /
				vx6953_ctrl->fps) + 1;
		} else {
			rt = RES_CAPTURE;
			vx6953_stm5m0edof_delay_msecs_stdby =
				((1000 * Q8 * vx6953_ctrl->fps_divider)/
				vx6953_ctrl->fps) + 1;
		}
		if (vx6953_sensor_setting(UPDATE_PERIODIC, rt) < 0)
			return rc;
	}
	vx6953_ctrl->edof_mode = VX6953_EDOF_APPLICATION;
	if (vx6953_enable_edof(vx6953_ctrl->edof_mode) < 0)
		return rc;
	vx6953_ctrl->curr_res = vx6953_ctrl->pict_res;
	vx6953_ctrl->sensormode = mode;
	return rc;
} /*end of vx6953_raw_snapshot_config*/
static int32_t vx6953_set_sensor_mode(int mode,
	int res)
{
	int32_t rc = 0;
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		rc = vx6953_video_config(mode);
		break;
	case SENSOR_SNAPSHOT_MODE:
		rc = vx6953_snapshot_config(mode);
		break;
	case SENSOR_RAW_SNAPSHOT_MODE:
		rc = vx6953_raw_snapshot_config(mode);
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}
static int32_t vx6953_power_down(void)
{
	return 0;
}


static int vx6953_probe_init_done(const struct msm_camera_sensor_info *data)
{
	gpio_direction_output(data->sensor_reset, 0);
	gpio_free(data->sensor_reset);
	return 0;
}
static int vx6953_probe_init_sensor(const struct msm_camera_sensor_info *data)
{
	int32_t rc = 0;
	unsigned short chipidl, chipidh;
	CDBG("%s: %d\n", __func__, __LINE__);
	rc = gpio_request(data->sensor_reset, "vx6953");
	CDBG(" vx6953_probe_init_sensor \n");
	if (!rc) {
		CDBG("sensor_reset = %d\n", rc);
		CDBG(" vx6953_probe_init_sensor 1\n");
		gpio_direction_output(data->sensor_reset, 0);
		mdelay(50);
		CDBG(" vx6953_probe_init_sensor 1\n");
		gpio_direction_output(data->sensor_reset, 1);
		mdelay(13);
	} else {
		CDBG(" vx6953_probe_init_sensor 2\n");
		goto init_probe_done;
	}
	mdelay(20);
	CDBG(" vx6953_probe_init_sensor is called \n");
	/* 3. Read sensor Model ID: */
	rc = vx6953_i2c_read(0x0000, &chipidh, 1);
	if (rc < 0) {
		CDBG(" vx6953_probe_init_sensor 3 \n");
		goto init_probe_fail;
	}
	rc = vx6953_i2c_read(0x0001, &chipidl, 1);
	if (rc < 0) {
		CDBG(" vx6953_probe_init_sensor4 \n");
		goto init_probe_fail;
	}
	CDBG("vx6953 model_id = 0x%x  0x%x\n", chipidh, chipidl);
	/* 4. Compare sensor ID to VX6953 ID: */
	if (chipidh != 0x03 || chipidl != 0xB9) {
		rc = -ENODEV;
		CDBG("vx6953_probe_init_sensor fail chip id doesnot match\n");
		goto init_probe_fail;
	}
	goto init_probe_done;
init_probe_fail:
	CDBG(" vx6953_probe_init_sensor fails\n");
	vx6953_probe_init_done(data);
init_probe_done:
	CDBG(" vx6953_probe_init_sensor finishes\n");
	return rc;
	}
/* camsensor_iu060f_vx6953_reset */
int vx6953_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	unsigned short revision_number;
	int32_t rc = 0;
	struct msm_camera_csi_params *vx6953_csi_params =
		kzalloc(sizeof(struct msm_camera_csi_params), GFP_KERNEL);
	CDBG("%s: %d\n", __func__, __LINE__);
	CDBG("Calling vx6953_sensor_open_init\n");
	vx6953_ctrl = kzalloc(sizeof(struct vx6953_ctrl_t), GFP_KERNEL);
	if (!vx6953_ctrl) {
		CDBG("vx6953_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}
	vx6953_ctrl->fps_divider = 1 * 0x00000100;
	vx6953_ctrl->pict_fps_divider = 1 * 0x00000100;
	vx6953_ctrl->set_test = TEST_OFF;
	vx6953_ctrl->prev_res = QTR_SIZE;
	vx6953_ctrl->pict_res = FULL_SIZE;
	vx6953_ctrl->curr_res = INVALID_SIZE;
	vx6953_ctrl->sensor_type = VX6953_STM5M0EDOF_CUT_2;
	vx6953_ctrl->edof_mode = VX6953_EDOF_ESTIMATION;
	if (data)
		vx6953_ctrl->sensordata = data;
	if (rc < 0) {
		CDBG("Calling vx6953_sensor_open_init fail1\n");
		return rc;
	}
	CDBG("%s: %d\n", __func__, __LINE__);
	/* enable mclk first */
	msm_camio_clk_rate_set(VX6953_STM5M0EDOF_DEFAULT_MASTER_CLK_RATE);
	mdelay(20);
	CDBG("%s: %d\n", __func__, __LINE__);
	rc = vx6953_probe_init_sensor(data);
	if (rc < 0) {
		CDBG("Calling vx6953_sensor_open_init fail3\n");
		goto init_fail;
	}
	if (rc < 0) {
		CDBG("Calling vx6953_sensor_open_init fail5\n");
		return rc;
	}
	if (vx6953_i2c_read(0x0002, &revision_number, 1) < 0)
		return rc;
		CDBG("sensor revision number major = 0x%x\n", revision_number);
	if (vx6953_i2c_read(0x0018, &revision_number, 1) < 0)
		return rc;
		CDBG("sensor revision number = 0x%x\n", revision_number);
	if (revision_number == VX6953_REVISION_NUMBER) {
		vx6953_ctrl->sensor_type = VX6953_STM5M0EDOF_CUT_2;
		CDBG("VX6953 EDof Cut 2.0 sensor\n ");
	} else {/* Cut1.0 reads 0x00 for register 0x0018*/
		vx6953_ctrl->sensor_type = VX6953_STM5M0EDOF_CUT_1;
		CDBG("VX6953 EDof Cut 1.0 sensor \n");
	}
	/* config mipi csi controller */
	CDBG("vx6953_sensor_open_init: config csi controller \n");
	vx6953_csi_params->data_format = CSI_8BIT;
	vx6953_csi_params->lane_cnt = 1;
	vx6953_csi_params->lane_assign = 0xe4;
	vx6953_csi_params->dpcm_scheme = 0;
	vx6953_csi_params->settle_cnt = 7;
	rc = msm_camio_csi_config(vx6953_csi_params);
	if (rc < 0)
		CDBG(" config csi controller failed \n");
	if (vx6953_ctrl->prev_res == QTR_SIZE) {
		if (vx6953_sensor_setting(REG_INIT, RES_PREVIEW) < 0)
			return rc;
	} else {
		if (vx6953_sensor_setting(REG_INIT, RES_CAPTURE) < 0)
			return rc;
	}

	vx6953_ctrl->fps = 30*Q8;
	if (rc < 0)
		goto init_fail;
	else
		goto init_done;
init_fail:
	CDBG(" init_fail \n");
		vx6953_probe_init_done(data);
		kfree(vx6953_ctrl);
init_done:
	CDBG("init_done \n");
	return rc;
} /*endof vx6953_sensor_open_init*/

static int vx6953_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&vx6953_wait_queue);
	return 0;
}

static const struct i2c_device_id vx6953_i2c_id[] = {
	{"vx6953", 0},
	{ }
};

static int vx6953_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	CDBG("vx6953_probe called!\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		CDBG("i2c_check_functionality failed\n");
		goto probe_failure;
	}

	vx6953_sensorw = kzalloc(sizeof(struct vx6953_work_t), GFP_KERNEL);
	if (!vx6953_sensorw) {
		CDBG("kzalloc failed.\n");
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, vx6953_sensorw);
	vx6953_init_client(client);
	vx6953_client = client;

	mdelay(50);

	CDBG("vx6953_probe successed! rc = %d\n", rc);
	return 0;

probe_failure:
	CDBG("vx6953_probe failed! rc = %d\n", rc);
	return rc;
}

static int vx6953_send_wb_info(struct wb_info_cfg *wb)
{
	unsigned short read_data;
	uint8_t temp[8];
	int rc = 0;
	int i = 0;

	/* red_gain */
	temp[2] = wb->red_gain >> 8;
	temp[3] = wb->red_gain & 0xFF;

	/* green_gain */
	temp[0] = wb->green_gain >> 8;
	temp[1] = wb->green_gain & 0xFF;
	temp[6] = temp[0];
	temp[7] = temp[1];

	/* blue_gain */
	temp[4] = wb->blue_gain >> 8;
	temp[5] = wb->blue_gain & 0xFF;
	rc = vx6953_i2c_write_seq_sensor(0x0B8E, &temp[0], 8);

	for (i = 0; i < 6; i++) {
		rc = vx6953_i2c_read(0x0B8E + i, &read_data, 1);
		CDBG("%s addr 0x%x val %d \n", __func__, 0x0B8E + i, read_data);
	}
	rc = vx6953_i2c_read(0x0B82, &read_data, 1);
	CDBG("%s addr 0x%x val %d \n", __func__, 0x0B82, read_data);
	if (rc < 0)
		return rc;
	return rc;
} /*end of vx6953_snapshot_config*/

static int __exit vx6953_remove(struct i2c_client *client)
{
	struct vx6953_work_t_t *sensorw = i2c_get_clientdata(client);
	free_irq(client->irq, sensorw);
	vx6953_client = NULL;
	kfree(sensorw);
	return 0;
}

static struct i2c_driver vx6953_i2c_driver = {
	.id_table = vx6953_i2c_id,
	.probe  = vx6953_i2c_probe,
	.remove = __exit_p(vx6953_i2c_remove),
	.driver = {
		.name = "vx6953",
	},
};

int vx6953_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cdata;
	long   rc = 0;
	if (copy_from_user(&cdata,
		(void *)argp,
		sizeof(struct sensor_cfg_data)))
		return -EFAULT;
	mutex_lock(&vx6953_mut);
	CDBG("vx6953_sensor_config: cfgtype = %d\n",
	cdata.cfgtype);
		switch (cdata.cfgtype) {
		case CFG_GET_PICT_FPS:
			vx6953_get_pict_fps(
				cdata.cfg.gfps.prevfps,
				&(cdata.cfg.gfps.pictfps));

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_GET_PREV_L_PF:
			cdata.cfg.prevl_pf =
			vx6953_get_prev_lines_pf();

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_GET_PREV_P_PL:
			cdata.cfg.prevp_pl =
				vx6953_get_prev_pixels_pl();

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_GET_PICT_L_PF:
			cdata.cfg.pictl_pf =
				vx6953_get_pict_lines_pf();

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_GET_PICT_P_PL:
			cdata.cfg.pictp_pl =
				vx6953_get_pict_pixels_pl();

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_GET_PICT_MAX_EXP_LC:
			cdata.cfg.pict_max_exp_lc =
				vx6953_get_pict_max_exp_lc();

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_SET_FPS:
		case CFG_SET_PICT_FPS:
			rc = vx6953_set_fps(&(cdata.cfg.fps));
			break;

		case CFG_SET_EXP_GAIN:
			rc =
				vx6953_write_exp_gain(
					cdata.cfg.exp_gain.gain,
					cdata.cfg.exp_gain.line);
			break;

		case CFG_SET_PICT_EXP_GAIN:
			rc =
				vx6953_set_pict_exp_gain(
				cdata.cfg.exp_gain.gain,
				cdata.cfg.exp_gain.line);
			break;

		case CFG_SET_MODE:
			rc = vx6953_set_sensor_mode(cdata.mode,
					cdata.rs);
			break;

		case CFG_PWR_DOWN:
			rc = vx6953_power_down();
			break;

		case CFG_MOVE_FOCUS:
			rc =
				vx6953_move_focus(
				cdata.cfg.focus.dir,
				cdata.cfg.focus.steps);
			break;

		case CFG_SET_DEFAULT_FOCUS:
			rc =
				vx6953_set_default_focus(
				cdata.cfg.focus.steps);
			break;

		case CFG_SET_EFFECT:
			rc = vx6953_set_default_focus(
				cdata.cfg.effect);
			break;


		case CFG_SEND_WB_INFO:
			rc = vx6953_send_wb_info(
				&(cdata.cfg.wb_info));
			break;

		default:
			rc = -EFAULT;
			break;
		}

	mutex_unlock(&vx6953_mut);

	return rc;
}




static int vx6953_sensor_release(void)
{
	int rc = -EBADF;
	mutex_lock(&vx6953_mut);
	vx6953_power_down();
	gpio_direction_output(vx6953_ctrl->sensordata->sensor_reset,
		0);
	gpio_free(vx6953_ctrl->sensordata->sensor_reset);
	kfree(vx6953_ctrl);
	vx6953_ctrl = NULL;
	CDBG("vx6953_release completed\n");
	mutex_unlock(&vx6953_mut);

	return rc;
}

static int vx6953_sensor_probe(const struct msm_camera_sensor_info *info,
		struct msm_sensor_ctrl *s)
{
	int rc = 0;
	rc = i2c_add_driver(&vx6953_i2c_driver);
	if (rc < 0 || vx6953_client == NULL) {
		rc = -ENOTSUPP;
		goto probe_fail;
	}
	msm_camio_clk_rate_set(24000000);
	rc = vx6953_probe_init_sensor(info);
	if (rc < 0)
		goto probe_fail;
	s->s_init = vx6953_sensor_open_init;
	s->s_release = vx6953_sensor_release;
	s->s_config  = vx6953_sensor_config;
	vx6953_probe_init_done(info);

	return rc;

probe_fail:
	CDBG("vx6953_sensor_probe: SENSOR PROBE FAILS!\n");
	return rc;
}

static int __vx6953_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, vx6953_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __vx6953_probe,
	.driver = {
		.name = "msm_camera_vx6953",
		.owner = THIS_MODULE,
	},
};

static int __init vx6953_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(vx6953_init);
void vx6953_exit(void)
{
	i2c_del_driver(&vx6953_i2c_driver);
}


