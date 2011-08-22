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

#include "vx6953.h"
const struct reg_struct_init vx6953_reg_init[1] = {
	{
		10,			/*REG = 0x0112 , 10 bit */
		10,			/*REG = 0x0113*/
		9,			/*REG = 0x0301 vt_pix_clk_div*/
		4,		/*REG = 0x0305 pre_pll_clk_div*/
		133,		/*REG = 0x0307 pll_multiplier*/
		10,		/*REG = 0x0309 op_pix_clk_div*/
		0x01,		/*REG = 0x3030*/
		0x02,		/*REG = 0x0111*/
		0x00,		/*REG = 0x0b00 ,lens shading off */
		0x30,		/*REG = 0x3001*/
		0x34,		/*REG = 0x3004*/
		0x0b,		/*REG = 0x3007*/
		0x1F,		/*REG = 0x3016*/
		0x03,		/*REG = 0x301d*/
		0x11,		/*REG = 0x317E*/
		0x09,		/*REG = 0x317F*/
		0x38,		/*REG = 0x3400*/
		0x00,		/*REG_0x0b06*/
		0x80,		/*REG_0x0b07*/
		0x01,		/*REG_0x0b08*/
		0x4F,		/*REG_0x0b09*/
		0x18,		/*REG_0x0136*/
		0x00,		/*/REG_0x0137*/
		0x20,		/*REG = 0x0b83*/
		0x20,		/*REG = 0x0b84*/
		0x20,		/*REG = 0x0b85*/
		0x80,		/*REG = 0x0b88*/
		0x00,		/*REG = 0x0b89*/
		0x00,		/*REG = 0x0b8a*/
	}
};
/* Preview / Snapshot register settings */
const struct reg_struct vx6953_reg_pat[2] = {
	{/* Preview */
		0x03,	/*REG = 0x0202 coarse integration_time_hi*/
		0xd0,	/*REG = 0x0203 coarse_integration_time_lo*/
		0xc0,	/*REG = 0x0205 analogue_gain_code_global*/
		0x03,	/*REG = 0x0340 frame_length_lines_hi*/
		0xf0,	/*REG = 0x0341 frame_length_lines_lo*/
		0x0b,	/*REG = 0x0342  line_length_pck_hi*/
		0x74,	/*REG = 0x0343  line_length_pck_lo*/
		0x01,	/*REG = 0x3005*/
		0x00,	/*REG = 0x3010*/
		0x01,	/*REG = 0x3011*/
		0x55,	/*REG = 0x301a*/
		0x03,	/*REG = 0x3035*/
		0x23,	/*REG = 0x3036*/
		0x00,	/*REG = 0x3041*/
		0x24,	/*REG = 0x3042*/
		0xb1,	/*REG = 0x3045*/
		0x02,	/*REG = 0x0b80 edof estimate*/
		0x01,	/*REG = 0x0900*/
		0x22,	/*REG = 0x0901*/
		0x05,	/*REG = 0x0902*/
		0x03,	/*REG = 0x0383*/
		0x03,	/*REG = 0x0387*/
		0x05,	/*REG = 0x034c*/
		0x18,	/*REG = 0x034d*/
		0x03,	/*REG = 0x034e*/
		0xd4,	/*REG = 0x034f*/
		0x02,	/*0x1716*/
		0x04,	/*0x1717*/
		0x08,	/*0x1718*/
		0x2c,	/*0x1719*/
		0x02,
		0x04,
	},
	{ /* Snapshot */
		0x07,/*REG = 0x0202 coarse_integration_time_hi*/
		0x00,/*REG = 0x0203 coarse_integration_time_lo*/
		0xc0,/*REG = 0x0205 analogue_gain_code_global*/
		0x07,/*REG = 0x0340 frame_length_lines_hi*/
		0xd0,/*REG = 0x0341 frame_length_lines_lo*/
		0x0b,/*REG = 0x0342 line_length_pck_hi*/
		0x8c,/*REG = 0x0343 line_length_pck_lo*/
		0x01,/*REG = 0x3005*/
		0x00,/*REG = 0x3010*/
		0x00,/*REG = 0x3011*/
		0x55,/*REG = 0x301a*/
		0x01,/*REG = 0x3035*/
		0x23,/*REG = 0x3036*/
		0x00,/*REG = 0x3041*/
		0x24,/*REG = 0x3042*/
		0xb7,/*REG = 0x3045*/
		0x01,/*REG = 0x0b80 edof application*/
		0x00,/*REG = 0x0900*/
		0x00,/*REG = 0x0901*/
		0x00,/*REG = 0x0902*/
		0x01,/*REG = 0x0383*/
		0x01,/*REG = 0x0387*/
		0x0A,/*REG = 0x034c*/
		0x30,/*REG = 0x034d*/
		0x07,/*REG = 0x034e*/
		0xA8,/*REG = 0x034f*/
		0x02,/*0x1716*/
		0x0d,/*0x1717*/
		0x07,/*0x1718*/
		0x7d,/*0x1719*/
		0x02,
		0x00,
	}
};



struct vx6953_reg vx6953_regs = {
	.reg_pat_init = &vx6953_reg_init[0],
	.reg_pat = &vx6953_reg_pat[0],
};
