/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
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

#include <linux/delay.h>
#include <linux/pwm.h>
#ifdef CONFIG_PMIC8058_PWM
#include <linux/mfd/pmic8058.h>
#include <linux/pmic8058-pwm.h>
#endif
#include <mach/gpio.h>
#include "msm_fb.h"


static int lcdc_sharp_panel_off(struct platform_device *pdev);

#ifdef CONFIG_PMIC8058_PWM
static struct pwm_device *bl_pwm;

/* 50 Khz == 20000 ns period
 * divide 20000 ns to 15 levels
 * each level has 1333 ns
 */

#define PWM_PERIOD 20000	/* ns, period of 50Khz */
#endif

static int spi_cs;
static int spi_sclk;
static int spi_mosi;
static int spi_miso;
static unsigned char bit_shift[8] = { (1 << 7),	/* MSB */
	(1 << 6),
	(1 << 5),
	(1 << 4),
	(1 << 3),
	(1 << 2),
	(1 << 1),
	(1 << 0)		               /* LSB */
};

struct sharp_state_type {
	boolean disp_initialized;
	boolean display_on;
	boolean disp_powered_up;
};

struct sharp_spi_data {
	u8 addr;
	u8 data;
};

static struct sharp_spi_data init_sequence[] = {
	{  15, 0x01 },
	{   5, 0x01 },
	{   7, 0x10 },
	{   9, 0x1E },
	{  10, 0x04 },
	{  17, 0xFF },
	{  21, 0x8A },
	{  22, 0x00 },
	{  23, 0x82 },
	{  24, 0x24 },
	{  25, 0x22 },
	{  26, 0x6D },
	{  27, 0xEB },
	{  28, 0xB9 },
	{  29, 0x3A },
	{  49, 0x1A },
	{  50, 0x16 },
	{  51, 0x05 },
	{  55, 0x7F },
	{  56, 0x15 },
	{  57, 0x7B },
	{  60, 0x05 },
	{  61, 0x0C },
	{  62, 0x80 },
	{  63, 0x00 },
	{  92, 0x90 },
	{  97, 0x01 },
	{  98, 0xFF },
	{ 113, 0x11 },
	{ 114, 0x02 },
	{ 115, 0x08 },
	{ 123, 0xAB },
	{ 124, 0x04 },
	{   6, 0x02 },
	{ 133, 0x00 },
	{ 134, 0xFE },
	{ 135, 0x22 },
	{ 136, 0x0B },
	{ 137, 0xFF },
	{ 138, 0x0F },
	{ 139, 0x00 },
	{ 140, 0xFE },
	{ 141, 0x22 },
	{ 142, 0x0B },
	{ 143, 0xFF },
	{ 144, 0x0F },
	{ 145, 0x00 },
	{ 146, 0xFE },
	{ 147, 0x22 },
	{ 148, 0x0B },
	{ 149, 0xFF },
	{ 150, 0x0F },
	{ 202, 0x30 },
	{  30, 0x01 },
	{   4, 0x01 },
	{  31, 0x41 },
};

static struct sharp_state_type sharp_state = { 0 };
static struct msm_panel_common_pdata *lcdc_sharp_pdata;

static void sharp_spi_write_byte(u8 val)
{
	int i;

	/* Clock should be Low before entering */
	for (i = 0; i < 8; i++) {
		/* #1: Drive the Data (High or Low) */
		if (val & bit_shift[i])
			gpio_set_value(spi_mosi, 1);
		else
			gpio_set_value(spi_mosi, 0);

		/* #2: Drive the Clk High and then Low */
		gpio_set_value(spi_sclk, 1);
		gpio_set_value(spi_sclk, 0);
	}
}

static void serigo(u8 reg, u8 data)
{
	/* Enable the Chip Select - low */
	gpio_set_value(spi_cs, 0);
	udelay(1);

	/* Transmit register address first, then data */
	sharp_spi_write_byte(reg);

	/* Idle state of MOSI is Low */
	gpio_set_value(spi_mosi, 0);
	udelay(1);
	sharp_spi_write_byte(data);

	gpio_set_value(spi_mosi, 0);
	gpio_set_value(spi_cs, 1);
}

static void sharp_spi_init(void)
{
	spi_sclk = *(lcdc_sharp_pdata->gpio_num);
	spi_cs   = *(lcdc_sharp_pdata->gpio_num + 1);
	spi_mosi = *(lcdc_sharp_pdata->gpio_num + 2);
	spi_miso = *(lcdc_sharp_pdata->gpio_num + 3);

	/* Set the output so that we don't disturb the slave device */
	gpio_set_value(spi_sclk, 0);
	gpio_set_value(spi_mosi, 0);

	/* Set the Chip Select deasserted (active low) */
	gpio_set_value(spi_cs, 1);
}

static void sharp_disp_powerup(void)
{
	if (!sharp_state.disp_powered_up && !sharp_state.display_on)
		sharp_state.disp_powered_up = TRUE;
}

static void sharp_disp_on(void)
{
	int i;

	if (sharp_state.disp_powered_up && !sharp_state.display_on) {
		for (i = 0; i < ARRAY_SIZE(init_sequence); i++) {
			serigo(init_sequence[i].addr,
			       init_sequence[i].data);
		}
		mdelay(10);
		serigo(31, 0xC1);
		mdelay(10);
		serigo(31, 0xD9);
		serigo(31, 0xDF);

		sharp_state.display_on = TRUE;
	}
}

static int lcdc_sharp_panel_on(struct platform_device *pdev)
{
	if (!sharp_state.disp_initialized) {
		lcdc_sharp_pdata->panel_config_gpio(1);
		sharp_spi_init();
		sharp_disp_powerup();
		sharp_disp_on();
		sharp_state.disp_initialized = TRUE;
	}
	return 0;
}

static int lcdc_sharp_panel_off(struct platform_device *pdev)
{
	if (sharp_state.disp_powered_up && sharp_state.display_on) {
		serigo(4, 0x00);
		mdelay(40);
		serigo(31, 0xC1);
		mdelay(40);
		serigo(31, 0x00);
		mdelay(100);
		sharp_state.display_on = FALSE;
		sharp_state.disp_initialized = FALSE;
	}
	return 0;
}

static void lcdc_sharp_panel_set_backlight(struct msm_fb_data_type *mfd)
{
	int bl_level;

	bl_level = mfd->bl_level;

#ifdef CONFIG_PMIC8058_PWM
	if (bl_pwm) {
		int duty_level;
		duty_level = (PWM_PERIOD / mfd->panel_info.bl_max);
		pwm_config(bl_pwm, duty_level * bl_level, PWM_PERIOD);
		pwm_enable(bl_pwm);
	}
#endif
}

static int __init sharp_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		lcdc_sharp_pdata = pdev->dev.platform_data;
		return 0;
	}

#ifdef CONFIG_PMIC8058_PWM
	bl_pwm = pwm_request(lcdc_sharp_pdata->gpio, "backlight");
	if (bl_pwm == NULL || IS_ERR(bl_pwm)) {
		pr_err("%s pwm_request() failed\n", __func__);
		bl_pwm = NULL;
	}

	printk(KERN_INFO "sharp_probe: bl_pwm=%x LPG_chan=%d\n",
			(int) bl_pwm, (int)lcdc_sharp_pdata->gpio);
#endif

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = sharp_probe,
	.driver = {
		.name   = "lcdc_sharp_wvga",
	},
};

static struct msm_fb_panel_data sharp_panel_data = {
	.on = lcdc_sharp_panel_on,
	.off = lcdc_sharp_panel_off,
	.set_backlight = lcdc_sharp_panel_set_backlight,
};

static struct platform_device this_device = {
	.name   = "lcdc_sharp_wvga",
	.id	= 1,
	.dev	= {
		.platform_data = &sharp_panel_data,
	}
};

static int __init lcdc_sharp_panel_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

#ifdef CONFIG_FB_MSM_MDDI_AUTO_DETECT
	if (msm_fb_detect_client("lcdc_sharp_wvga_pt"))
		return 0;
#endif

	ret = platform_driver_register(&this_driver);
	if (ret)
		return ret;

	pinfo = &sharp_panel_data.panel_info;
	pinfo->xres = 480;
	pinfo->yres = 800;
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 18;
	pinfo->fb_num = 2;
	pinfo->clk_rate = 24500000;
	pinfo->bl_max = 15;
	pinfo->bl_min = 1;

	pinfo->lcdc.h_back_porch = 20;
	pinfo->lcdc.h_front_porch = 10;
	pinfo->lcdc.h_pulse_width = 10;
	pinfo->lcdc.v_back_porch = 2;
	pinfo->lcdc.v_front_porch = 2;
	pinfo->lcdc.v_pulse_width = 2;
	pinfo->lcdc.border_clr = 0;
	pinfo->lcdc.underflow_clr = 0xff;
	pinfo->lcdc.hsync_skew = 0;

	ret = platform_device_register(&this_device);
	if (ret)
		platform_driver_unregister(&this_driver);

	return ret;
}

module_init(lcdc_sharp_panel_init);
