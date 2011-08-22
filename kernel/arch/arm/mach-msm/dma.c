/* linux/arch/arm/mach-msm/dma.c
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
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

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <mach/dma.h>

#define MODULE_NAME "msm_dmov"
#define MSM_DMOV_CHANNEL_COUNT 16

enum {
	MSM_DMOV_PRINT_ERRORS = 1,
	MSM_DMOV_PRINT_IO = 2,
	MSM_DMOV_PRINT_FLOW = 4
};

enum {
	CLK_DIS,
	CLK_TO_BE_DIS,
	CLK_EN
};

static DEFINE_SPINLOCK(msm_dmov_lock);
static struct clk *msm_dmov_clk;
static unsigned int channel_active;
static struct list_head ready_commands[MSM_DMOV_CHANNEL_COUNT];
static struct list_head active_commands[MSM_DMOV_CHANNEL_COUNT];
unsigned int msm_dmov_print_mask = MSM_DMOV_PRINT_ERRORS;
unsigned int clk_ctl = CLK_DIS;

#define MSM_DMOV_DPRINTF(mask, format, args...) \
	do { \
		if ((mask) & msm_dmov_print_mask) \
			printk(KERN_ERR format, args); \
	} while (0)
#define PRINT_ERROR(format, args...) \
	MSM_DMOV_DPRINTF(MSM_DMOV_PRINT_ERRORS, format, args);
#define PRINT_IO(format, args...) \
	MSM_DMOV_DPRINTF(MSM_DMOV_PRINT_IO, format, args);
#define PRINT_FLOW(format, args...) \
	MSM_DMOV_DPRINTF(MSM_DMOV_PRINT_FLOW, format, args);

void msm_dmov_stop_cmd(unsigned id, struct msm_dmov_cmd *cmd, int graceful)
{
	writel((graceful << 31), DMOV_FLUSH0(id));
}
EXPORT_SYMBOL(msm_dmov_stop_cmd);

static void timer_func(unsigned long func_paramter)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&msm_dmov_lock, irq_flags);
	if (clk_ctl == CLK_TO_BE_DIS) {
		BUG_ON(channel_active);
		clk_disable(msm_dmov_clk);
		clk_ctl = CLK_DIS;
	}
	spin_unlock_irqrestore(&msm_dmov_lock, irq_flags);
}
DEFINE_TIMER(timer, timer_func, 0, 0);

void msm_dmov_enqueue_cmd_ext(unsigned id, struct msm_dmov_cmd *cmd)
{
	unsigned long irq_flags;
	unsigned int status;

	spin_lock_irqsave(&msm_dmov_lock, irq_flags);
	if (clk_ctl == CLK_DIS)
		clk_enable(msm_dmov_clk);
	else if (clk_ctl == CLK_TO_BE_DIS)
		del_timer(&timer);
	clk_ctl = CLK_EN;

	status = readl(DMOV_STATUS(id));
	if (list_empty(&ready_commands[id]) &&
		(status & DMOV_STATUS_CMD_PTR_RDY)) {
		if (cmd->exec_func)
			cmd->exec_func(cmd);
		PRINT_IO(
		 "msm_dmov_enqueue_cmd_ext(%d), start command, status %x\n",
		 id, status);
		list_add_tail(&cmd->list, &active_commands[id]);
		if (!channel_active)
			enable_irq(INT_ADM_AARM);
		channel_active |= 1U << id;
		writel(cmd->cmdptr, DMOV_CMD_PTR(id));
	} else {
		if (!channel_active) {
			clk_ctl = CLK_TO_BE_DIS;
			mod_timer(&timer, jiffies + HZ);
		}
		if (list_empty(&active_commands[id]))
			PRINT_ERROR(
			 "msm_dmov_enqueue_cmd_ext(%d), stalled, status %x\n",
			 id, status);

		PRINT_IO(
		 "msm_dmov_enqueue_cmd_ext(%d), enqueue command, status %x\n",
		 id, status);
		list_add_tail(&cmd->list, &ready_commands[id]);
	}
	spin_unlock_irqrestore(&msm_dmov_lock, irq_flags);
}
EXPORT_SYMBOL(msm_dmov_enqueue_cmd_ext);

void msm_dmov_reset(void)
{
	int i, ch;
	unsigned int int_status;

	/* read and clear ADM interrupt */
	int_status = readl(DMOV_ISR);
	/* read the results and clean fifo */
	for (ch = 0; ch < MSM_DMOV_CHANNEL_COUNT; ch++) {
		for (i = 0; i < 3; i++) {	/* the FIFO is 3-deep */
			(void)readl(DMOV_RSLT(ch));
		}
	}

	return;
}

void msm_dmov_enqueue_cmd(unsigned id, struct msm_dmov_cmd *cmd)
{
	/* Disable callback function (for backwards compatibility) */
	cmd->exec_func = NULL;

	msm_dmov_enqueue_cmd_ext(id, cmd);
}
EXPORT_SYMBOL(msm_dmov_enqueue_cmd);

void msm_dmov_flush(unsigned int id)
{
	unsigned long irq_flags;
	spin_lock_irqsave(&msm_dmov_lock, irq_flags);
	/* XXX not checking if flush cmd sent already */
	if (!list_empty(&active_commands[id])) {
		PRINT_IO("msm_dmov_flush(%d), send flush cmd\n", id);
		writel(DMOV_FLUSH_TYPE, DMOV_FLUSH0(id));
	}
	spin_unlock_irqrestore(&msm_dmov_lock, irq_flags);
}
EXPORT_SYMBOL(msm_dmov_flush);

struct msm_dmov_exec_cmdptr_cmd {
	struct msm_dmov_cmd dmov_cmd;
	struct completion complete;
	unsigned id;
	unsigned int result;
	struct msm_dmov_errdata err;
};

static void
dmov_exec_cmdptr_complete_func(struct msm_dmov_cmd *_cmd,
			       unsigned int result,
			       struct msm_dmov_errdata *err)
{
	struct msm_dmov_exec_cmdptr_cmd *cmd = container_of(_cmd, struct msm_dmov_exec_cmdptr_cmd, dmov_cmd);
	cmd->result = result;
	if (result != 0x80000002 && err)
		memcpy(&cmd->err, err, sizeof(struct msm_dmov_errdata));

	complete(&cmd->complete);
}

int msm_dmov_exec_cmd(unsigned id, unsigned int cmdptr)
{
	struct msm_dmov_exec_cmdptr_cmd cmd;

	PRINT_FLOW("dmov_exec_cmdptr(%d, %x)\n", id, cmdptr);

	cmd.dmov_cmd.cmdptr = cmdptr;
	cmd.dmov_cmd.complete_func = dmov_exec_cmdptr_complete_func;
	cmd.dmov_cmd.exec_func = NULL;
	cmd.id = id;
	init_completion(&cmd.complete);

	msm_dmov_enqueue_cmd(id, &cmd.dmov_cmd);
	wait_for_completion_io(&cmd.complete);

	if (cmd.result != 0x80000002) {
		PRINT_ERROR("dmov_exec_cmdptr(%d): ERROR, result: %x\n", id, cmd.result);
		PRINT_ERROR("dmov_exec_cmdptr(%d):  flush: %x %x %x %x\n",
			id, cmd.err.flush[0], cmd.err.flush[1], cmd.err.flush[2], cmd.err.flush[3]);
		return -EIO;
	}
	PRINT_FLOW("dmov_exec_cmdptr(%d, %x) done\n", id, cmdptr);
	return 0;
}
EXPORT_SYMBOL(msm_dmov_exec_cmd);


static irqreturn_t msm_datamover_irq_handler(int irq, void *dev_id)
{
	unsigned int int_status, mask, id;
	unsigned long irq_flags;
	unsigned int ch_status;
	unsigned int ch_result;
	struct msm_dmov_cmd *cmd;

	spin_lock_irqsave(&msm_dmov_lock, irq_flags);

	int_status = readl(DMOV_ISR); /* read and clear interrupt */
	PRINT_FLOW("msm_datamover_irq_handler: DMOV_ISR %x\n", int_status);

	while (int_status) {
		mask = int_status & -int_status;
		id = fls(mask) - 1;
		PRINT_FLOW("msm_datamover_irq_handler %08x %08x id %d\n", int_status, mask, id);
		int_status &= ~mask;
		ch_status = readl(DMOV_STATUS(id));
		if (!(ch_status & DMOV_STATUS_RSLT_VALID)) {
			PRINT_FLOW("msm_datamover_irq_handler id %d, result not valid %x\n", id, ch_status);
			continue;
		}
		do {
			ch_result = readl(DMOV_RSLT(id));
			if (list_empty(&active_commands[id])) {
				PRINT_ERROR("msm_datamover_irq_handler id %d, got result "
					"with no active command, status %x, result %x\n",
					id, ch_status, ch_result);
				cmd = NULL;
			} else
				cmd = list_entry(active_commands[id].next, typeof(*cmd), list);
			PRINT_FLOW("msm_datamover_irq_handler id %d, status %x, result %x\n", id, ch_status, ch_result);
			if (ch_result & DMOV_RSLT_DONE) {
				PRINT_FLOW("msm_datamover_irq_handler id %d, status %x\n",
					id, ch_status);
				PRINT_IO("msm_datamover_irq_handler id %d, got result "
					"for %p, result %x\n", id, cmd, ch_result);
				if (cmd) {
					list_del(&cmd->list);
					cmd->complete_func(cmd, ch_result, NULL);
				}
			}
			if (ch_result & DMOV_RSLT_FLUSH) {
				struct msm_dmov_errdata errdata;

				errdata.flush[0] = readl(DMOV_FLUSH0(id));
				errdata.flush[1] = readl(DMOV_FLUSH1(id));
				errdata.flush[2] = readl(DMOV_FLUSH2(id));
				errdata.flush[3] = readl(DMOV_FLUSH3(id));
				errdata.flush[4] = readl(DMOV_FLUSH4(id));
				errdata.flush[5] = readl(DMOV_FLUSH5(id));
				PRINT_FLOW("msm_datamover_irq_handler id %d, status %x\n", id, ch_status);
				PRINT_FLOW("msm_datamover_irq_handler id %d, flush, result %x, flush0 %x\n", id, ch_result, errdata.flush[0]);
				if (cmd) {
					list_del(&cmd->list);
					cmd->complete_func(cmd, ch_result, &errdata);
				}
			}
			if (ch_result & DMOV_RSLT_ERROR) {
				struct msm_dmov_errdata errdata;

				errdata.flush[0] = readl(DMOV_FLUSH0(id));
				errdata.flush[1] = readl(DMOV_FLUSH1(id));
				errdata.flush[2] = readl(DMOV_FLUSH2(id));
				errdata.flush[3] = readl(DMOV_FLUSH3(id));
				errdata.flush[4] = readl(DMOV_FLUSH4(id));
				errdata.flush[5] = readl(DMOV_FLUSH5(id));

				PRINT_ERROR("msm_datamover_irq_handler id %d, status %x\n", id, ch_status);
				PRINT_ERROR("msm_datamover_irq_handler id %d, error, result %x, flush0 %x\n", id, ch_result, errdata.flush[0]);
				if (cmd) {
					list_del(&cmd->list);
					cmd->complete_func(cmd, ch_result, &errdata);
				}
				/* this does not seem to work, once we get an error */
				/* the datamover will no longer accept commands */
				writel(0, DMOV_FLUSH0(id));

				/*
				 * When the DMOV signals an error the hardware
				 * seems unable to recover (or SW don't perform
				 * the correct * reset).
				 * The register * HI0_CHn_STATUS_SDs
				 * (offset 0x200) is * always read as 0 and thus
				 * no new * commands can be issued to the block
				 * (CMD_PTR_RDY never gets set). All threads
				 * accessing the nand will be parked in the
				 * ready_commands queue,indefinetely waiting for
				 * "nothing" to complete.
				 *
				 * For now we only do this with the NAND channel
				 * since we don't know if stalls on other
				 * channels cause a freeze.
				 */
				if (id == DMOV_NAND_CHAN) {
					panic("msm_datamover_irq_handler(%d). Unable to recover "
					      "from DMOV stall, Bailing out...\n", id);
				}
			}
			ch_status = readl(DMOV_STATUS(id));
			PRINT_FLOW("msm_datamover_irq_handler id %d, status %x\n", id, ch_status);
			if ((ch_status & DMOV_STATUS_CMD_PTR_RDY) && !list_empty(&ready_commands[id])) {
				cmd = list_entry(ready_commands[id].next, typeof(*cmd), list);
				list_del(&cmd->list);
				list_add_tail(&cmd->list, &active_commands[id]);
				if (cmd->exec_func)
					cmd->exec_func(cmd);
				PRINT_FLOW("msm_datamover_irq_handler id %d, start command\n", id);
				writel(cmd->cmdptr, DMOV_CMD_PTR(id));
			}
		} while (ch_status & DMOV_STATUS_RSLT_VALID);
		if (list_empty(&active_commands[id]) && list_empty(&ready_commands[id]))
			channel_active &= ~(1U << id);
		PRINT_FLOW("msm_datamover_irq_handler id %d, status %x\n", id, ch_status);
	}

	if (!channel_active) {
		disable_irq(INT_ADM_AARM);
		clk_ctl = CLK_TO_BE_DIS;
		mod_timer(&timer, jiffies + HZ);
	}

	spin_unlock_irqrestore(&msm_dmov_lock, irq_flags);
	return IRQ_HANDLED;
}

static int msm_dmov_suspend_late(struct platform_device *pdev,
			    pm_message_t state)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&msm_dmov_lock, irq_flags);
	if (clk_ctl == CLK_TO_BE_DIS) {
		BUG_ON(channel_active);
		del_timer(&timer);
		clk_disable(msm_dmov_clk);
		clk_ctl = CLK_DIS;
	}
	spin_unlock_irqrestore(&msm_dmov_lock, irq_flags);
	return 0;
}

static struct platform_driver msm_dmov_driver = {
	.suspend_late = msm_dmov_suspend_late,
	.driver = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init msm_init_datamover(void)
{
	int i;
	int ret;

	/* If we are in KDUMP Kernel*/
	if (reset_devices)
		msm_dmov_reset();

	for (i = 0; i < MSM_DMOV_CHANNEL_COUNT; i++) {
		INIT_LIST_HEAD(&ready_commands[i]);
		INIT_LIST_HEAD(&active_commands[i]);
		writel(DMOV_CONFIG_IRQ_EN | DMOV_CONFIG_FORCE_TOP_PTR_RSLT | DMOV_CONFIG_FORCE_FLUSH_RSLT, DMOV_CONFIG(i));
	}
	msm_dmov_clk = clk_get(NULL, "adm_clk");
	if (IS_ERR(msm_dmov_clk))
		return PTR_ERR(msm_dmov_clk);
	ret = request_irq(INT_ADM_AARM, msm_datamover_irq_handler, 0, "msmdatamover", NULL);
	if (ret)
		return ret;
	disable_irq(INT_ADM_AARM);
	ret = platform_driver_register(&msm_dmov_driver);
	if (ret)
		return ret;
	return 0;
}

arch_initcall(msm_init_datamover);
