/* linux/include/mach/rpc_hsusb.h
 *
 * Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 *
 * All source code in this file is licensed under the following license except
 * where indicated.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can find it at http://www.fsf.org
 */

#ifndef __ASM_ARCH_MSM_RPC_HSUSB_H
#define __ASM_ARCH_MSM_RPC_HSUSB_H

#include <mach/msm_rpcrouter.h>
#include <mach/msm_otg.h>
#include <mach/msm_hsusb.h>

#if defined(CONFIG_SEMC_POWER) || \
    defined(CONFIG_SEMC_POWER_MODULE) || \
    defined(CONFIG_MAX17040_FUELGAUGE)
enum semc_charger {
	NO_CHARGER = 0,
	USB_CHARGER,
	WALL_CHARGER
};

typedef void (*usb_connect_status_callback_t) (enum semc_charger connected, uint32_t current_ma);

void msm_chg_rpc_register_semc_callback(usb_connect_status_callback_t connect_status_fn);
void msm_chg_rpc_unregister_semc_callback(void);
void msm_chg_rpc_semc_get_usb_connected(enum semc_charger *connected, u16 *max_current);
#endif /* CONFIG_SEMC_POWER ||
	  CONFIG_SEMC_POWER_MODULE ||
	  CONFIG_MAX17040_FUELGAUGE */

int msm_hsusb_rpc_connect(void);
int msm_hsusb_phy_reset(void);
int msm_hsusb_vbus_powerup(void);
int msm_hsusb_vbus_shutdown(void);
int msm_hsusb_send_productID(uint32_t product_id);
int msm_hsusb_send_serial_number(char *serial_number);
int msm_hsusb_is_serial_num_null(uint32_t val);
int msm_hsusb_reset_rework_installed(void);
int msm_hsusb_enable_pmic_ulpidata0(void);
int msm_hsusb_disable_pmic_ulpidata0(void);
int msm_hsusb_rpc_close(void);

int msm_chg_rpc_connect(void);
int msm_chg_usb_charger_connected(uint32_t type);
int msm_chg_usb_i_is_available(uint32_t sample);
int msm_chg_usb_i_is_not_available(void);
int msm_chg_usb_charger_disconnected(void);
int msm_chg_rpc_close(void);

#ifdef CONFIG_USB_GADGET_MSM_72K
int hsusb_chg_init(int connect);
void hsusb_chg_vbus_draw(unsigned mA);
void hsusb_chg_connected(enum chg_type chgtype);
#endif


int msm_fsusb_rpc_init(struct msm_otg_ops *ops);
int msm_fsusb_init_phy(void);
int msm_fsusb_reset_phy(void);
int msm_fsusb_suspend_phy(void);
int msm_fsusb_resume_phy(void);
int msm_fsusb_rpc_close(void);
int msm_fsusb_remote_dev_disconnected(void);
int msm_fsusb_set_remote_wakeup(void);
void msm_fsusb_rpc_deinit(void);

#if defined(CONFIG_MACH_ES209RA)
int msm_hsusb_chg_is_charging(void);
int msm_chg_battery_thermo(void);
int msm_chg_charger_current(void);
int msm_chg_qsd_thermo(void);
int msm_chg_charger_thermo(void);
#endif /* CONFIG_MACH_ES209RA */
#endif
