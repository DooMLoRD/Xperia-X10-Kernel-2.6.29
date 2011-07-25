/*
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) 2009 HTC Corporation
 * Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * Author: Brian Swetland <swetland@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/uaccess.h>

#include <linux/msm_audio_qcp.h>
#include <mach/msm_qdsp6_audio.h>
#include <mach/debug_audio_mm.h>
#include "dal_audio_format.h"

struct qcelp {
	struct mutex lock;
	struct msm_audio_qcelp_enc_config cfg;
	struct msm_audio_stream_config str_cfg;
	struct audio_client *audio_client;
	struct msm_voicerec_mode voicerec_mode;
};


static long q6_qcelp_in_ioctl(struct file *file, unsigned int cmd,
				 unsigned long arg)
{
	struct qcelp *qcelp = file->private_data;
	int rc = 0;


	mutex_lock(&qcelp->lock);
	switch (cmd) {
	case AUDIO_SET_VOLUME:
		break;
	case AUDIO_GET_STATS:
	{
		struct msm_audio_stats stats;
		memset(&stats, 0, sizeof(stats));
		if (copy_to_user((void *) arg, &stats, sizeof(stats))) {
			mutex_unlock(&qcelp->lock);
			return -EFAULT;
		}
		mutex_unlock(&qcelp->lock);
		return 0;
	}
	case AUDIO_START:
	{
		uint32_t acdb_id;
		if (arg == 0) {
			acdb_id = 0;
		} else {
			if (copy_from_user(&acdb_id,
				(void *) arg, sizeof(acdb_id))) {
				rc = -EFAULT;
				break;
			}
		}
		if (qcelp->audio_client) {
			rc = -EBUSY;
			break;
		} else {
			qcelp->audio_client = q6audio_open_qcp(
				qcelp->str_cfg.buffer_size,
				qcelp->cfg.min_bit_rate,
				qcelp->cfg.max_bit_rate,
				qcelp->voicerec_mode.rec_mode,
				ADSP_AUDIO_FORMAT_V13K_FS,
				acdb_id);

			if (!qcelp->audio_client) {
				kfree(qcelp);
				rc = -ENOMEM;
				break;
			}
		}
		break;
	}
	case AUDIO_STOP:
		break;
	case AUDIO_FLUSH:
		break;
	case AUDIO_SET_INCALL: {
		if (copy_from_user(&qcelp->voicerec_mode,
			(void *)arg, sizeof(struct msm_voicerec_mode)))
			rc = -EFAULT;

		if (qcelp->voicerec_mode.rec_mode != AUDIO_FLAG_READ
			&& qcelp->voicerec_mode.rec_mode !=
			AUDIO_FLAG_INCALL_MIXED) {
			qcelp->voicerec_mode.rec_mode = AUDIO_FLAG_READ;
			MM_ERR("Invalid rec_mode\n");
			rc = -EINVAL;
		}
		break;
	}
	case AUDIO_GET_STREAM_CONFIG:
		if (copy_to_user((void *)arg, &qcelp->str_cfg,
				sizeof(struct msm_audio_stream_config)))
			rc = -EFAULT;
		break;
	case AUDIO_SET_STREAM_CONFIG:
		if (copy_from_user(&qcelp->str_cfg, (void *)arg,
			sizeof(struct msm_audio_stream_config))) {
			rc = -EFAULT;
			break;
		}

		if (qcelp->str_cfg.buffer_size < 35) {
			MM_ERR("Buffer size too small\n");
			rc = -EINVAL;
			break;
		}

		if (qcelp->str_cfg.buffer_count != 2)
			MM_INFO("Buffer count set to 2\n");
		break;
	case AUDIO_SET_QCELP_ENC_CONFIG:
		if (copy_from_user(&qcelp->cfg, (void *) arg,
				sizeof(struct msm_audio_qcelp_enc_config)))
			rc = -EFAULT;

		if (qcelp->cfg.min_bit_rate > 4 ||
			 qcelp->cfg.min_bit_rate < 1) {

			MM_ERR("invalid min bitrate\n");
			rc = -EINVAL;
		}
		if (qcelp->cfg.max_bit_rate > 4 ||
			 qcelp->cfg.max_bit_rate < 1) {

			MM_ERR("invalid max bitrate\n");
			rc = -EINVAL;
		}

		break;
	case AUDIO_GET_QCELP_ENC_CONFIG:
		if (copy_to_user((void *) arg, &qcelp->cfg,
			 sizeof(struct msm_audio_qcelp_enc_config)))
			rc = -EFAULT;
		break;

	default:
		rc = -EINVAL;
	}
	mutex_unlock(&qcelp->lock);
	return rc;
}

static int q6_qcelp_in_open(struct inode *inode, struct file *file)
{
	struct qcelp *qcelp;
	qcelp = kmalloc(sizeof(struct qcelp), GFP_KERNEL);
	if (qcelp == NULL) {
		MM_ERR("Could not allocate memory for qcelp driver\n");
		return -ENOMEM;
	}

	mutex_init(&qcelp->lock);
	file->private_data = qcelp;
	qcelp->audio_client = NULL;
	qcelp->str_cfg.buffer_size = 35;
	qcelp->str_cfg.buffer_count = 2;
	qcelp->cfg.cdma_rate = CDMA_RATE_FULL;
	qcelp->cfg.min_bit_rate = 1;
	qcelp->cfg.max_bit_rate = 4;
	qcelp->voicerec_mode.rec_mode = AUDIO_FLAG_READ;
	return 0;
}

static ssize_t q6_qcelp_in_read(struct file *file, char __user *buf,
			   size_t count, loff_t *pos)
{
	struct audio_client *ac;
	struct audio_buffer *ab;
	const char __user *start = buf;
	struct qcelp *qcelp = file->private_data;
	int xfer = 0;
	int res;

	mutex_lock(&qcelp->lock);
	ac = qcelp->audio_client;
	if (!ac) {
		res = -ENODEV;
		goto fail;
	}
	while (count > xfer) {
		ab = ac->buf + ac->cpu_buf;

		if (ab->used)
			wait_event(ac->wait, (ab->used == 0));

		xfer = ab->actual_size;

		if (copy_to_user(buf, ab->data, xfer)) {
			res = -EFAULT;
			goto fail;
		}

		buf += xfer;
		count -= xfer;

		ab->used = 1;
		q6audio_read(ac, ab);
		ac->cpu_buf ^= 1;
	}

	res = buf - start;

fail:
	mutex_unlock(&qcelp->lock);

	return res;
}

static int q6_qcelp_in_release(struct inode *inode, struct file *file)
{
	int rc = 0;
	struct qcelp *qcelp = file->private_data;

	mutex_lock(&qcelp->lock);
	if (qcelp->audio_client)
		rc = q6audio_close(qcelp->audio_client);
	mutex_unlock(&qcelp->lock);
	kfree(qcelp);
	return rc;
}

static const struct file_operations q6_qcelp_in_fops = {
	.owner		= THIS_MODULE,
	.open		= q6_qcelp_in_open,
	.read		= q6_qcelp_in_read,
	.release	= q6_qcelp_in_release,
	.unlocked_ioctl	= q6_qcelp_in_ioctl,
};

struct miscdevice q6_qcelp_in_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "msm_qcelp_in",
	.fops	= &q6_qcelp_in_fops,
};

static int __init q6_qcelp_in_init(void)
{
	return misc_register(&q6_qcelp_in_misc);
}

device_initcall(q6_qcelp_in_init);
