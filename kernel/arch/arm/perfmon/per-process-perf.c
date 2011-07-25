/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
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

/*
per-process_perf
DESCRIPTION
Capture the processor performances registers when the process context
switches.  The /proc file system is used to control and access the results
of the performance counters.

Each time a process is context switched, the performance counters for
the Snoop Control Unit and the standard ARM counters are set according
to the values stored for that process.

The events to capture per process are set in the /proc/ppPerf/settings
directory.

EXTERNALIZED FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS
Detail how to initialize and use this service.  The sequencing aspect
is only needed if the order of operations is important.
*/

/*
INCLUDE FILES FOR MODULE
*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/sysrq.h>
#include <linux/time.h>
#include "linux/proc_fs.h"
#include "linux/kernel_stat.h"
#include <asm/thread_notify.h>
#include "asm/uaccess.h"
#include "cp15_registers.h"
#include <asm/perftypes.h>
#include "per-axi.h"
#include "perf.h"

/*
DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains definitions for constants, macros, types, variables
and other items needed by this module.
*/

/*
Constant / Define Declarations
*/

#define PERF_MON_PROCESS_NUM 0x400
#define PERF_MON_PROCESS_MASK (PERF_MON_PROCESS_NUM-1)
#define PP_MAX_PROC_ENTRIES 32

/*
 * The entry is locked and is not to be replaced.
 */
#define PERF_ENTRY_LOCKED (1<<0)
#define PERF_NOT_FIRST_TIME   (1<<1)
#define PERF_EXITED (1<<2)
#define PERF_AUTOLOCK (1<<3)

#define IS_LOCKED(p) (p->flags & PERF_ENTRY_LOCKED)

#define PERF_NUM_MONITORS 4

#define L1_EVENTS_0    0
#define L1_EVENTS_1    1
#define L2_EVENTS_0  2
#define L2_EVENTS_1  3

#define PM_CYCLE_OVERFLOW_MASK 0x80000000

#define PM_START_ALL() do {\
	if (pm_global) \
		pmStartAll();\
	} while (0);
#define PM_STOP_ALL() do {\
	if (pm_global)\
		pmStopAll();\
	} while (0);
#define PM_RESET_ALL() do {\
	if (pm_global)\
		pmResetAll();\
	} while (0);

/*
  Type Declarations
*/
struct per_process_perf_mon_type{
  unsigned long long  cycles;
  unsigned long long  counts[PERF_NUM_MONITORS];
  unsigned long  control;
  unsigned long  index[PERF_NUM_MONITORS];
  unsigned long  pid;
  struct proc_dir_entry *proc;
  unsigned long  swaps;
  unsigned short flags;
  char           *pidName;
  unsigned long lpm0evtyper;
  unsigned long lpm1evtyper;
  unsigned long lpm2evtyper;
  unsigned long l2lpmevtyper;
  unsigned long vlpmevtyper;
};

/*
  Local Object Definitions
*/
struct per_process_perf_mon_type perf_mons[PERF_MON_PROCESS_NUM];
struct proc_dir_entry *proc_dir;
struct proc_dir_entry *settings_dir;
struct proc_dir_entry *values_dir;
struct proc_dir_entry *axi_dir;
struct proc_dir_entry *axi_settings_dir;
struct proc_dir_entry *axi_results_dir;

unsigned long pp_enabled;
unsigned long pp_settings_valid = -1;
unsigned long pp_auto_lock;
unsigned long pp_set_pid;
signed long pp_clear_pid = -1;
unsigned long per_proc_event[PERF_NUM_MONITORS];
unsigned long dbg_flags;
unsigned long pp_lpm0evtyper;
unsigned long pp_lpm1evtyper;
unsigned long pp_lpm2evtyper;
unsigned long pp_l2lpmevtyper;
unsigned long pp_vlpmevtyper;
unsigned long pm_stop_for_interrupts;
unsigned long pm_global;   /* track all, not process based */
unsigned long pm_global_enable;
unsigned long pm_remove_pid;

unsigned long pp_proc_entry_index;
char *per_process_proc_names[PP_MAX_PROC_ENTRIES];

unsigned int axi_swaps;
#define MAX_AXI_SWAPS	10
int first_switch = 1;
/*
  Forward Declarations
*/

/*
Function Definitions
*/

/*
FUNCTION  per_process_find

DESCRIPTION
  Find the per process information based on the process id (pid) passed.
  This is a simple mask based on the number of entries stored in the
  static array

DEPENDENCIES

RETURN VALUE
  Pointer to the per process data
SIDE EFFECTS

*/
struct per_process_perf_mon_type *per_process_find(unsigned long pid)
{
  return &perf_mons[pid & PERF_MON_PROCESS_MASK];
}

/*
FUNCTION  per_process_get_name

DESCRIPTION
  Retreive the name of the performance counter based on the table and
  index passed.  We have two different sets of performance counters so
  different table need to be used.

DEPENDENCIES

RETURN VALUE
  Pointer to char string with the name of the event or "BAD"
  Never returns NULL or a bad pointer.

SIDE EFFECTS
*/
char *per_process_get_name(unsigned long index)
{
  return pm_find_event_name(index);
}

/*
FUNCTION per_process_results_read

DESCRIPTION
  Print out the formatted results from the process id read.  Event names
  and counts are printed.

DEPENDENCIES

RETURN VALUE

SIDE EFFECTS
*/
int per_process_results_read(char *page, char **start, off_t off, int count,
   int *eof, void *data)
{
  struct per_process_perf_mon_type *p =
	(struct per_process_perf_mon_type *)data;

  return sprintf(page, "pid:%lu one:%s:%llu two:%s:%llu three:%s:%llu \
	four:%s:%llu cycles:%llu swaps:%lu\n",
     p->pid,
     per_process_get_name(p->index[0]), p->counts[0],
     per_process_get_name(p->index[1]), p->counts[1],
     per_process_get_name(p->index[2]), p->counts[2],
     per_process_get_name(p->index[3]), p->counts[3],
     p->cycles, p->swaps);

}

/*
FUNCTION  per_process_results_write

DESCRIPTION
  Allow some control over the results.  If the user forgets to autolock or
  wants to unlock the results so they will be deleted, then this is
  where it is processed.

  For example, to unlock process 23
  echo "unlock" > 23

DEPENDENCIES

RETURN VALUE
  Number of characters used (all of them!)

SIDE EFFECTS
*/
int per_process_results_write(struct file *file, const char *buff,
    unsigned long cnt, void *data)
{
  char *newbuf;
  struct per_process_perf_mon_type *p =
	(struct per_process_perf_mon_type *)data;

	if (p == 0)
		return cnt;
	/*
	* Alloc the user data in kernel space. and then copy user to kernel
	*/
	newbuf = kmalloc(cnt + 1, GFP_KERNEL);
	if (0 == newbuf)
		return cnt;
	if (copy_from_user(newbuf, buff, cnt) != 0) {
		printk(KERN_INFO "%s copy_from_user failed\n", __func__);
		return cnt;
	}

	if (0 == strcmp("lock", newbuf))
		p->flags |= PERF_ENTRY_LOCKED;
	else if (0 == strcmp("unlock", newbuf))
		p->flags &= ~PERF_ENTRY_LOCKED;
	else if (0 == strcmp("auto", newbuf))
		p->flags |= PERF_AUTOLOCK;
	else if (0 == strcmp("autoun", newbuf))
		p->flags &= ~PERF_AUTOLOCK;

	return cnt;
}

/*
FUNCTION  perProcessCreateResults

DESCRIPTION
  Create the results /proc file if the system parameters allow it...
DEPENDENCIES

RETURN VALUE

SIDE EFFECTS
*/
void per_process_create_results_proc(struct per_process_perf_mon_type *p)
{

	if (0 == p->pidName)
		p->pidName = kmalloc(12, GFP_KERNEL);
	if (0 == p->pidName)
		return;
	sprintf(p->pidName, "%ld", p->pid);

	if (0 == p->proc) {
		p->proc = create_proc_entry(p->pidName, 0777, values_dir);
		if (0 == p->proc)
			return;
	} else {
		p->proc->name = p->pidName;
	}

	p->proc->read_proc = per_process_results_read;
	p->proc->write_proc = per_process_results_write;
	p->proc->data = (void *)p;
}

/*
FUNCTION per_process_swap_out

DESCRIPTION
  Store the counters from the process that is about to swap out.  We take
  the old counts and add them to the current counts in the perf registers.
  Before the new process is swapped in, the counters are reset.

DEPENDENCIES

RETURN VALUE

SIDE EFFECTS
*/
void per_process_swap_out(struct per_process_perf_mon_type *p)
{
	int i;
	unsigned long overflow;

	RCP15_PMOVSR(overflow);

	if (pp_enabled)
		p->swaps++;
	p->cycles += pm_get_cycle_count();
	if (overflow & PM_CYCLE_OVERFLOW_MASK)
		p->cycles += 0xFFFFFFFF;

	for (i = 0; i < PERF_NUM_MONITORS; i++) {
		p->counts[i] += pm_get_count(i);
		if (overflow & (1<<i))
			p->counts[i] += 0xFFFFFFFF;
	}
}

/*
FUNCTION per_process_remove_manual

DESCRIPTION
  Remove an entry from the results directory if the flags allow this.
  When not enbled or the entry is locked, the values/results will
  not be removed.

DEPENDENCIES

RETURN VALUE

SIDE EFFECTS
*/
void per_process_remove_manual(unsigned long pid)
{
	struct per_process_perf_mon_type *p = per_process_find(pid);

	/*
	* Check all of the flags to see if we can remove this one
	* Then mark as not used
	*/
	if (0 == p)
		return;
	p->pid = (0xFFFFFFFF);

	/*
	* Remove the proc entry.
	*/
	if (p->proc)
		remove_proc_entry(p->pidName, values_dir);
	kfree(p->pidName);

	/*
	* Clear them out...and ensure the pid is invalid
	*/
	memset(p, 0, sizeof *p);
	p->pid = 0xFFFFFFFF;
	pm_remove_pid = -1;
}

/*
* Remove called when a process exits...
*/
void _per_process_remove(unsigned long pid) {}

/*
FUNCTION  per_process_initialize

DESCRIPTION
Initialize performance collection information for a new process.

DEPENDENCIES

RETURN VALUE

SIDE EFFECTS
May create a new proc entry
*/
void per_process_initialize(struct per_process_perf_mon_type *p,
				unsigned long pid)
{
	int i;

	/*
	* See if this is the pid we are interested in...
	*/
	if (pp_settings_valid == -1)
		return;
	if ((pp_set_pid != pid) && (pp_set_pid != 0))
		return;

	/*
	* Clear out the statistics table then insert this pid
	* We want to keep the proc entry and the name
	*/
	p->pid = pid;
	/*
	* Create a proc entry for this pid, then get the current event types and
	* store in data struct so when the process is switched in we can track
	* it.
	*/
	if (p->proc == 0)
		per_process_create_results_proc(p);
	p->cycles = 0;
	p->swaps = 0;
	/*
	* Set the per process data struct, but not the monitors until later...
	* Init only happens with the user sets the SetPID variable to this pid
	* so we can load new values.
	*/
	for (i = 0; i < PERF_NUM_MONITORS; i++) {
		p->index[i] = per_proc_event[i];
		p->counts[i] = 0;
	}
	p->lpm0evtyper  = pp_lpm0evtyper;
	p->lpm1evtyper  = pp_lpm1evtyper;
	p->lpm2evtyper  = pp_lpm2evtyper;
	p->l2lpmevtyper = pp_l2lpmevtyper;
	p->vlpmevtyper  = pp_vlpmevtyper;
	/*
	* Reset pid and settings value
	*/
	pp_set_pid = -1;
	pp_settings_valid = -1;
}

/*
FUNCTION  per_process_swap_in

DESCRIPTION
  Called when a context switch is about to start this PID.
  We check to see if this process has an entry or not and create one
  if not locked...

DEPENDENCIES

RETURN VALUE

SIDE EFFECTS
*/
void per_process_swap_in(struct per_process_perf_mon_type *p_new,
				unsigned long pid)
{
	int i;

	/*
	* If the set proc variable == the current pid then init a new
	* entry...
	*/
	if (pp_set_pid == pid)
		per_process_initialize(p_new, pid);

	/*
	* setup the monitors for this process.
	*/
	for (i = 0; i < PERF_NUM_MONITORS; i++)
		pm_set_event(i, p_new->index[i]);
	pm_set_local_iu(p_new->lpm0evtyper);
	pm_set_local_xu(p_new->lpm1evtyper);
	pm_set_local_su(p_new->lpm2evtyper);
	pm_set_local_l2(p_new->l2lpmevtyper);

}

/*
FUNCTION  perProcessSwitch

DESCRIPTION
  Called during context switch.  Updates the counts on the process about to
  be swapped out and brings in the counters for the process about to be
  swapped in.

  All is dependant on the enabled and lock flags.

DEPENDENCIES

RETURN VALUE

SIDE EFFECTS
*/
void _per_process_switch(unsigned long old_pid, unsigned long new_pid)
{
	struct per_process_perf_mon_type *p_old, *p_new;

	if (pm_global_enable == 0)
		return;
	  pm_stop_all();
	/*
	* Always collect for 0, it collects for all.
	*/
	if (pp_enabled) {
		if (first_switch == 1) {
			per_process_initialize(&perf_mons[0], 0);
			first_switch = 0;
		}
		per_process_swap_out(&perf_mons[0]);
		per_process_swap_in(&perf_mons[0], 0);
	}

	p_old = per_process_find(old_pid);
	p_new = per_process_find(new_pid);

	/*
	* Clear the data collected so far for this process?
	*/
	if (pp_clear_pid != -1) {
		int i;
		struct per_process_perf_mon_type *p_clear =
			per_process_find(pp_clear_pid);
		if (p_clear) {
			p_clear->swaps = 0;
			p_clear->cycles = 0;
			for (i = 0; i < PERF_NUM_MONITORS; i++)
				p_clear->counts[i] = 0;
			printk(KERN_INFO "Clear Per Processor Stats for \
				PID:%ld\n", pp_clear_pid);
			pp_clear_pid = -1;
		}
	}

	/*
	* save the old counts to the old data struct, if the returned
	* ptr is NULL or the process id passed is not the same as the
	* process id in the data struct then don't update the data.
	*/
	if ((p_old) && (p_old->pid == old_pid))
		per_process_swap_out(p_old);
	/*
	* Setup the counters for the new process
	*/
	if (pp_set_pid == new_pid)
		per_process_initialize(p_new, new_pid);
	if (p_new->pid != 0)
		per_process_swap_in(p_new, new_pid);
	pm_reset_all();
	axi_swaps++;
	if (axi_swaps%pm_axi_info.refresh == 0) {
		if (pm_axi_info.clear == 1) {
			pm_axi_clear_cnts();
			pm_axi_info.clear = 0;
		}
		if (pm_axi_info.enable == 0)
			pm_axi_disable();
		else
			pm_axi_update_cnts();
		axi_swaps = 0;
	}
	if (0 == pp_enabled)
		return;
	pm_start_all();
}

/*
FUNCTION  pmInterruptIn

DESCRIPTION
  Called when an interrupt is being processed.  If the pmStopForInterrutps
  flag is non zero then we disable the counting of performance monitors.

DEPENDENCIES

RETURN VALUE

SIDE EFFECTS
*/
static int pm_interrupt_nesting_count;
static unsigned long pm_cycle_in, pm_cycle_out;
void _perf_mon_interrupt_in(void)
{
	if (pm_global_enable == 0)
		return;
	if (pm_stop_for_interrupts == 0)
		return;
	pm_interrupt_nesting_count++;  	/* Atomic */
	pm_stop_all();
	pm_cycle_in = pm_get_cycle_count();
}

/*
FUNCTION perfMonInterruptOut

DESCRIPTION
  Reenable performance monitor counting whn the nest count goes to zero
  provided the counting has been stoped

DEPENDENCIES

RETURN VALUE

SIDE EFFECTS
*/
void _perf_mon_interrupt_out(void)
{
	if (pm_global_enable == 0)
		return;
	if (pm_stop_for_interrupts == 0)
		return;
	--pm_interrupt_nesting_count;  /* Atomic?? */

	if (pm_interrupt_nesting_count <= 0) {
		pm_cycle_out = pm_get_cycle_count();
		if (pm_cycle_in != pm_cycle_out)
			printk(KERN_INFO "pmIn!=pmOut in:%lx out:%lx\n",
			pm_cycle_in, pm_cycle_out);
		if (pp_enabled)
			pm_start_all();
		pm_interrupt_nesting_count = 0;
	}
}

void per_process_do_global(unsigned long g)
{
	pm_global = g;

	if (pm_global == 1) {
		pm_stop_all();
		pm_reset_all();
		pp_set_pid = 0;
		per_process_swap_in(&perf_mons[0], 0);
		pm_start_all();
	} else {
		pm_stop_all();
	}
}

/*
FUNCTION   per_process_write

DESCRIPTION
  Generic routine to handle any of the settings /proc directory writes.

DEPENDENCIES

RETURN VALUE

SIDE EFFECTS
*/
int per_process_write(struct file *file, const char *buff,
    unsigned long cnt, void *data, const char *fmt)
{
	char *newbuf;
	unsigned long *d = (unsigned long *)data;

	/*
	* Alloc the user data in kernel space. and then copy user to kernel
	*/
	newbuf = kmalloc(cnt + 1, GFP_KERNEL);
	if (0 == newbuf)
		return -1;
	if (copy_from_user(newbuf, buff, cnt) != 0) {
		printk(KERN_INFO "%s copy_from_user failed\n", __func__);
	return cnt;
	}
	sscanf(newbuf, fmt, d);
	kfree(newbuf);

	/*
	* If this is a remove  command then do it now...
	*/
	if (d == &pm_remove_pid)
		per_process_remove_manual(*d);
	if (d == &pm_global)
		per_process_do_global(*d);
	return cnt;
}

int per_process_write_dec(struct file *file, const char *buff,
    unsigned long cnt, void *data)
{
	return per_process_write(file, buff, cnt, data, "%ld");
}

int per_process_write_hex(struct file *file, const char *buff,
    unsigned long cnt, void *data)
{
	return per_process_write(file, buff, cnt, data, "%lx");
}

/*
FUNCTION per_process_read

DESCRIPTION
  Generic read handler for the /proc settings directory.

DEPENDENCIES

RETURN VALUE
  Number of characters to output.

SIDE EFFECTS
*/
int per_process_read(char *page, char **start, off_t off, int count,
   int *eof, void *data)
{
	unsigned long *d = (unsigned long *)data;
	return sprintf(page, "%lx", *d);
}

int per_process_read_decimal(char *page, char **start, off_t off, int count,
   int *eof, void *data)
{
	unsigned long *d = (unsigned long *)data;
	return sprintf(page, "%ld", *d);
}

/*
FUNCTION  per_process_proc_entry

DESCRIPTION
  Create a generic entry for the /proc settings directory.

DEPENDENCIES

RETURN VALUE

SIDE EFFECTS
*/
void per_process_proc_entry(char *name, unsigned long *var,
    struct proc_dir_entry *d, int hex)
{
	struct proc_dir_entry *pe;

	pe = create_proc_entry(name, 0777, d);
	if (0 == pe)
		return;
	if (hex) {
		pe->read_proc = per_process_read;
		pe->write_proc = per_process_write_hex;
	} else {
		pe->read_proc = per_process_read_decimal;
		pe->write_proc = per_process_write_dec;
	}
	pe->data = (void *)var;

	if (pp_proc_entry_index >= PP_MAX_PROC_ENTRIES) {
		printk(KERN_INFO "PERF: proc entry overflow,\
		memleak on module unload occured");
	return;
	}
	per_process_proc_names[pp_proc_entry_index++] = name;
}

static int perfmon_notifier(struct notifier_block *self, unsigned long cmd,
	void *v)
{
	static int old_pid = -1;
	struct thread_info *thread = v;
	int current_pid;

	if (cmd != THREAD_NOTIFY_SWITCH)
		return old_pid;

	current_pid = thread->task->pid;
	if (old_pid != -1)
		_per_process_switch(old_pid, current_pid);
	old_pid = current_pid;
	return old_pid;
}

static struct notifier_block perfmon_notifier_block = {
	.notifier_call  = perfmon_notifier,
};

/*
FUNCTION  per_process_perf_init

DESCRIPTION
  Initialze the per process performance monitor variables and /proc space.

DEPENDENCIES

RETURN VALUE

SIDE EFFECTS
*/
int per_process_perf_init(void)
{
	pm_initialize();
	pm_axi_init();
	pm_axi_clear_cnts();
	proc_dir = proc_mkdir("ppPerf", NULL);
	values_dir = proc_mkdir("results", proc_dir);
	settings_dir = proc_mkdir("settings", proc_dir);
	per_process_proc_entry("enable", &pp_enabled, settings_dir, 1);
	per_process_proc_entry("valid", &pp_settings_valid, settings_dir, 1);
	per_process_proc_entry("setPID", &pp_set_pid, settings_dir, 0);
	per_process_proc_entry("clearPID", &pp_clear_pid, settings_dir, 0);
	per_process_proc_entry("event0", &per_proc_event[0], settings_dir, 1);
	per_process_proc_entry("event1", &per_proc_event[1], settings_dir, 1);
	per_process_proc_entry("event2", &per_proc_event[2], settings_dir, 1);
	per_process_proc_entry("event3", &per_proc_event[3], settings_dir, 1);
	per_process_proc_entry("debug", &dbg_flags, settings_dir, 1);
	per_process_proc_entry("autolock", &pp_auto_lock, settings_dir, 1);
	per_process_proc_entry("lpm0evtyper", &pp_lpm0evtyper, settings_dir, 1);
	per_process_proc_entry("lpm1evtyper", &pp_lpm1evtyper, settings_dir, 1);
	per_process_proc_entry("lpm2evtyper", &pp_lpm2evtyper, settings_dir, 1);
	per_process_proc_entry("l2lpmevtyper", &pp_l2lpmevtyper, settings_dir,
				1);
	per_process_proc_entry("vlpmevtyper", &pp_vlpmevtyper, settings_dir, 1);
	per_process_proc_entry("stopForInterrupts", &pm_stop_for_interrupts,
		settings_dir, 1);
	per_process_proc_entry("global", &pm_global, settings_dir, 1);
	per_process_proc_entry("globalEnable", &pm_global_enable, settings_dir,
				1);
	per_process_proc_entry("removePID", &pm_remove_pid, settings_dir, 0);

	axi_dir = proc_mkdir("axi", proc_dir);
	axi_settings_dir = proc_mkdir("settings", axi_dir);
	axi_results_dir = proc_mkdir("results", axi_dir);
	pm_axi_set_proc_entry("axi_enable", &pm_axi_info.enable,
		axi_settings_dir, 1);
	pm_axi_set_proc_entry("axi_clear", &pm_axi_info.clear, axi_settings_dir,
		0);
	pm_axi_set_proc_entry("axi_valid", &pm_axi_info.valid, axi_settings_dir,
		1);
	pm_axi_set_proc_entry("axi_sel_reg0", &pm_axi_info.sel_reg0,
		axi_settings_dir, 1);
	pm_axi_set_proc_entry("axi_sel_reg1", &pm_axi_info.sel_reg1,
		axi_settings_dir, 1);
	pm_axi_set_proc_entry("axi_ten_sel", &pm_axi_info.ten_sel_reg,
		axi_settings_dir, 1);
	pm_axi_set_proc_entry("axi_refresh", &pm_axi_info.refresh,
		axi_settings_dir, 1);
	pm_axi_get_cnt_proc_entry("axi_cnts", &axi_cnts, axi_results_dir, 0);

	memset(perf_mons, 0, sizeof(perf_mons));
	per_process_create_results_proc(&perf_mons[0]);
	thread_register_notifier(&perfmon_notifier_block);
	/*
	* Set the function pointers so the module can be activated.
	*/
	pp_interrupt_out_ptr = _perf_mon_interrupt_out;
	pp_interrupt_in_ptr  = _perf_mon_interrupt_in;
	pp_process_remove_ptr = _per_process_remove;
	pp_loaded = 1;
	pm_axi_info.refresh = 1;
	return 0;
}

/*
FUNCTION  per_process_perf_exit

DESCRIPTION
  Module exit functionm, clean up, renmove proc entries

DEPENDENCIES

RETURN VALUE

SIDE EFFECTS
  No more per process
*/
void per_process_perf_exit(void)
{
	unsigned long i;
	/*
	* Sert the function pointers to 0 so the functions will no longer
	* be invoked
	*/
	pp_loaded = 0;
	pp_interrupt_out_ptr = 0;
	pp_interrupt_in_ptr  = 0;
	pp_process_remove_ptr = 0;
	/*
	* Remove the results
	*/
	for (i = 0; i < PERF_MON_PROCESS_NUM; i++)
		per_process_remove_manual(perf_mons[i].pid);
	/*
	* Remove the proc entries in the settings dir
	*/
	i = 0;
	for (i = 0; i < pp_proc_entry_index; i++)
		remove_proc_entry(per_process_proc_names[i], settings_dir);

	/*remove proc axi files*/
	remove_proc_entry("axi_enable", axi_settings_dir);
	remove_proc_entry("axi_valid", axi_settings_dir);
	remove_proc_entry("axi_refresh", axi_settings_dir);
	remove_proc_entry("axi_clear", axi_settings_dir);
	remove_proc_entry("axi_sel_reg0", axi_settings_dir);
	remove_proc_entry("axi_sel_reg1", axi_settings_dir);
	remove_proc_entry("axi_ten_sel", axi_settings_dir);
	remove_proc_entry("axi_cnts", axi_results_dir);
	/*
	* Remove the directories
	*/
	remove_proc_entry("results", proc_dir);
	remove_proc_entry("settings", proc_dir);
	remove_proc_entry("results", axi_dir);
	remove_proc_entry("settings", axi_dir);
	remove_proc_entry("axi", proc_dir);
	remove_proc_entry("ppPerf", NULL);
	pm_free_irq();
	thread_unregister_notifier(&perfmon_notifier_block);
	pm_deinitialize();
}
