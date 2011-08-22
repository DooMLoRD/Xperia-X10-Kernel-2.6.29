/* linux/arch/arm/mach-msm/timer.c
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
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

#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/delay.h>
#include <linux/io.h>

#include <asm/mach/time.h>
#include <mach/msm_iomap.h>

#include "smd_private.h"
#include "timer.h"

enum {
	MSM_TIMER_DEBUG_SYNC = 1U << 0,
};
static int msm_timer_debug_mask;
module_param_named(debug_mask, msm_timer_debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

#if defined(CONFIG_ARCH_MSM7X30)
#define MSM_GPT_BASE (MSM_TMR_BASE + 0x4)
#define MSM_DGT_BASE (MSM_TMR_BASE + 0x24)
#else
#define MSM_GPT_BASE MSM_TMR_BASE
#define MSM_DGT_BASE (MSM_TMR_BASE + 0x10)
#endif

#if defined(CONFIG_ARCH_MSM_ARM11)
#define MSM_DGT_SHIFT (5)
#else
#define MSM_DGT_SHIFT (0)
#endif

#define TIMER_MATCH_VAL         0x0000
#define TIMER_COUNT_VAL         0x0004
#define TIMER_ENABLE            0x0008
#define TIMER_ENABLE_EN		1

#if defined(CONFIG_ARCH_QSD8X50)
#define DGT_HZ 4800000	/* Uses TCXO/4 (19.2 MHz / 4) */
#elif defined(CONFIG_ARCH_MSM7X30)
#define DGT_HZ 6144000	/* Uses LPXO/4 (24.576 MHz / 4) */
#else
#define DGT_HZ 19200000	/* Uses TCXO (19.2 MHz) */
#endif

#define GPT_HZ 32768
#define SCLK_HZ 32768

#if defined(CONFIG_MSM_N_WAY_SMSM)
/* Time Master State Bits */
#define MASTER_BITS_PER_CPU        1
#define MASTER_TIME_PENDING \
	(0x01UL << (MASTER_BITS_PER_CPU * SMSM_APPS_STATE))

/* Time Slave State Bits */
#define SLAVE_TIME_REQUEST         0x0400
#define SLAVE_TIME_POLL            0x0800
#define SLAVE_TIME_INIT            0x1000
#endif

enum {
	MSM_CLOCK_FLAGS_UNSTABLE_COUNT = 1U << 0,
	MSM_CLOCK_FLAGS_ODD_MATCH_WRITE = 1U << 1,
	MSM_CLOCK_FLAGS_DELAYED_WRITE_POST = 1U << 2,
};

struct msm_clock {
	struct clock_event_device   clockevent;
	struct clocksource          clocksource;
	struct irqaction            irq;
	void __iomem                *regbase;
	uint32_t                    freq;
	uint32_t                    shift;
	uint32_t                    flags;
	uint32_t                    write_delay;
	uint32_t                    last_set;
	uint32_t                    sleep_offset;
	uint32_t                    alarm_vtime;
	uint32_t                    non_sleep_offset;
	uint32_t                    in_sync;
	cycle_t                     stopped_tick;
	int                         stopped;
	uint32_t                    rollover_offset;
	uint32_t                    last_sync_gpt;
	u64                         last_sync_jiffies;
};
enum {
	MSM_CLOCK_GPT,
	MSM_CLOCK_DGT,
};
static struct msm_clock msm_clocks[];
static struct msm_clock *msm_active_clock;
static void (*msm_timer_sync_timeout)(void);

struct msm_timer_sync_data_t {
	struct msm_clock *clock;
	uint32_t         timeout;
	int              exit_sleep;
};

static irqreturn_t msm_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = dev_id;
	evt->event_handler(evt);
	return IRQ_HANDLED;
}

static uint32_t msm_read_timer_count(struct msm_clock *clock)
{
	uint32_t t1, t2;
	int loop_count = 0;

	t1 = readl(clock->regbase + TIMER_COUNT_VAL);
	if (!(clock->flags & MSM_CLOCK_FLAGS_UNSTABLE_COUNT))
		return t1;
	while (1) {
		t2 = readl(clock->regbase + TIMER_COUNT_VAL);
		if (t1 == t2)
			return t1;
		if (loop_count++ > 10) {
			printk(KERN_ERR "msm_read_timer_count timer %s did not"
			       "stabilize %u != %u\n", clock->clockevent.name,
			       t2, t1);
			return t2;
		}
		t1 = t2;
	}
}

static cycle_t msm_gpt_read(void)
{
	struct msm_clock *clock = &msm_clocks[MSM_CLOCK_GPT];
	if (clock->stopped)
		return clock->stopped_tick;
	else
		return msm_read_timer_count(clock) + clock->sleep_offset;
}

static cycle_t msm_dgt_read(void)
{
	struct msm_clock *clock = &msm_clocks[MSM_CLOCK_DGT];
	if (clock->stopped)
		return clock->stopped_tick >> MSM_DGT_SHIFT;
	return (msm_read_timer_count(clock) + clock->sleep_offset)
		>> MSM_DGT_SHIFT;
}

static int msm_timer_set_next_event(unsigned long cycles,
				    struct clock_event_device *evt)
{
	int i;
	struct msm_clock *clock;
	uint32_t now;
	uint32_t alarm;
	int late;

	clock = container_of(evt, struct msm_clock, clockevent);
	now = msm_read_timer_count(clock);
	alarm = now + (cycles << clock->shift);
	if (clock->flags & MSM_CLOCK_FLAGS_ODD_MATCH_WRITE)
		while (now == clock->last_set)
			now = msm_read_timer_count(clock);
	writel(alarm, clock->regbase + TIMER_MATCH_VAL);
	if (clock->flags & MSM_CLOCK_FLAGS_DELAYED_WRITE_POST) {
		/* read the counter four extra times to make sure write posts
		   before reading the time */
		for (i = 0; i < 4; i++)
			readl(clock->regbase + TIMER_COUNT_VAL);
	}
	now = msm_read_timer_count(clock);
	clock->last_set = now;
	clock->alarm_vtime = alarm + clock->sleep_offset;
	late = now - alarm;
	if (late >= (int)(-clock->write_delay << clock->shift) && late < DGT_HZ*5) {
		static int print_limit = 10;
		if (print_limit > 0) {
			print_limit--;
			printk(KERN_NOTICE "msm_timer_set_next_event(%lu) "
			       "clock %s, alarm already expired, now %x, "
			       "alarm %x, late %d%s\n",
			       cycles, clock->clockevent.name, now, alarm, late,
			       print_limit ? "" : " stop printing");
		}
		return -ETIME;
	}
	return 0;
}

static void msm_timer_set_mode(enum clock_event_mode mode,
			      struct clock_event_device *evt)
{
	struct msm_clock *gpt_clk = &msm_clocks[MSM_CLOCK_GPT];
	struct msm_clock *clock;
	unsigned long irq_flags;

	clock = container_of(evt, struct msm_clock, clockevent);
	local_irq_save(irq_flags);

	switch (mode) {
	case CLOCK_EVT_MODE_RESUME:
	case CLOCK_EVT_MODE_PERIODIC:
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		clock->stopped = 0;
		clock->sleep_offset = -msm_read_timer_count(clock) +
				clock->stopped_tick;
		msm_active_clock = clock;
		writel(TIMER_ENABLE_EN, clock->regbase + TIMER_ENABLE);
		if (clock != gpt_clk)
			writel(TIMER_ENABLE_EN,
				gpt_clk->regbase + TIMER_ENABLE);
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
		msm_active_clock = NULL;
		clock->in_sync = 0;
		clock->stopped = 1;
		clock->stopped_tick = msm_read_timer_count(clock) +
					clock->sleep_offset;
		writel(0, clock->regbase + TIMER_ENABLE);
		if (clock != gpt_clk) {
			gpt_clk->in_sync = 0;
			writel(0, gpt_clk->regbase + TIMER_ENABLE);
		}
		break;
	}
	local_irq_restore(irq_flags);
}

/*
 * Retrieve the cycle count from sclk and optionally synchronize local clock
 * with the sclk value.
 *
 * time_start and time_expired are callbacks that must be specified.  The
 * protocol uses them to detect timeout.  The update callback is optional.
 * If not NULL, update will be called so that it can update local clock.
 *
 * The function does not use the argument data directly; it passes data to
 * the callbacks.
 *
 * Return value:
 *      0: the operation failed
 *      >0: the slow clock value after time-sync
 */
#if defined(CONFIG_MSM_N_WAY_SMSM)
static uint32_t msm_timer_do_sync_to_sclk(
	void (*time_start)(struct msm_timer_sync_data_t *data),
	bool (*time_expired)(struct msm_timer_sync_data_t *data),
	void (*update)(struct msm_timer_sync_data_t *, uint32_t, uint32_t),
	struct msm_timer_sync_data_t *data)
{
	uint32_t *smem_clock;
	uint32_t smem_clock_val;
	uint32_t state;

	smem_clock = smem_alloc(SMEM_SMEM_SLOW_CLOCK_VALUE, sizeof(uint32_t));
	if (smem_clock == NULL) {
		printk(KERN_ERR "no smem clock\n");
		return 0;
	}

	state = smsm_get_state(SMSM_MODEM_STATE);
	if ((state & SMSM_INIT) == 0) {
		printk(KERN_ERR "smsm not initialized\n");
		return 0;
	}

	time_start(data);
	while ((state = smsm_get_state(SMSM_TIME_MASTER_DEM)) &
		MASTER_TIME_PENDING) {
		if (time_expired(data)) {
			printk(KERN_EMERG "get_smem_clock: timeout 1 still "
				"invalid state %x\n", state);
			msm_timer_sync_timeout();
		}
	}

	smsm_change_state(SMSM_APPS_DEM, SLAVE_TIME_POLL | SLAVE_TIME_INIT,
		SLAVE_TIME_REQUEST);

	time_start(data);
	while (!((state = smsm_get_state(SMSM_TIME_MASTER_DEM)) &
		MASTER_TIME_PENDING)) {
		if (time_expired(data)) {
			printk(KERN_EMERG "get_smem_clock: timeout 2 still "
				"invalid state %x\n", state);
			msm_timer_sync_timeout();
		}
	}

	smsm_change_state(SMSM_APPS_DEM, SLAVE_TIME_REQUEST, SLAVE_TIME_POLL);

	time_start(data);
	do {
		smem_clock_val = *smem_clock;
	} while (smem_clock_val == 0 && !time_expired(data));

	state = smsm_get_state(SMSM_TIME_MASTER_DEM);

	if (smem_clock_val) {
		if (update != NULL)
			update(data, smem_clock_val, SCLK_HZ);

		if (msm_timer_debug_mask & MSM_TIMER_DEBUG_SYNC)
			printk(KERN_INFO
				"get_smem_clock: state %x clock %u\n",
				state, smem_clock_val);
	} else {
		printk(KERN_EMERG
			"get_smem_clock: timeout state %x clock %u\n",
			state, smem_clock_val);
		msm_timer_sync_timeout();
	}

	smsm_change_state(SMSM_APPS_DEM, SLAVE_TIME_REQUEST | SLAVE_TIME_POLL,
		SLAVE_TIME_INIT);
	return smem_clock_val;
}
#else /* CONFIG_MSM_N_WAY_SMSM */
static uint32_t msm_timer_do_sync_to_sclk(
	void (*time_start)(struct msm_timer_sync_data_t *data),
	bool (*time_expired)(struct msm_timer_sync_data_t *data),
	void (*update)(struct msm_timer_sync_data_t *, uint32_t, uint32_t),
	struct msm_timer_sync_data_t *data)
{
	uint32_t *smem_clock;
	uint32_t smem_clock_val;
	uint32_t last_state;
	uint32_t state;

	smem_clock = smem_alloc(SMEM_SMEM_SLOW_CLOCK_VALUE,
				sizeof(uint32_t));

	if (smem_clock == NULL) {
		printk(KERN_ERR "no smem clock\n");
		return 0;
	}

	last_state = state = smsm_get_state(SMSM_MODEM_STATE);
	smem_clock_val = *smem_clock;
	if (smem_clock_val) {
		printk(KERN_INFO "get_smem_clock: invalid start state %x "
			"clock %u\n", state, smem_clock_val);
		smsm_change_state(SMSM_APPS_STATE,
				  SMSM_TIMEWAIT, SMSM_TIMEINIT);

		time_start(data);
		while (*smem_clock != 0 && !time_expired(data))
			;

		smem_clock_val = *smem_clock;
		if (smem_clock_val) {
			printk(KERN_EMERG "get_smem_clock: timeout still "
				"invalid state %x clock %u\n",
				state, smem_clock_val);
			msm_timer_sync_timeout();
		}
	}

	time_start(data);
	smsm_change_state(SMSM_APPS_STATE, SMSM_TIMEINIT, SMSM_TIMEWAIT);
	do {
		smem_clock_val = *smem_clock;
		state = smsm_get_state(SMSM_MODEM_STATE);
		if (state != last_state) {
			last_state = state;
			if (msm_timer_debug_mask & MSM_TIMER_DEBUG_SYNC)
				printk(KERN_INFO
					"get_smem_clock: state %x clock %u\n",
					state, smem_clock_val);
		}
	} while (smem_clock_val == 0 && !time_expired(data));

	if (smem_clock_val) {
		if (update != NULL)
			update(data, smem_clock_val, SCLK_HZ);
	} else {
		printk(KERN_EMERG
			"get_smem_clock: timeout state %x clock %u\n",
			state, smem_clock_val);
		msm_timer_sync_timeout();
	}

	smsm_change_state(SMSM_APPS_STATE, SMSM_TIMEWAIT, SMSM_TIMEINIT);
	return smem_clock_val;
}
#endif /* CONFIG_MSM_N_WAY_SMSM */

/*
 * Callback function that initializes the timeout value.
 */
static void msm_timer_sync_to_sclk_time_start(
	struct msm_timer_sync_data_t *data)
{
	/* approx 2 seconds */
	uint32_t delta = data->clock->freq << data->clock->shift << 1;
	data->timeout = msm_read_timer_count(data->clock) + delta;
}

/*
 * Callback function that checks the timeout.
 */
static bool msm_timer_sync_to_sclk_time_expired(
	struct msm_timer_sync_data_t *data)
{
	uint32_t delta = msm_read_timer_count(data->clock) - data->timeout;
	return ((int32_t) delta) > 0;
}

/*
 * Callback function that updates local clock from the specified source clock
 * value and frequency.
 */
static void msm_timer_sync_update(struct msm_timer_sync_data_t *data,
	uint32_t src_clk_val, uint32_t src_clk_freq)
{
	struct msm_clock *dst_clk = data->clock;
	uint32_t dst_clk_val = msm_read_timer_count(dst_clk);
	uint32_t new_offset;

	if ((dst_clk->freq << dst_clk->shift) == src_clk_freq) {
		new_offset = src_clk_val - dst_clk_val;
	} else {
		uint64_t temp;

		/* separate multiplication and division steps to reduce
		   rounding error */
		temp = src_clk_val;
		temp *= dst_clk->freq << dst_clk->shift;
		do_div(temp, src_clk_freq);

		new_offset = (uint32_t)(temp) - dst_clk_val;
	}

	if (dst_clk->sleep_offset + dst_clk->non_sleep_offset != new_offset) {
		if (data->exit_sleep)
			dst_clk->sleep_offset =
				new_offset - dst_clk->non_sleep_offset;
		else
			dst_clk->non_sleep_offset =
				new_offset - dst_clk->sleep_offset;

		if (msm_timer_debug_mask & MSM_TIMER_DEBUG_SYNC)
			printk(KERN_INFO "sync clock %s: "
				"src %u, new offset %u + %u\n",
				dst_clk->clocksource.name, src_clk_val,
				dst_clk->sleep_offset,
				dst_clk->non_sleep_offset);
	}
}

/*
 * Synchronize GPT clock with sclk.
 */
static void msm_timer_sync_gpt_to_sclk(int exit_sleep)
{
	struct msm_clock *gpt_clk = &msm_clocks[MSM_CLOCK_GPT];
	struct msm_timer_sync_data_t data;
	uint32_t ret;

	if (gpt_clk->in_sync)
		return;

	data.clock = gpt_clk;
	data.timeout = 0;
	data.exit_sleep = exit_sleep;

	ret = msm_timer_do_sync_to_sclk(
		msm_timer_sync_to_sclk_time_start,
		msm_timer_sync_to_sclk_time_expired,
		msm_timer_sync_update,
		&data);

	if (ret)
		gpt_clk->in_sync = 1;
}

/*
 * Synchronize clock with GPT clock.
 */
static void msm_timer_sync_to_gpt(struct msm_clock *clock, int exit_sleep)
{
	struct msm_clock *gpt_clk = &msm_clocks[MSM_CLOCK_GPT];
	struct msm_timer_sync_data_t data;
	uint32_t gpt_clk_val;
	u64 gpt_period = (1ULL << 32) * HZ / GPT_HZ;
	u64 now = get_jiffies_64();

	BUG_ON(clock == gpt_clk);

	if (clock->in_sync &&
		(now - clock->last_sync_jiffies < (gpt_period >> 1)))
		return;

	gpt_clk_val = msm_read_timer_count(gpt_clk)
		+ gpt_clk->sleep_offset + gpt_clk->non_sleep_offset;

	if (exit_sleep && gpt_clk_val < clock->last_sync_gpt)
		clock->non_sleep_offset -= clock->rollover_offset;

	data.clock = clock;
	data.timeout = 0;
	data.exit_sleep = exit_sleep;

	msm_timer_sync_update(&data, gpt_clk_val, GPT_HZ);

	clock->in_sync = 1;
	clock->last_sync_gpt = gpt_clk_val;
	clock->last_sync_jiffies = now;
}

static void msm_timer_reactivate_alarm(struct msm_clock *clock)
{
	long alarm_delta = clock->alarm_vtime - clock->sleep_offset -
		msm_read_timer_count(clock);
	alarm_delta >>= clock->shift;
	if (alarm_delta < (long)clock->write_delay + 4)
		alarm_delta = clock->write_delay + 4;
	while (msm_timer_set_next_event(alarm_delta, &clock->clockevent))
		;
}

int64_t msm_timer_enter_idle(void)
{
	struct msm_clock *gpt_clk = &msm_clocks[MSM_CLOCK_GPT];
	struct msm_clock *clock = msm_active_clock;
	uint32_t alarm;
	uint32_t count;
	int32_t delta;

	BUG_ON(clock != &msm_clocks[MSM_CLOCK_GPT] &&
		clock != &msm_clocks[MSM_CLOCK_DGT]);

	msm_timer_sync_gpt_to_sclk(0);
	if (clock != gpt_clk)
		msm_timer_sync_to_gpt(clock, 0);

	count = msm_read_timer_count(clock);
	if (clock->stopped++ == 0)
		clock->stopped_tick = count + clock->sleep_offset;
	alarm = readl(clock->regbase + TIMER_MATCH_VAL);
	delta = alarm - count;
	if (delta <= -(int32_t)((clock->freq << clock->shift) >> 10)) {
		/* timer should have triggered 1ms ago */
		printk(KERN_ERR "msm_timer_enter_idle: timer late %d, "
			"reprogram it\n", delta);
		msm_timer_reactivate_alarm(clock);
	}
	if (delta <= 0)
		return 0;
	return cyc2ns(&clock->clocksource, (alarm - count) >> clock->shift);
}

void msm_timer_exit_idle(int low_power)
{
	struct msm_clock *gpt_clk = &msm_clocks[MSM_CLOCK_GPT];
	struct msm_clock *clock = msm_active_clock;
	uint32_t enabled;

	BUG_ON(clock != &msm_clocks[MSM_CLOCK_GPT] &&
		clock != &msm_clocks[MSM_CLOCK_DGT]);

	if (!low_power)
		goto exit_idle_exit;

	enabled = readl(gpt_clk->regbase + TIMER_ENABLE) & TIMER_ENABLE_EN;
	if (!enabled)
		writel(TIMER_ENABLE_EN, gpt_clk->regbase + TIMER_ENABLE);

#if defined(CONFIG_ARCH_MSM_SCORPION)
	gpt_clk->in_sync = 0;
#else
	gpt_clk->in_sync = gpt_clk->in_sync && enabled;
#endif
	msm_timer_sync_gpt_to_sclk(1);

	if (clock == gpt_clk)
		goto exit_idle_alarm;

	enabled = readl(clock->regbase + TIMER_ENABLE) & TIMER_ENABLE_EN;
	if (!enabled)
		writel(TIMER_ENABLE_EN, clock->regbase + TIMER_ENABLE);

#if defined(CONFIG_ARCH_MSM_SCORPION)
	clock->in_sync = 0;
#else
	clock->in_sync = clock->in_sync && enabled;
#endif
	msm_timer_sync_to_gpt(clock, 1);

exit_idle_alarm:
	msm_timer_reactivate_alarm(clock);

exit_idle_exit:
	clock->stopped--;
}

/*
 * Callback function that initializes the timeout value.
 */
static void msm_timer_get_sclk_time_start(
	struct msm_timer_sync_data_t *data)
{
	data->timeout = 1000000;
}

/*
 * Callback function that checks the timeout.
 */
static bool msm_timer_get_sclk_time_expired(
	struct msm_timer_sync_data_t *data)
{
	return --data->timeout <= 0;
}

/*
 * Retrieve the cycle count from the sclk and convert it into
 * nanoseconds.
 *
 * On exit, if period is not NULL, it contains the period of the
 * sclk in nanoseconds, i.e. how long the cycle count wraps around.
 *
 * Return value:
 *      0: the operation failed; period is not set either
 *      >0: time in nanoseconds
 */
int64_t msm_timer_get_sclk_time(int64_t *period)
{
	struct msm_timer_sync_data_t data;
	uint32_t clock_value;
	int64_t tmp;

	memset(&data, 0, sizeof(data));

	clock_value = msm_timer_do_sync_to_sclk(
		msm_timer_get_sclk_time_start,
		msm_timer_get_sclk_time_expired,
		NULL,
		&data);

	if (!clock_value)
		return 0;

	if (period) {
		tmp = 1LL << 32;
		tmp = tmp * NSEC_PER_SEC / SCLK_HZ;
		*period = tmp;
	}

	tmp = (int64_t)clock_value;
	tmp = tmp * NSEC_PER_SEC / SCLK_HZ;
	return tmp;
}

int __init msm_timer_init_time_sync(void (*timeout)(void))
{
#if defined(CONFIG_MSM_N_WAY_SMSM)
	int ret = smsm_change_intr_mask(SMSM_TIME_MASTER_DEM, 0xFFFFFFFF, 0);

	if (ret) {
		printk(KERN_ERR	"%s: failed to clear interrupt mask, %d\n",
			__func__, ret);
		return ret;
	}

	smsm_change_state(SMSM_APPS_DEM,
		SLAVE_TIME_REQUEST | SLAVE_TIME_POLL, SLAVE_TIME_INIT);
#endif

	BUG_ON(timeout == NULL);
	msm_timer_sync_timeout = timeout;

	return 0;
}

unsigned long long sched_clock(void)
{
	static cycle_t saved_ticks;
	static int saved_ticks_valid;
	static unsigned long long base;
	static unsigned long long last_result;

	unsigned long irq_flags;
	static cycle_t last_ticks;
	cycle_t ticks;
	static unsigned long long result;
	struct clocksource *cs;
	struct msm_clock *clock = msm_active_clock;

	local_irq_save(irq_flags);
	if (clock) {
		cs = &clock->clocksource;

		last_ticks = saved_ticks;
		saved_ticks = ticks = cs->read();
		if (!saved_ticks_valid) {
			saved_ticks_valid = 1;
			last_ticks = ticks;
			base -= cyc2ns(cs, ticks);
		}
		if (ticks < last_ticks) {
			base += cyc2ns(cs, cs->mask);
			base += cyc2ns(cs, 1);
		}
		last_result = result = cyc2ns(cs, ticks) + base;
	} else {
		base = result = last_result;
		saved_ticks_valid = 0;
	}
	local_irq_restore(irq_flags);
	return result; 
}

#ifdef CONFIG_MSM7X00A_USE_GP_TIMER
	#define DG_TIMER_RATING 100
#else
	#define DG_TIMER_RATING 300
#endif

static struct msm_clock msm_clocks[] = {
	[MSM_CLOCK_GPT] = {
		.clockevent = {
			.name           = "gp_timer",
			.features       = CLOCK_EVT_FEAT_ONESHOT,
			.shift          = 32,
			.rating         = 200,
			.set_next_event = msm_timer_set_next_event,
			.set_mode       = msm_timer_set_mode,
		},
		.clocksource = {
			.name           = "gp_timer",
			.rating         = 200,
			.read           = msm_gpt_read,
			.mask           = CLOCKSOURCE_MASK(32),
			.shift          = 17,
			.flags          = CLOCK_SOURCE_IS_CONTINUOUS,
		},
		.irq = {
			.name    = "gp_timer",
			.flags   = IRQF_DISABLED | IRQF_TIMER |
				   IRQF_TRIGGER_RISING,
			.handler = msm_timer_interrupt,
			.dev_id  = &msm_clocks[0].clockevent,
			.irq     = INT_GP_TIMER_EXP
		},
		.regbase = MSM_GPT_BASE,
		.freq = GPT_HZ,
		.flags   =
			MSM_CLOCK_FLAGS_UNSTABLE_COUNT |
			MSM_CLOCK_FLAGS_ODD_MATCH_WRITE |
			MSM_CLOCK_FLAGS_DELAYED_WRITE_POST,
		.write_delay = 9,
	},
	[MSM_CLOCK_DGT] = {
		.clockevent = {
			.name           = "dg_timer",
			.features       = CLOCK_EVT_FEAT_ONESHOT,
			.shift          = 32 + MSM_DGT_SHIFT,
			.rating         = DG_TIMER_RATING,
			.set_next_event = msm_timer_set_next_event,
			.set_mode       = msm_timer_set_mode,
		},
		.clocksource = {
			.name           = "dg_timer",
			.rating         = DG_TIMER_RATING,
			.read           = msm_dgt_read,
			.mask           = CLOCKSOURCE_MASK((32-MSM_DGT_SHIFT)),
			.shift          = 24 - MSM_DGT_SHIFT,
			.flags          = CLOCK_SOURCE_IS_CONTINUOUS,
		},
		.irq = {
			.name    = "dg_timer",
			.flags   = IRQF_DISABLED | IRQF_TIMER |
				   IRQF_TRIGGER_RISING,
			.handler = msm_timer_interrupt,
			.dev_id  = &msm_clocks[1].clockevent,
			.irq     = INT_DEBUG_TIMER_EXP
		},
		.regbase = MSM_DGT_BASE,
		.freq = DGT_HZ >> MSM_DGT_SHIFT,
		.shift = MSM_DGT_SHIFT,
		.write_delay = 2,
	}
};

static void __init msm_timer_init(void)
{
	int i;
	int res;

	for (i = 0; i < ARRAY_SIZE(msm_clocks); i++) {
		struct msm_clock *clock = &msm_clocks[i];
		struct clock_event_device *ce = &clock->clockevent;
		struct clocksource *cs = &clock->clocksource;
		writel(0, clock->regbase + TIMER_ENABLE);
		writel(~0, clock->regbase + TIMER_MATCH_VAL);

		if ((clock->freq << clock->shift) == GPT_HZ) {
			clock->rollover_offset = 0;
		} else {
			uint64_t temp;

			temp = clock->freq << clock->shift;
			temp <<= 32;
			temp /= GPT_HZ;

			clock->rollover_offset = (uint32_t) temp;
		}

		ce->mult = div_sc(clock->freq, NSEC_PER_SEC, ce->shift);
		/* allow at least 10 seconds to notice that the timer wrapped */
		ce->max_delta_ns =
			clockevent_delta2ns(0xf0000000 >> clock->shift, ce);
		/* ticks gets rounded down by one */
		ce->min_delta_ns =
			clockevent_delta2ns(clock->write_delay + 4, ce);
		ce->cpumask = cpumask_of(0);

		cs->mult = clocksource_hz2mult(clock->freq, cs->shift);
		res = clocksource_register(cs);
		if (res)
			printk(KERN_ERR "msm_timer_init: clocksource_register "
			       "failed for %s\n", cs->name);

		res = setup_irq(clock->irq.irq, &clock->irq);
		if (res)
			printk(KERN_ERR "msm_timer_init: setup_irq "
			       "failed for %s\n", cs->name);

		clockevents_register_device(ce);
	}
}

struct sys_timer msm_timer = {
	.init = msm_timer_init
};
