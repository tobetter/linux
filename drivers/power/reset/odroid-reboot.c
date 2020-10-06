/*
 * drivers/power/reset/odroid-reboot.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/reboot.h>

#include <asm/system_misc.h>

#include <asm/compiler.h>
#include <linux/kdebug.h>
#include <linux/arm-smccc.h>

#include <linux/gpio.h>
#include <linux/of_gpio.h>

static void odroid_card_reset(void);

static int sd_vqsw;
static int sd_vmmc;
static int sd_vqen;

#define CHECK_RET(ret) { \
	if (ret) \
	pr_err("[%s] gpio op failed(%d) at line %d\n",\
			__func__, ret, __LINE__); \
}

static noinline int __invoke_psci_fn_smc(u64 function_id, u64 arg0, u64 arg1,
					 u64 arg2)
{
	struct arm_smccc_res res;

	arm_smccc_smc((unsigned long)function_id,
			(unsigned long)arg0,
			(unsigned long)arg1,
			(unsigned long)arg2,
			0, 0, 0, 0, &res);
	return res.a0;
}

static void odroid_card_reset(void)
{
	int ret = 0;

	if ((sd_vqsw == 0) && (sd_vmmc == 0))
		return;

	if (sd_vqen == 0) {
		gpio_free(sd_vqsw);
		gpio_free(sd_vmmc);
		ret = gpio_request_one(sd_vqsw,
				GPIOF_OUT_INIT_LOW, "REBOOT");
		CHECK_RET(ret);
		mdelay(10);
		ret = gpio_direction_output(sd_vqsw, 1);
		CHECK_RET(ret);
		ret = gpio_request_one(sd_vmmc,
				GPIOF_OUT_INIT_LOW, "REBOOT");
		CHECK_RET(ret);
		mdelay(10);
		ret = gpio_direction_output(sd_vqsw, 0);
		CHECK_RET(ret);
		ret = gpio_direction_output(sd_vmmc, 1);
		CHECK_RET(ret);
		mdelay(5);
		gpio_free(sd_vqsw);
		gpio_free(sd_vmmc);
	} else {
		gpio_free(sd_vqsw);
		gpio_free(sd_vqen);
		gpio_free(sd_vmmc);

		ret = gpio_request_one(sd_vqsw,
				GPIOF_OUT_INIT_LOW, "REBOOT");
		CHECK_RET(ret);
		ret = gpio_request_one(sd_vqen,
				GPIOF_OUT_INIT_LOW, "REBOOT");
		CHECK_RET(ret);
		ret = gpio_request_one(sd_vmmc,
				GPIOF_OUT_INIT_LOW, "REBOOT");
		CHECK_RET(ret);
		mdelay(100);
		ret = gpio_direction_input(sd_vqen);
		CHECK_RET(ret);
		ret = gpio_direction_input(sd_vmmc);
		CHECK_RET(ret);
		ret = gpio_direction_input(sd_vqsw);
		CHECK_RET(ret);
		mdelay(5);
		gpio_free(sd_vqen);
		gpio_free(sd_vmmc);
		gpio_free(sd_vqsw);
	}
}

static int odroid_reset_handler(struct notifier_block *this,
				 unsigned long mode, void *cmd)
{
	odroid_card_reset();

	switch (mode) {
		case SYS_OFF_MODE_POWER_OFF:
			__invoke_psci_fn_smc(0x82000042, 1, 0, 0);
			__invoke_psci_fn_smc(0x84000008, 0, 0, 0);
			break;
		case SYS_OFF_MODE_RESTART:
			__invoke_psci_fn_smc(0x84000009, 0, 0, 0);
			break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block odroid_reset_nb = {
	.notifier_call = odroid_reset_handler,
	.priority = 192,
};

static int odroid_restart_probe(struct platform_device *pdev)
{
	struct device_node *of_node;
	int ret;

	of_node = pdev->dev.of_node;

	sd_vqsw = of_get_named_gpio(of_node, "sd-vqsw", 0);
	if (!gpio_is_valid(sd_vqsw))
		sd_vqsw = 0;

	sd_vmmc = of_get_named_gpio(of_node, "sd-vmmc", 0);
	if (!gpio_is_valid(sd_vmmc))
		sd_vmmc = 0;

	sd_vqen = of_get_named_gpio(of_node, "sd-vqen", 0);
	if (!gpio_is_valid(sd_vqen))
		sd_vqen = 0;

	ret = register_restart_handler(&odroid_reset_nb);
	if (ret) {
		dev_err(&pdev->dev,
			"cannot register restart handler (err=%d)\n", ret);
		return ret;
	}

	return 0;
}

static const struct of_device_id of_odroid_restart_match[] = {
	{ .compatible = "odroid,reboot", },
	{},
};
MODULE_DEVICE_TABLE(of, of_odroid_restart_match);

static struct platform_driver odroid_restart_driver = {
	.probe = odroid_restart_probe,
	.driver = {
		.name = "odroid-restart",
		.of_match_table = of_match_ptr(of_odroid_restart_match),
	},
};

static int __init odroid_restart_init(void)
{
	return platform_driver_register(&odroid_restart_driver);
}
device_initcall(odroid_restart_init);
