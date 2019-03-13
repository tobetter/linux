// SPDX-License-Identifier: GPL-2.0
/*
 * Meson G12A USB2 PHY driver
 *
 * Copyright (C) 2017 Martin Blumenstingl <martin.blumenstingl@googlemail.com>
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved
 * Copyright (C) 2019 BayLibre, SAS
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>

#define PHY_CTRL_R0						0x0
#define PHY_CTRL_R1						0x4
#define PHY_CTRL_R2						0x8
#define PHY_CTRL_R3						0xc
#define PHY_CTRL_R4						0x10
#define PHY_CTRL_R5						0x14
#define PHY_CTRL_R6						0x18
#define PHY_CTRL_R7						0x1c
#define PHY_CTRL_R8						0x20
#define PHY_CTRL_R9						0x24
#define PHY_CTRL_R10						0x28
#define PHY_CTRL_R11						0x2c
#define PHY_CTRL_R12						0x30
#define PHY_CTRL_R13						0x34
#define PHY_CTRL_R14						0x38
#define PHY_CTRL_R15						0x3c
#define PHY_CTRL_R16						0x40
#define PHY_CTRL_R17						0x44
#define PHY_CTRL_R18						0x48
#define PHY_CTRL_R19						0x4c
#define PHY_CTRL_R20						0x50
#define PHY_CTRL_R21						0x54
#define PHY_CTRL_R22						0x58
#define PHY_CTRL_R23						0x5c

#define RESET_COMPLETE_TIME					1000
#define PLL_RESET_COMPLETE_TIME					100

struct phy_meson_g12a_usb2_priv {
	struct device		*dev;
	struct regmap		*regmap;
	struct clk		*clk;
	struct reset_control	*reset;
};

static const struct regmap_config phy_meson_g12a_usb2_regmap_conf = {
	.reg_bits = 8,
	.val_bits = 32,
	.reg_stride = 4,
	.max_register = PHY_CTRL_R23,
};

static int phy_meson_g12a_usb2_init(struct phy *phy)
{
	struct phy_meson_g12a_usb2_priv *priv = phy_get_drvdata(phy);
	int ret;

	ret = reset_control_reset(priv->reset);
	if (ret)
		return ret;

	udelay(RESET_COMPLETE_TIME);

	/* usb2_otg_aca_en == 0 */
	regmap_update_bits(priv->regmap, PHY_CTRL_R21, BIT(2), 0);

	/* PLL Setup : 24MHz * 20 / 1 = 480MHz */
	regmap_write(priv->regmap, PHY_CTRL_R16, 0x39400414);
	regmap_write(priv->regmap, PHY_CTRL_R17, 0x927e0000);
	regmap_write(priv->regmap, PHY_CTRL_R18, 0xac5f49e5);

	udelay(PLL_RESET_COMPLETE_TIME);

	/* UnReset PLL */
	regmap_write(priv->regmap, PHY_CTRL_R16, 0x19400414);

	/* PHY Tuning */
	regmap_write(priv->regmap, PHY_CTRL_R20, 0xfe18);
	regmap_write(priv->regmap, PHY_CTRL_R4, 0x8000fff);

	/* Tuning Disconnect Threshold */
	regmap_write(priv->regmap, PHY_CTRL_R3, 0x34);

	/* Analog Settings */
	regmap_write(priv->regmap, PHY_CTRL_R14, 0);
	regmap_write(priv->regmap, PHY_CTRL_R13, 0x78000);

	return 0;
}

static int phy_meson_g12a_usb2_exit(struct phy *phy)
{
	struct phy_meson_g12a_usb2_priv *priv = phy_get_drvdata(phy);

	return reset_control_reset(priv->reset);
}

/* set_mode is not needed, mode setting is handled via the UTMI bus */
static const struct phy_ops phy_meson_g12a_usb2_ops = {
	.init		= phy_meson_g12a_usb2_init,
	.exit		= phy_meson_g12a_usb2_exit,
	.owner		= THIS_MODULE,
};

static int phy_meson_g12a_usb2_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phy_provider *phy_provider;
	struct resource *res;
	struct phy_meson_g12a_usb2_priv *priv;
	struct phy *phy;
	void __iomem *base;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = dev;
	platform_set_drvdata(pdev, priv);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	priv->regmap = devm_regmap_init_mmio(dev, base,
					     &phy_meson_g12a_usb2_regmap_conf);
	if (IS_ERR(priv->regmap))
		return PTR_ERR(priv->regmap);

	priv->clk = devm_clk_get(dev, "xtal");
	if (IS_ERR(priv->clk))
		return PTR_ERR(priv->clk);

	priv->reset = devm_reset_control_get(dev, "phy");
	if (IS_ERR(priv->reset))
		return PTR_ERR(priv->reset);

	ret = reset_control_deassert(priv->reset);
	if (ret)
		return ret;

	phy = devm_phy_create(dev, NULL, &phy_meson_g12a_usb2_ops);
	if (IS_ERR(phy)) {
		ret = PTR_ERR(phy);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "failed to create PHY\n");

		return ret;
	}

	phy_set_bus_width(phy, 8);
	phy_set_drvdata(phy, priv);

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);

	return PTR_ERR_OR_ZERO(phy_provider);
}

static const struct of_device_id phy_meson_g12a_usb2_of_match[] = {
	{ .compatible = "amlogic,g12a-usb2-phy", },
	{ },
};
MODULE_DEVICE_TABLE(of, phy_meson_g12a_usb2_of_match);

static struct platform_driver phy_meson_g12a_usb2_driver = {
	.probe	= phy_meson_g12a_usb2_probe,
	.driver	= {
		.name		= "phy-meson-g12a-usb2",
		.of_match_table	= phy_meson_g12a_usb2_of_match,
	},
};
module_platform_driver(phy_meson_g12a_usb2_driver);

MODULE_AUTHOR("Martin Blumenstingl <martin.blumenstingl@googlemail.com>");
MODULE_AUTHOR("Neil Armstrong <narmstrong@baylibre.com>");
MODULE_DESCRIPTION("Meson G12A USB2 PHY driver");
MODULE_LICENSE("GPL v2");
