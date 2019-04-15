/*
 * Driver for configuration of remote wakeup functionality
 *   Avilable to assign user remote wakeup key
 *   and its decode protocol
 *
 * Copyright (C) 2019 Hardkernel Co., Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>

#include <media/rc-core.h>
#include <linux/amlogic/iomap.h>
#include <linux/amlogic/scpi_protocol.h>

#include <linux/reboot.h>

#include "hk-lirc-helper.h"

#define DRIVER_NAME	"hk-lirc-helper"

static u32 remotewakeup = 0xffffffff;
static u32 remotewakeupmask = 0xffffffff;
static int decode_type = IR_DECODE_NEC;
void __iomem *ir_reg;

const struct remote_reg_proto *remote_reg_proto_hk[] = {
	&reg_nec,
	&reg_duokan,
	&reg_xmp_1,
	&reg_xmp_1_sw,
	&reg_nec_sw,
	&reg_rc5,
	&reg_rc6,
	&reg_legacy_nec,
	&reg_rc6_21bit,
	NULL
};

void remote_wakeup_decode_type(int dec_type)
{
	decode_type = dec_type;
}
EXPORT_SYMBOL(remote_wakeup_decode_type);

static int remote_handle_usrkey(void)
{
	scpi_send_usr_data(SCPI_CL_REMOTE, &remotewakeup,
				sizeof(remotewakeup));
	scpi_send_usr_data(SCPI_CL_IRPROTO, &decode_type,
				sizeof(decode_type));
	scpi_send_usr_data(SCPI_CL_REMOTE_MASK, &remotewakeupmask,
				sizeof(remotewakeupmask));
	return 0;
}

static void remote_nec_convert_key(void)
{
	u32 usr_key;
	u32 code_inverse;
	u16 code;
	u16 i, shift;

	usr_key = remotewakeup;
	remotewakeup = 0xffffffff;

	/* pre-data */
	remotewakeup = ((usr_key >> 16) & 0xffff);

	/* code bit inverse */
	code = (u16)(usr_key & 0xffff);
	code_inverse = 0;
	for (i = 0; i < 8; i++) {
		shift = 15 - (2 * i);
		code_inverse |= ((code & BIT_MASK(i)) << shift);
		code_inverse |= ((code & BIT_MASK(15 - i)) >> shift);
	}
	remotewakeup |= (code_inverse << 16);
}

static int __init remote_wakeupmask_setup(char *str)
{
	int ret;

	if (str == NULL) {
		pr_info("%s no string\n", __func__);
		return -EINVAL;
	}

	ret = kstrtouint(str, 16, &remotewakeupmask);
	if (ret) {
		remotewakeupmask = 0xffffffff;
		return -EINVAL;
	}

	return 0;
}
__setup("remotewakeupmask=", remote_wakeupmask_setup);

static int __init remote_irdecode_type(char *str)
{
	if (str == NULL) {
		decode_type = IR_DECODE_NEC;
		return -EINVAL;
	}

	if (!strncmp(str, "DUOKAN", 6))
		decode_type = IR_DECODE_DUOKAN;
	else if (!strncmp(str, "NEC_LEGACY", 10))
		decode_type = IR_DECODE_LEGACY_NEC;
	else if (!strncmp(str, "NEC_SW", 6))
		decode_type = IR_DECODE_NEC_SW;
	else if (!strncmp(str, "NEC", 3))
		decode_type = IR_DECODE_NEC;
	else if (!strncmp(str, "XMP_1_SW", 8))
		decode_type = IR_DECODE_XMP_1_SW;
	else if (!strncmp(str, "XMP_1", 5))
		decode_type = IR_DECODE_XMP_1;
	else if (!strncmp(str, "RC5", 3))
		decode_type = IR_DECODE_RC5;
	else if (!strncmp(str, "RC6_21BIT", 9))
		decode_type = IR_DECODE_RC6_21BIT;
	else if (!strncmp(str, "RC6", 3))
		decode_type = IR_DECODE_RC6;
	/* default NEC protocol for invalid strings */
	else
		decode_type = IR_DECODE_NEC;

	return 0;
}
__setup("irdecodetype=", remote_irdecode_type);

static int __init remote_wakeup_setup(char *str)
{
	int ret;

	if (str == NULL) {
		pr_info("%s no string\n", __func__);
		return -EINVAL;
	}

	ret = kstrtouint(str, 16, &remotewakeup);
	if (ret) {
		remotewakeup = 0xffffffff;
		return -EINVAL;
	}

	return 0;
}
__setup("remotewakeup=", remote_wakeup_setup);

static int hk_lirc_helper_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	int ret;

	if (!(pdev->dev.of_node)) {
		dev_err(dev, "pdev->dev.of_node == NULL\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res) {
		ir_reg = ioremap(res->start, res->end - res->start);
	} else {
		dev_info(dev, "Failed to get ir reg\n");
		ir_reg = NULL;
	}

	remote_handle_usrkey();

	pr_info("lirc_helper: wakeupkey 0x%x, protocol 0x%x, mask 0x%x\n",
		remotewakeup, decode_type, remotewakeupmask);

	return 0;
}

static int hk_lirc_helper_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id hk_lirc_helper_match[] = {
	{ .compatible = DRIVER_NAME },
	{ },
};

static struct platform_driver hk_lirc_helper_driver = {
	.probe		= hk_lirc_helper_probe,
	.remove		= hk_lirc_helper_remove,
	.driver = {
		.name		= DRIVER_NAME,
		.of_match_table	= hk_lirc_helper_match,
	},
};

module_platform_driver(hk_lirc_helper_driver);

module_param(remotewakeup,uint,0660);
MODULE_PARM_DESC(remotewakeup, "remotewakeup is the ir keycode to wakeup from suspend/poweroff");
module_param(decode_type,int,0660);
MODULE_PARM_DESC(decode_type, "decode_type is the ir decoding type. Default is 3 (NEC)");
module_param(remotewakeupmask,uint,0660);
MODULE_PARM_DESC(remotewakeupmask, "remotewakeupmask is the ir keycode mask for remotewakeup");

MODULE_DESCRIPTION("Hardkernel LIRC helper driver");
MODULE_AUTHOR("Joy Cho <joy.cho@hardkernel.com>");
MODULE_LICENSE("GPL");
