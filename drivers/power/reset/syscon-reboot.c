/*
 * Generic Syscon Reboot Driver
 *
 * Copyright (c) 2013, Applied Micro Circuits Corporation
 * Author: Feng Kan <fkan@apm.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/notifier.h>
#include <linux/mfd/syscon.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <linux/regmap.h>
#include <asm/system_misc.h>

struct syscon_reboot_context {
	struct platform_device *pdev;
	struct regmap *map;
	u32 offset;
	u32 mask;
};

static struct syscon_reboot_context *syscon_reboot_context;

static void syscon_restart(enum reboot_mode reboot_mode, const char *cmd)
{
	struct syscon_reboot_context *ctx = syscon_reboot_context;

	/* Issue the reboot */
	regmap_write(ctx->map, ctx->offset, ctx->mask);

	mdelay(1000);

	dev_emerg(&ctx->pdev->dev, "Unable to restart system\n");
}

static int syscon_reboot_probe(struct platform_device *pdev)
{
	struct syscon_reboot_context *ctx;
	struct device *dev = &pdev->dev;

	ctx = devm_kzalloc(&pdev->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->map = syscon_regmap_lookup_by_phandle(dev->of_node, "regmap");
	if (IS_ERR(ctx->map))
		return PTR_ERR(ctx->map);

	if (of_property_read_u32(pdev->dev.of_node, "offset", &ctx->offset))
		return -EINVAL;

	if (of_property_read_u32(pdev->dev.of_node, "mask", &ctx->mask))
		return -EINVAL;

	ctx->pdev = pdev;

	syscon_reboot_context = ctx;
	arm_pm_restart = syscon_restart;

	return 0;
}

static struct of_device_id syscon_reboot_of_match[] = {
	{ .compatible = "syscon-reboot" },
	{}
};

static struct platform_driver syscon_reboot_driver = {
	.probe = syscon_reboot_probe,
	.driver = {
		.name = "syscon-reboot",
		.of_match_table = syscon_reboot_of_match,
	},
};
module_platform_driver(syscon_reboot_driver);
