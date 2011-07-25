/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
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

#ifndef __LINUX_USB_GADGET_MSM72K_OTG_H__
#define __LINUX_USB_GADGET_MSM72K_OTG_H__

#include <linux/usb.h>
#include <linux/usb/gadget.h>
#include <linux/usb/otg.h>

#define OTGSC_BSVIE            (1 << 27)
#define OTGSC_IDIE             (1 << 24)
#define OTGSC_BSVIS            (1 << 19)
#define OTGSC_ID               (1 << 8)
#define OTGSC_IDIS             (1 << 16)
#define OTGSC_BSV              (1 << 11)

#define ULPI_STP_CTRL   (1 << 30)
#define ASYNC_INTR_CTRL (1 << 29)

#define PORTSC_PHCD     (1 << 23)
#define disable_phy_clk() (writel(readl(USB_PORTSC) | PORTSC_PHCD, USB_PORTSC))
#define enable_phy_clk() (writel(readl(USB_PORTSC) & ~PORTSC_PHCD, USB_PORTSC))
#define is_phy_clk_disabled() (readl(USB_PORTSC) & PORTSC_PHCD)
#define is_usb_active()       (!(readl(USB_PORTSC) & PORTSC_SUSP))

struct msm_otg {
	struct otg_transceiver otg;

	struct clk		*clk;
	struct clk		*pclk;
	struct clk		*cclk;
	int			irq;
	int			vbus_on_irq;
	void __iomem		*regs;
	u8			in_lpm;
	atomic_t		chg_type;
	struct msm_otg_platform_data *pdata;
	unsigned int 		core_clk;
	int 			(*rpc_connect)(int);
	int 			(*phy_reset)(void __iomem *);
	void (*start_host)	(struct usb_bus *bus, int suspend);
	/* Enable/disable the clocks */
	int (*set_clk)		(struct otg_transceiver *otg, int on);
	/* pmic notfications apis */
	u8 pmic_notif_supp;
	int (*pmic_notif_init) (void);
	void (*pmic_notif_deinit) (void);
	int (*pmic_register_vbus_sn) (void (*callback)(int online));
	void (*pmic_unregister_vbus_sn) (void (*callback)(int online));
	int (*pmic_enable_ldo) (int);
	int pclk_required_during_lpm;
};

/* usb controller's protocol engine depends on AXI clock.
 * On some platforms this dependency is removed by
 * introducing usb core clock
 */
static inline int depends_on_axi_freq(struct otg_transceiver *xceiv)
{
	struct msm_otg *dev;

	if (!xceiv)
		return 0;

	dev = container_of(xceiv, struct msm_otg, otg);

	return !dev->core_clk;
}

#endif
