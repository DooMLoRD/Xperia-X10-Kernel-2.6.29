/*
 * Copyright (C) 2007 Google, Inc.
 * Author: Brian Swetland <swetland@google.com>
 */

#ifndef __ARCH_ARM_MACH_MSM_GPIO_HW_7XXX_H
#define __ARCH_ARM_MACH_MSM_GPIO_HW_7XXX_H

/* output value */
#define GPIO_OUT_0         GPIO1_REG(0x00)  /* gpio  15-0  */
#define GPIO_OUT_1         GPIO2_REG(0x00)  /* gpio  42-16 */
#define GPIO_OUT_2         GPIO1_REG(0x04)  /* gpio  67-43 */
#define GPIO_OUT_3         GPIO1_REG(0x08)  /* gpio  94-68 */
#define GPIO_OUT_4         GPIO1_REG(0x0C)  /* gpio 106-95 */
#define GPIO_OUT_5         GPIO1_REG(0x50)  /* gpio 107-121 */

/* same pin map as above, output enable */
#define GPIO_OE_0          GPIO1_REG(0x10)
#define GPIO_OE_1          GPIO2_REG(0x08)
#define GPIO_OE_2          GPIO1_REG(0x14)
#define GPIO_OE_3          GPIO1_REG(0x18)
#define GPIO_OE_4          GPIO1_REG(0x1C)
#define GPIO_OE_5          GPIO1_REG(0x54)

/* same pin map as above, input read */
#define GPIO_IN_0          GPIO1_REG(0x34)
#define GPIO_IN_1          GPIO2_REG(0x20)
#define GPIO_IN_2          GPIO1_REG(0x38)
#define GPIO_IN_3          GPIO1_REG(0x3C)
#define GPIO_IN_4          GPIO1_REG(0x40)
#define GPIO_IN_5          GPIO1_REG(0x44)

/* same pin map as above, 1=edge 0=level interrup */
#define GPIO_INT_EDGE_0    GPIO1_REG(0x60)
#define GPIO_INT_EDGE_1    GPIO2_REG(0x50)
#define GPIO_INT_EDGE_2    GPIO1_REG(0x64)
#define GPIO_INT_EDGE_3    GPIO1_REG(0x68)
#define GPIO_INT_EDGE_4    GPIO1_REG(0x6C)
#define GPIO_INT_EDGE_5    GPIO1_REG(0xC0)

/* same pin map as above, 1=positive 0=negative */
#define GPIO_INT_POS_0     GPIO1_REG(0x70)
#define GPIO_INT_POS_1     GPIO2_REG(0x58)
#define GPIO_INT_POS_2     GPIO1_REG(0x74)
#define GPIO_INT_POS_3     GPIO1_REG(0x78)
#define GPIO_INT_POS_4     GPIO1_REG(0x7C)
#define GPIO_INT_POS_5     GPIO1_REG(0xBC)

/* same pin map as above, interrupt enable */
#define GPIO_INT_EN_0      GPIO1_REG(0x80)
#define GPIO_INT_EN_1      GPIO2_REG(0x60)
#define GPIO_INT_EN_2      GPIO1_REG(0x84)
#define GPIO_INT_EN_3      GPIO1_REG(0x88)
#define GPIO_INT_EN_4      GPIO1_REG(0x8C)
#define GPIO_INT_EN_5      GPIO1_REG(0xB8)

/* same pin map as above, write 1 to clear interrupt */
#define GPIO_INT_CLEAR_0   GPIO1_REG(0x90)
#define GPIO_INT_CLEAR_1   GPIO2_REG(0x68)
#define GPIO_INT_CLEAR_2   GPIO1_REG(0x94)
#define GPIO_INT_CLEAR_3   GPIO1_REG(0x98)
#define GPIO_INT_CLEAR_4   GPIO1_REG(0x9C)
#define GPIO_INT_CLEAR_5   GPIO1_REG(0xB4)

/* same pin map as above, 1=interrupt pending */
#define GPIO_INT_STATUS_0  GPIO1_REG(0xA0)
#define GPIO_INT_STATUS_1  GPIO2_REG(0x70)
#define GPIO_INT_STATUS_2  GPIO1_REG(0xA4)
#define GPIO_INT_STATUS_3  GPIO1_REG(0xA8)
#define GPIO_INT_STATUS_4  GPIO1_REG(0xAC)
#define GPIO_INT_STATUS_5  GPIO1_REG(0xB0)

#endif
