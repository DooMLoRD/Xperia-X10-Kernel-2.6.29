/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/pmic.h>
#include <mach/msm_qdsp6_audio.h>

#define GPIO_HEADSET_AMP 149

#ifdef CONFIG_AUDIO_QDSP_AMP_GPIO_CTRL
#define GPIO_SPEAKER_AMP 154
#endif

void analog_init(void)
{
#ifndef CONFIG_AUDIO_QDSP_AMP_GPIO_CTRL
	/* stereo pmic init */
	pmic_spkr_set_gain(LEFT_SPKR, SPKR_GAIN_PLUS12DB);
	pmic_spkr_set_gain(RIGHT_SPKR, SPKR_GAIN_PLUS12DB);
#endif

	pmic_mic_set_volt(MIC_VOLT_2_00V);

	gpio_direction_output(GPIO_HEADSET_AMP, 1);
	gpio_set_value(GPIO_HEADSET_AMP, 0);
}

void analog_headset_enable(int en)
{
	if (en)
		msleep(50);

	/* enable audio amp */
	gpio_set_value(GPIO_HEADSET_AMP, !!en);

	msleep(5);
}

void analog_speaker_enable(int en)
{
#ifdef CONFIG_AUDIO_QDSP_AMP_GPIO_CTRL
	if (en) {
		/* enable audio amp */
		msleep(50);
		gpio_set_value(GPIO_SPEAKER_AMP, 1);
		msleep(30);
	} else {
		gpio_set_value(GPIO_SPEAKER_AMP, 0);
		msleep(5);
	}
#else
	struct spkr_config_mode scm;
	memset(&scm, 0, sizeof(scm));

	if (en) {
		scm.is_right_chan_en = 1;
		scm.is_left_chan_en = 1;
		scm.is_stereo_en = 1;

		scm.is_hpf_en = 0;

		pmic_spkr_en_mute(LEFT_SPKR, 0);
		pmic_spkr_en_mute(RIGHT_SPKR, 0);

		pmic_spkr_set_mux_hpf_corner_freq( SPKR_FREQ_0_57KHZ );

		pmic_set_spkr_configuration(&scm);
		pmic_spkr_en(LEFT_SPKR, 1);
		pmic_spkr_en(RIGHT_SPKR, 1);
		
		/* unmute */
		pmic_spkr_en_mute(LEFT_SPKR, 1);
		pmic_spkr_en_mute(RIGHT_SPKR, 1);

		msleep(30);
	} else {
		pmic_spkr_en_mute(LEFT_SPKR, 0);
		pmic_spkr_en_mute(RIGHT_SPKR, 0);

		pmic_spkr_en_left_chan(0);
		pmic_spkr_en_right_chan(0);

		pmic_spkr_en(LEFT_SPKR, 0);
		pmic_spkr_en(RIGHT_SPKR, 0);

		pmic_set_spkr_configuration(&scm);
	}
#endif
}

void analog_mic_enable(int en)
{
	pmic_mic_en(en);
}

static struct q6audio_analog_ops ops = {
	.init = analog_init,
	.speaker_enable = analog_speaker_enable,
	.headset_enable = analog_headset_enable,
	.int_mic_enable = analog_mic_enable,
	.ext_mic_enable = analog_mic_enable,
};

static int __init init(void)
{
	q6audio_register_analog_ops(&ops);
	return 0;
}

device_initcall(init);
