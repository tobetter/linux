// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2017 BayLibre, SAS
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 */

#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/bitfield.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/regmap.h>

#define MSR_CLK_DUTY		0x0
#define MSR_CLK_REG0		0x4
#define MSR_CLK_REG1		0x8
#define MSR_CLK_REG2		0xc

#define MSR_CLK_DIV		GENMASK(15, 0)
#define MSR_ENABLE		BIT(16)
#define MSR_CONT		BIT(17) /* continuous measurement */
#define MSR_INTR		BIT(18) /* interrupts */
#define MSR_RUN			BIT(19)
#define MSR_CLK_SRC		GENMASK(26, 20)
#define MSR_BUSY		BIT(31)

#define MSR_VAL_MASK		GENMASK(15, 0)

#define DIV_50US		64

#define CLK_MSR_MAX		128

struct meson_gx_msr {
	struct regmap *regmap;
};

struct meson_gx_msr_id {
	struct meson_gx_msr *priv;
	unsigned int id;
	const char *name;
};

#define CLK_MSR_ID(__id, __name) \
	[__id] = {.id = __id, .name = __name,}

static struct meson_gx_msr_id clk_msr[CLK_MSR_MAX] = {
	CLK_MSR_ID(0, "ring_osc_out_ee_0"),
	CLK_MSR_ID(1, "ring_osc_out_ee_1"),
	CLK_MSR_ID(2, "ring_osc_out_ee_2"),
	CLK_MSR_ID(3, "a53_ring_osc"),
	CLK_MSR_ID(4, "gp0_pll"),
	CLK_MSR_ID(6, "enci"),
	CLK_MSR_ID(7, "clk81"),
	CLK_MSR_ID(8, "encp"),
	CLK_MSR_ID(9, "encl"),
	CLK_MSR_ID(10, "vdac"),
	CLK_MSR_ID(11, "rgmii_tx"),
	CLK_MSR_ID(12, "pdm"),
	CLK_MSR_ID(13, "amclk"),
	CLK_MSR_ID(14, "fec_0"),
	CLK_MSR_ID(15, "fec_1"),
	CLK_MSR_ID(16, "fec_2"),
	CLK_MSR_ID(17, "sys_pll_div16"),
	CLK_MSR_ID(18, "sys_cpu_div16"),
	CLK_MSR_ID(19, "hdmitx_sys"),
	CLK_MSR_ID(20, "rtc_osc_out"),
	CLK_MSR_ID(21, "i2s_in_src0"),
	CLK_MSR_ID(22, "eth_phy_ref"),
	CLK_MSR_ID(23, "hdmi_todig"),
	CLK_MSR_ID(26, "sc_int"),
	CLK_MSR_ID(28, "sar_adc"),
	CLK_MSR_ID(31, "mpll_test_out"),
	CLK_MSR_ID(32, "vdec"),
	CLK_MSR_ID(35, "mali"),
	CLK_MSR_ID(36, "hdmi_tx_pixel"),
	CLK_MSR_ID(37, "i958"),
	CLK_MSR_ID(38, "vdin_meas"),
	CLK_MSR_ID(39, "pcm_sclk"),
	CLK_MSR_ID(40, "pcm_mclk"),
	CLK_MSR_ID(41, "eth_rx_or_rmii"),
	CLK_MSR_ID(42, "mp0_out"),
	CLK_MSR_ID(43, "fclk_div5"),
	CLK_MSR_ID(44, "pwm_b"),
	CLK_MSR_ID(45, "pwm_a"),
	CLK_MSR_ID(46, "vpu"),
	CLK_MSR_ID(47, "ddr_dpll_pt"),
	CLK_MSR_ID(48, "mp1_out"),
	CLK_MSR_ID(49, "mp2_out"),
	CLK_MSR_ID(50, "mp3_out"),
	CLK_MSR_ID(51, "nand_core"),
	CLK_MSR_ID(52, "sd_emmc_b"),
	CLK_MSR_ID(53, "sd_emmc_a"),
	CLK_MSR_ID(55, "vid_pll_div_out"),
	CLK_MSR_ID(56, "cci"),
	CLK_MSR_ID(57, "wave420l_c"),
	CLK_MSR_ID(58, "wave420l_b"),
	CLK_MSR_ID(59, "hcodec"),
	CLK_MSR_ID(60, "alt_32k"),
	CLK_MSR_ID(61, "gpio_msr"),
	CLK_MSR_ID(62, "hevc"),
	CLK_MSR_ID(66, "vid_lock"),
	CLK_MSR_ID(70, "pwm_f"),
	CLK_MSR_ID(71, "pwm_e"),
	CLK_MSR_ID(72, "pwm_d"),
	CLK_MSR_ID(73, "pwm_C"),
	CLK_MSR_ID(75, "aoclkx2_int"),
	CLK_MSR_ID(76, "aoclk_int"),
	CLK_MSR_ID(77, "rng_ring_osc_0"),
	CLK_MSR_ID(78, "rng_ring_osc_1"),
	CLK_MSR_ID(79, "rng_ring_osc_2"),
	CLK_MSR_ID(80, "rng_ring_osc_3"),
	CLK_MSR_ID(81, "vapb"),
	CLK_MSR_ID(82, "ge2d"),
};

static int meson_gx_measure_id(struct meson_gx_msr *priv, unsigned int id)
{
	unsigned int val;
	int ret;

	regmap_write(priv->regmap, MSR_CLK_REG0, 0);

	/* Set measurement gate to 50uS */
	regmap_update_bits(priv->regmap, MSR_CLK_REG0, MSR_CLK_DIV,
			   FIELD_PREP(MSR_CLK_DIV, DIV_50US));

	/* Set ID */
	regmap_update_bits(priv->regmap, MSR_CLK_REG0, MSR_CLK_SRC,
			   FIELD_PREP(MSR_CLK_SRC, id));

	/* Enable & Start */
	regmap_update_bits(priv->regmap, MSR_CLK_REG0,
			   MSR_RUN | MSR_ENABLE,
			   MSR_RUN | MSR_ENABLE);

	ret = regmap_read_poll_timeout(priv->regmap, MSR_CLK_REG0,
				       val, !(val & MSR_BUSY), 10, 1000);
	if (ret)
		return ret;

	/* Disable */
	regmap_update_bits(priv->regmap, MSR_CLK_REG0, MSR_ENABLE, 0);

	/* Get the value in MHz*64 */
	regmap_read(priv->regmap, MSR_CLK_REG2, &val);

	return (((val + 31) & MSR_VAL_MASK) / 64) * 1000000;
}

static int clk_msr_show(struct seq_file *s, void *data)
{
	struct meson_gx_msr_id *clk_msr_id = s->private;
	int val;

	val = meson_gx_measure_id(clk_msr_id->priv, clk_msr_id->id);
	if (val < 0)
		return val;

	seq_printf(s, "%d\n", val);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(clk_msr);

static const struct regmap_config clk_msr_regmap_config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
	.max_register = MSR_CLK_REG2,
};

static int meson_gx_msr_probe(struct platform_device *pdev)
{
	struct meson_gx_msr *priv;
	struct resource *res;
	struct dentry *root;
	void __iomem *base;
	int i;

	priv = devm_kzalloc(&pdev->dev, sizeof(struct meson_gx_msr),
			    GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base)) {
		dev_err(&pdev->dev, "io resource mapping failed\n");
		return PTR_ERR(base);
	}

	priv->regmap = devm_regmap_init_mmio(&pdev->dev, base,
					      &clk_msr_regmap_config);
	if (IS_ERR(priv->regmap))
		return PTR_ERR(priv->regmap);

	root = debugfs_create_dir("meson-clk-msr", NULL);

	for (i = 0 ; i < CLK_MSR_MAX ; ++i) {
		if (!clk_msr[i].name)
			continue;

		clk_msr[i].priv = priv;

		debugfs_create_file(clk_msr[i].name, 0444, root,
				    &clk_msr[i], &clk_msr_fops);
	}

	return 0;
}

static const struct of_device_id meson_gx_msr_match_table[] = {
	{ .compatible = "amlogic,meson-gx-clk-measure" },
	{ /* sentinel */ }
};

static struct platform_driver meson_gx_msr_driver = {
	.probe	= meson_gx_msr_probe,
	.driver = {
		.name		= "meson_gx_msr",
		.of_match_table	= meson_gx_msr_match_table,
	},
};
builtin_platform_driver(meson_gx_msr_driver);
