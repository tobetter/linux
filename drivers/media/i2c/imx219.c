/*
 * Driver for IMX219 CMOS Image Sensor from Sony
 *
 * Copyright (C) 2014, Andrew Chew <achew@nvidia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * V0.0X01.0X01 add enum_frame_interval function.
 * V0.0X01.0X02 add function g_mbus_config.
 */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <linux/version.h>
#include <linux/rk-camera-module.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-image-sizes.h>
#include <media/v4l2-mediabus.h>
#include <asm/unaligned.h>

#define DRIVER_VERSION			KERNEL_VERSION(0, 0x01, 0x2)

#define REG_VALUE_08BIT		1
#define REG_VALUE_16BIT		2

/* IMX219 supported geometry */
#define IMX219_TABLE_END		0xffff

#define IMX219_ANALOGUE_GAIN_MULTIPLIER	256
#define IMX219_ANALOGUE_GAIN_MIN	(1 * IMX219_ANALOGUE_GAIN_MULTIPLIER)
#define IMX219_ANALOGUE_GAIN_MAX	(11 * IMX219_ANALOGUE_GAIN_MULTIPLIER)
#define IMX219_ANALOGUE_GAIN_DEFAULT	(2 * IMX219_ANALOGUE_GAIN_MULTIPLIER)

/* In dB*256 */
#define IMX219_DIGITAL_GAIN_MIN		256
#define IMX219_DIGITAL_GAIN_MAX		43663
#define IMX219_DIGITAL_GAIN_DEFAULT	256

#define IMX219_DIGITAL_EXPOSURE_MIN	0
#define IMX219_DIGITAL_EXPOSURE_MAX	4095
#define IMX219_DIGITAL_EXPOSURE_DEFAULT	1575

/* IMX219 Register address */
#define IMX219_REG_MODEL_ID	0x0000
#define IMX219_REG_LOT_ID_H	0x0004
#define IMX219_REG_LOT_ID_M	0x0005
#define IMX219_REG_LOT_ID_L	0x0006
#define IMX219_REG_CHIP_ID	0x000D
#define IMX219_REG_MODE_SELECT	0x0100
#define IMX219_REG_EXPOSURE	0x015A

#define IMX219_REG_ANA_GAIN_GLOBAL_A	0x0157
#define IMX219_REG_DIG_GAIN_GLOBAL_A	0x0158
#define IMX219_REG_FRM_LENGTH_A	0x0160

#define IMX219_REG_IMG_ORIENTATION	0x0172

#define IMX219_REG_TP	0x0600
#define IMX219_REG_TD_R	0x0602
#define IMX219_REG_TD_GR	0x0604
#define IMX219_REG_TD_B	0x0606
#define IMX219_REG_TD_GB	0x0608
#define IMX219_REG_TP_WINDOW_WIDTH	0x0624
#define IMX219_REG_TP_WINDOW_HEIGHT	0x0626

#define IMX219_EXP_LINES_MARGIN	4

#define IMX219_VTS_MAX	0xffff

#define IMX219_MODEL_ID	0x0219
#define IMX219_NAME			"imx219"

#define IMX219_LANES			2

static const s64 link_freq_menu_items[] = {
	456000000,
};

/* Pixel rate is fixed at 182.4M for all the modes */
#define IMX219_PIXEL_RATE		182400000

struct imx219_reg {
	u16 addr;
	u8 val;
};

struct imx219_mode {
	u32 bus_fmt;
	u32 width;
	u32 height;
	struct v4l2_fract max_fps;
	u32 hts_def;
	u32 vts_def;
	const struct imx219_reg *reg_list;
	u32 hdr_mode;
	u32 freq_idx;
};

/* MCLK:24MHz  3280x2464  21.2fps   MIPI LANE2 */
static const struct imx219_reg imx219_init_tab_3280_2464_21fps[] = {
	{0x30EB, 0x05},		/* Access Code for address over 0x3000 */
	{0x30EB, 0x0C},		/* Access Code for address over 0x3000 */
	{0x300A, 0xFF},		/* Access Code for address over 0x3000 */
	{0x300B, 0xFF},		/* Access Code for address over 0x3000 */
	{0x30EB, 0x05},		/* Access Code for address over 0x3000 */
	{0x30EB, 0x09},		/* Access Code for address over 0x3000 */
	{0x0114, 0x01},		/* CSI_LANE_MODE[1:0] */
	{0x0128, 0x00},		/* DPHY_CNTRL */
	{0x012A, 0x18},		/* EXCK_FREQ[15:8] */
	{0x012B, 0x00},		/* EXCK_FREQ[7:0] */
	{0x0164, 0x00},		/* X_ADD_STA_A[11:8] */
	{0x0165, 0x00},		/* X_ADD_STA_A[7:0] */
	{0x0166, 0x0c},		/* X_ADD_END_A[11:8] */
	{0x0167, 0xcf},		/* X_ADD_END_A[7:0] */
	{0x0168, 0x00},		/* Y_ADD_STA_A[11:8] */
	{0x0169, 0x00},		/* Y_ADD_STA_A[7:0] */
	{0x016A, 0x09},		/* Y_ADD_END_A[11:8] */
	{0x016B, 0x9f},		/* Y_ADD_END_A[7:0] */
	{0x016C, 0x0c},		/* X_OUTPUT_SIZE[11:8] */
	{0x016D, 0xd0},		/* X_OUTPUT_SIZE[7:0] */
	{0x016E, 0x09},		/* Y_OUTPUT_SIZE[11:8] */
	{0x016f, 0xa0},		/* Y_OUTPUT_SIZE[7:0] */
	{0x015A, 0x01},		/* INTEG TIME[15:8] */
	{0x015B, 0xF4},		/* INTEG TIME[7:0] */
	{0x0160, 0x09},		/* FRM_LENGTH_A[15:8] */
	{0x0161, 0xC4},		/* FRM_LENGTH_A[7:0] */
	{0x0162, 0x0D},		/* LINE_LENGTH_A[15:8] */
	{0x0163, 0x78},		/* LINE_LENGTH_A[7:0] */
	{0x0260, 0x09},		/* FRM_LENGTH_B[15:8] */
	{0x0261, 0xC4},		/* FRM_LENGTH_B[7:0] */
	{0x0262, 0x0D},		/* LINE_LENGTH_B[15:8] */
	{0x0263, 0x78},		/* LINE_LENGTH_B[7:0] */
	{0x0170, 0x01},		/* X_ODD_INC_A[2:0] */
	{0x0171, 0x01},		/* Y_ODD_INC_A[2:0] */
	{0x0270, 0x01},		/* X_ODD_INC_B[2:0] */
	{0x0271, 0x01},		/* Y_ODD_INC_B[2:0] */
	{0x0174, 0x00},		/* BINNING_MODE_H_A */
	{0x0175, 0x00},		/* BINNING_MODE_V_A */
	{0x0274, 0x00},		/* BINNING_MODE_H_B */
	{0x0275, 0x00},		/* BINNING_MODE_V_B */
	{0x018C, 0x0A},		/* CSI_DATA_FORMAT_A[15:8] */
	{0x018D, 0x0A},		/* CSI_DATA_FORMAT_A[7:0] */
	{0x028C, 0x0A},		/* CSI_DATA_FORMAT_B[15:8] */
	{0x028D, 0x0A},		/* CSI_DATA_FORMAT_B[7:0] */
	{0x0301, 0x05},		/* VTPXCK_DIV */
	{0x0303, 0x01},		/* VTSYCK_DIV */
	{0x0304, 0x03},		/* PREPLLCK_VT_DIV[3:0] */
	{0x0305, 0x03},		/* PREPLLCK_OP_DIV[3:0] */
	{0x0306, 0x00},		/* PLL_VT_MPY[10:8] */
	{0x0307, 0x39},		/* PLL_VT_MPY[7:0] */
	{0x0309, 0x0A},		/* OPPXCK_DIV[4:0] */
	{0x030B, 0x01},		/* OPSYCK_DIV */
	{0x030C, 0x00},		/* PLL_OP_MPY[10:8] */
	{0x030D, 0x72},		/* PLL_OP_MPY[7:0] */
	{0x455E, 0x00},		/* CIS Tuning */
	{0x471E, 0x4B},		/* CIS Tuning */
	{0x4767, 0x0F},		/* CIS Tuning */
	{0x4750, 0x14},		/* CIS Tuning */
	{0x47B4, 0x14},		/* CIS Tuning */
	{0x4713, 0x30},
	{0x478b, 0x10},
	{0x478f, 0x10},
	{0x4793, 0x10},
	{0x4797, 0x0e},
	{0x479b, 0x0e},
	{IMX219_TABLE_END, 0x00}
};

/* MCLK:24MHz  1920x1080  30fps   MIPI LANE2 */
static const struct imx219_reg imx219_init_tab_1920_1080_30fps[] = {
	{0x30EB, 0x05},
	{0x30EB, 0x0C},
	{0x300A, 0xFF},
	{0x300B, 0xFF},
	{0x30EB, 0x05},
	{0x30EB, 0x09},
	{0x0114, 0x01},
	{0x0128, 0x00},
	{0x012A, 0x18},
	{0x012B, 0x00},
	{0x0160, 0x06},
	{0x0161, 0xE6},
	{0x0162, 0x0D},
	{0x0163, 0x78},
	{0x0164, 0x02},
	{0x0165, 0xA8},
	{0x0166, 0x0A},
	{0x0167, 0x27},
	{0x0168, 0x02},
	{0x0169, 0xB4},
	{0x016A, 0x06},
	{0x016B, 0xEB},
	{0x016C, 0x07},
	{0x016D, 0x80},
	{0x016E, 0x04},
	{0x016F, 0x38},
	{0x0170, 0x01},
	{0x0171, 0x01},
	{0x0174, 0x00},
	{0x0175, 0x00},
	{0x018C, 0x0A},
	{0x018D, 0x0A},
	{0x0301, 0x05},
	{0x0303, 0x01},
	{0x0304, 0x03},
	{0x0305, 0x03},
	{0x0306, 0x00},
	{0x0307, 0x39},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030C, 0x00},
	{0x030D, 0x72},
	{0x455E, 0x00},
	{0x471E, 0x4B},
	{0x4767, 0x0F},
	{0x4750, 0x14},
	{0x4540, 0x00},
	{0x47B4, 0x14},
	{IMX219_TABLE_END, 0x00}
};

static const struct imx219_reg start[] = {
	{0x0100, 0x01},		/* mode select streaming on */
	{IMX219_TABLE_END, 0x00}
};

static const struct imx219_reg stop[] = {
	{0x0100, 0x00},		/* mode select streaming off */
	{IMX219_TABLE_END, 0x00}
};

#define IMX219_TESTP_COLOUR_MIN		0
#define IMX219_TESTP_COLOUR_MAX		0x03ff
#define IMX219_TESTP_COLOUR_STEP	1

#define TEST_PATTERN_DISABLED	0
#define TEST_PATTERN_SOLID_COLOR	1
#define TEST_PATTERN_COLOR_BAR	2
#define TEST_PATTERN_FADE_TO_GREY_COLOR_BAR	3
#define TEST_PATTERN_PN9	4

static const int test_pattern_val[] = {
	TEST_PATTERN_DISABLED,
	TEST_PATTERN_SOLID_COLOR,
	TEST_PATTERN_COLOR_BAR,
	TEST_PATTERN_FADE_TO_GREY_COLOR_BAR,
	TEST_PATTERN_PN9,
};

static const char *const test_pattern_menu[] = {
	"Disabled",
	"Solid Color",
	"Color Bar",
	"Fade to Grey Color Bar",
	"PN9",
};

#define SIZEOF_I2C_TRANSBUF 32

struct imx219 {
	struct v4l2_subdev subdev;
	struct media_pad pad;
	struct v4l2_ctrl_handler ctrl_handler;
	struct clk *clk;
	int hflip;
	int vflip;
	u8 analogue_gain;
	u16 digital_gain;	/* bits 11:0 */
	u16 exposure_time;
	u16 test_pattern;
	struct v4l2_ctrl *hblank;
	struct v4l2_ctrl *vblank;
	struct v4l2_ctrl *pixel_rate;
	const struct imx219_mode *cur_mode;
	struct mutex		mutex;
	u32 cfg_num;
	u16 cur_vts;
	u32 module_index;
	const char *module_facing;
	const char *module_name;
	const char *len_name;
};

static const struct imx219_mode supported_modes[] = {
	{
		.bus_fmt = MEDIA_BUS_FMT_SRGGB10_1X10,
		.width = 1920,
		.height = 1080,
		.max_fps = {
			.numerator = 10000,
			.denominator = 300000,
		},
		.hts_def = 0x0d78 - IMX219_EXP_LINES_MARGIN,
		.vts_def = 0x06E3,
		.reg_list = imx219_init_tab_1920_1080_30fps,
		.hdr_mode = NO_HDR,
		.freq_idx = 0,
	},
	{
		.bus_fmt = MEDIA_BUS_FMT_SRGGB10_1X10,
		.width = 3280,
		.height = 2464,
		.max_fps = {
			.numerator = 10000,
			.denominator = 210000,
		},
		.hts_def = 0x0d78 - IMX219_EXP_LINES_MARGIN,
		.vts_def = 0x09c4,
		.reg_list = imx219_init_tab_3280_2464_21fps,
		.hdr_mode = NO_HDR,
		.freq_idx = 0,
	},
};

static struct imx219 *to_imx219(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct imx219, subdev);
}

/* Write registers up to 2 at a time */
static int reg_write(struct i2c_client *client, const u16 addr, const u32 len, const u32 data)
{
	u8 buf[6];

	if (len > 4)
		return -EINVAL;

	put_unaligned_be16(addr, buf);
	put_unaligned_be32(data << (8 * (4 - len)), buf + 2);
	if (i2c_master_send(client, buf, len + 2) != len + 2)
		return -EIO;

	return 0;
}

/* Read registers up to 2 at a time */
static int reg_read(struct i2c_client *client, const u16 addr, const u32 len, u32 *val)
{
	u8 addr_buf[2] = {addr >> 8, addr & 0xff};
	u8 data_buf[4] = { 0, };
	struct i2c_msg msgs[2];
	int ret;
	if (len > 4)
		return -EINVAL;

	/* Write register address */
	msgs[0].addr  = client->addr;
	msgs[0].flags = 0;
	msgs[0].len   = ARRAY_SIZE(addr_buf);
	msgs[0].buf   = addr_buf;

	/* Read data from register */
	msgs[1].addr  = client->addr,
	msgs[1].flags = I2C_M_RD,
	msgs[1].len   = len,
	msgs[1].buf   = &data_buf[4 - len],

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));

	if (ret != ARRAY_SIZE(msgs))
		return -EIO;

	if (ret < 0) {
		dev_warn(&client->dev, "Reading register %x from %x failed\n",
			 addr, client->addr);
		return -EIO;
	}

	*val = get_unaligned_be32(data_buf);

	return 0;
}

static int reg_write_table(struct i2c_client *client,
			   const struct imx219_reg table[])
{
	const struct imx219_reg *reg;
	int ret;

	for (reg = table; reg->addr != IMX219_TABLE_END; reg++) {
		ret = reg_write(client, reg->addr, REG_VALUE_08BIT, reg->val);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/* V4L2 subdev video operations */
static int imx219_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx219 *priv = to_imx219(client);
	u8 reg = 0x00;
	int ret;

	if (!enable)
		return reg_write_table(client, stop);

	ret = reg_write_table(client, priv->cur_mode->reg_list);
	if (ret)
		return ret;

	/* Handle flip/mirror */
	if (priv->hflip)
		reg |= 0x1;
	if (priv->vflip)
		reg |= 0x2;

	ret = reg_write(client, IMX219_REG_IMG_ORIENTATION, REG_VALUE_08BIT, reg);
	if (ret)
		return ret;

	/* Handle test pattern */
	if (priv->test_pattern) {
		ret = reg_write(client, IMX219_REG_TP,
				REG_VALUE_16BIT, priv->test_pattern);
		ret |= reg_write(client, IMX219_REG_TP_WINDOW_WIDTH,
				REG_VALUE_16BIT, priv->cur_mode->width);
		ret |= reg_write(client, IMX219_REG_TP_WINDOW_HEIGHT,
				REG_VALUE_16BIT, priv->cur_mode->height);
	} else {
		ret = reg_write(client, IMX219_REG_TP, REG_VALUE_16BIT, 0x0);
	}

	priv->cur_vts = priv->cur_mode->vts_def - IMX219_EXP_LINES_MARGIN;
	if (ret)
		return ret;

	return reg_write_table(client, start);
}

/* V4L2 subdev core operations */
static int imx219_s_power(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx219 *priv = to_imx219(client);

	mutex_lock(&priv->mutex);
	if (on)	{
		dev_dbg(&client->dev, "imx219 power on\n");
		clk_prepare_enable(priv->clk);
	} else if (!on) {
		dev_dbg(&client->dev, "imx219 power off\n");
		clk_disable_unprepare(priv->clk);
	}
	mutex_unlock(&priv->mutex);

	return 0;
}

static int imx219_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx219 *priv = to_imx219(client);
	const struct imx219_mode *mode = priv->cur_mode;

	mutex_lock(&priv->mutex);
	fi->interval = mode->max_fps;
	mutex_unlock(&priv->mutex);

	return 0;
}

static int imx219_s_ctrl(struct v4l2_ctrl *ctrl)
{ struct imx219 *priv =
	    container_of(ctrl->handler, struct imx219, ctrl_handler);
	struct i2c_client *client = v4l2_get_subdevdata(&priv->subdev);
	u32 reg;
	int ret;
	u16 value;
	u16 gain = 256;
	u16 a_gain = 256;
	u16 d_gain = 1;

	switch (ctrl->id) {
	case V4L2_CID_HFLIP:
		priv->hflip = ctrl->val;
		break;

	case V4L2_CID_VFLIP:
		priv->vflip = ctrl->val;
		break;

	case V4L2_CID_ANALOGUE_GAIN:
	case V4L2_CID_GAIN:
		/*
		 * hal transfer (gain * 256)  to kernel
		 * than divide into analog gain & digital gain in kernel
		 */

		gain = ctrl->val;
		if (gain < 256)
			gain = 256;
		if (gain > 43663)
			gain = 43663;
		if (gain >= 256 && gain <= 2728) {
			a_gain = gain;
			d_gain = 1 * 256;
		} else {
			a_gain = 2728;
			d_gain = (gain * 256) / a_gain;
		}

		/*
		 * Analog gain, reg range[0, 232], gain value[1, 10.66]
		 * reg = 256 - 256 / again
		 * a_gain here is 256 multify
		 * so the reg = 256 - 256 * 256 / a_gain
		 */
		priv->analogue_gain = (256 - (256 * 256) / a_gain);
		if (a_gain < 256)
			priv->analogue_gain = 0;
		if (priv->analogue_gain > 232)
			priv->analogue_gain = 232;

		/*
		 * Digital gain, reg range[256, 4095], gain rage[1, 16]
		 * reg = dgain * 256
		 */
		priv->digital_gain = d_gain;
		if (priv->digital_gain < 256)
			priv->digital_gain = 256;
		if (priv->digital_gain > 4095)
			priv->digital_gain = 4095;

		/*
		 * for bank A and bank B switch
		 * exposure time , gain, vts must change at the same time
		 * so the exposure & gain can reflect at the same frame
		 */

		ret = reg_write(client, IMX219_REG_ANA_GAIN_GLOBAL_A, REG_VALUE_08BIT, priv->analogue_gain);
		ret |= reg_write(client, IMX219_REG_DIG_GAIN_GLOBAL_A, REG_VALUE_16BIT, priv->digital_gain);

		return ret;

	case V4L2_CID_EXPOSURE:
		priv->exposure_time = ctrl->val;

		ret = reg_write(client, IMX219_REG_EXPOSURE, REG_VALUE_16BIT, priv->exposure_time);
		return ret;
		break;

	case V4L2_CID_TEST_PATTERN:
		priv->test_pattern = test_pattern_val[ctrl->val];
		ret = reg_write(client, IMX219_REG_TP, REG_VALUE_16BIT, priv->test_pattern);
		return ret;
		break;

	case V4L2_CID_TEST_PATTERN_RED:
		value = ctrl->val & 0xffff;
		ret = reg_write(client, IMX219_REG_TD_R, REG_VALUE_16BIT, value);
		return ret;
		break;

	case V4L2_CID_TEST_PATTERN_GREENR:
		value = ctrl->val & 0xffff;
		ret = reg_write(client, IMX219_REG_TD_GR, REG_VALUE_16BIT, value);
		return ret;

	case V4L2_CID_TEST_PATTERN_BLUE:
		value = ctrl->val & 0xffff;
		ret = reg_write(client, IMX219_REG_TD_B, REG_VALUE_16BIT, value);
		return ret;

	case V4L2_CID_TEST_PATTERN_GREENB:
		value = ctrl->val & 0xffff;
		ret = reg_write(client, IMX219_REG_TD_GB, REG_VALUE_16BIT, value);
		return ret;

	case V4L2_CID_VBLANK:
		if (ctrl->val < priv->cur_mode->vts_def)
			ctrl->val = priv->cur_mode->vts_def;
		if ((ctrl->val - IMX219_EXP_LINES_MARGIN) != priv->cur_vts)
			priv->cur_vts = ctrl->val - IMX219_EXP_LINES_MARGIN;
		ret = reg_write(client, IMX219_REG_FRM_LENGTH_A, REG_VALUE_16BIT, priv->cur_vts);
		return ret;

	default:
		return -EINVAL;
	}
	/* If enabled, apply settings immediately */
	ret = reg_read(client, IMX219_REG_MODE_SELECT,
			REG_VALUE_08BIT, &reg);

	if (ret) {
		dev_err(&client->dev, "failed to read mode select\n");
		return ret;
	}

	if ((reg & 0x1f) == 0x01)
		imx219_s_stream(&priv->subdev, 1);

	return 0;
}

static int imx219_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx219 *priv = to_imx219(client);

	if (code->index != 0)
		return -EINVAL;
	code->code = priv->cur_mode->bus_fmt;

	return 0;
}

static int imx219_enum_frame_sizes(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_frame_size_enum *fse)
{
	if (fse->index >= ARRAY_SIZE(supported_modes))
		return -EINVAL;

	if (fse->code != supported_modes[fse->index].bus_fmt)
		return -EINVAL;

	fse->min_width  = supported_modes[fse->index].width;
	fse->max_width  = supported_modes[fse->index].width;
	fse->max_height = supported_modes[fse->index].height;
	fse->min_height = supported_modes[fse->index].height;

	return 0;
}

static int imx219_get_reso_dist(const struct imx219_mode *mode,
				struct v4l2_mbus_framefmt *framefmt)
{
	return abs(mode->width - framefmt->width) +
	       abs(mode->height - framefmt->height);
}

static const struct imx219_mode *imx219_find_best_fit(
					struct v4l2_subdev_format *fmt)
{
	struct v4l2_mbus_framefmt *framefmt = &fmt->format;
	int dist;
	int cur_best_fit = 0;
	int cur_best_fit_dist = -1;
	int i;

	for (i = 0; i < ARRAY_SIZE(supported_modes); i++) {
		dist = imx219_get_reso_dist(&supported_modes[i], framefmt);
		if (cur_best_fit_dist == -1 || dist < cur_best_fit_dist) {
			cur_best_fit_dist = dist;
			cur_best_fit = i;
		}
	}

	return &supported_modes[cur_best_fit];
}

static void imx219_reset_colorspace(struct v4l2_mbus_framefmt *fmt)
{
	fmt->colorspace = V4L2_COLORSPACE_RAW;
	fmt->ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(fmt->colorspace);
	fmt->quantization = V4L2_MAP_QUANTIZATION_DEFAULT(true,
							  fmt->colorspace,
							  fmt->ycbcr_enc);
	fmt->xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(fmt->colorspace);
}

static int imx219_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx219 *priv = to_imx219(client);
	const struct imx219_mode *mode;
	s64 h_blank, v_blank, pixel_rate;
	u32 fps = 0;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	mutex_lock(&priv->mutex);
	mode = imx219_find_best_fit(fmt);
	fmt->format.code = mode->bus_fmt;
	fmt->format.width = mode->width;
	fmt->format.height = mode->height;
	fmt->format.field = V4L2_FIELD_NONE;

	imx219_reset_colorspace(&fmt->format);
	priv->cur_mode = mode;
	h_blank = mode->hts_def - mode->width;
	__v4l2_ctrl_modify_range(priv->hblank, h_blank,
					h_blank, 1, h_blank);
	v_blank = mode->vts_def - mode->height;
	__v4l2_ctrl_modify_range(priv->vblank, v_blank,
					IMX219_VTS_MAX - mode->height,
					1, v_blank);
	fps = DIV_ROUND_CLOSEST(mode->max_fps.denominator,
		mode->max_fps.numerator);
	pixel_rate = mode->vts_def * mode->hts_def * fps;
	__v4l2_ctrl_modify_range(priv->pixel_rate, pixel_rate,
					pixel_rate, 1, pixel_rate);
	mutex_unlock(&priv->mutex);

	return 0;
}

static int imx219_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx219 *priv = to_imx219(client);
	const struct imx219_mode *mode = priv->cur_mode;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	mutex_lock(&priv->mutex);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		fmt->format = *v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
	} else {
		fmt->format.width = mode->width;
		fmt->format.height = mode->height;
		fmt->format.code = mode->bus_fmt;
		fmt->format.field = V4L2_FIELD_NONE;
	}
	mutex_unlock(&priv->mutex);

	return 0;
}

static void imx219_get_module_inf(struct imx219 *imx219,
				  struct rkmodule_inf *inf)
{
	memset(inf, 0, sizeof(*inf));
	strlcpy(inf->base.sensor, IMX219_NAME, sizeof(inf->base.sensor));
	strlcpy(inf->base.module, imx219->module_name,
		sizeof(inf->base.module));
	strlcpy(inf->base.lens, imx219->len_name, sizeof(inf->base.lens));
}

static long imx219_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx219 *imx219 = to_imx219(client);
	struct rkmodule_hdr_cfg *hdr;
	const struct imx219_mode *mode;
	u32 i, h, w;
	long ret = 0;
	s64 dst_pixel_rate = 0;
	s32 dst_link_freq = 0;
	u32 fps = 0;
	u32 stream = 0;

	switch (cmd) {
	case RKMODULE_GET_MODULE_INFO:
		imx219_get_module_inf(imx219, (struct rkmodule_inf *)arg);
		break;
	case RKMODULE_GET_HDR_CFG:
		hdr = (struct rkmodule_hdr_cfg *)arg;
		if (imx219->cur_mode->hdr_mode == NO_HDR)
			hdr->esp.mode = HDR_NORMAL_VC;
		else
			hdr->esp.mode = HDR_ID_CODE;
		hdr->hdr_mode = imx219->cur_mode->hdr_mode;
		break;
	case RKMODULE_SET_HDR_CFG:
		hdr = (struct rkmodule_hdr_cfg *)arg;
		for (i = 0; i < ARRAY_SIZE(supported_modes); i++){
			if (supported_modes[i].hdr_mode == hdr->hdr_mode) {
				imx219->cur_mode = &supported_modes[i];
				break;
			}
		}
		if (i == ARRAY_SIZE(supported_modes)) {
			dev_err(&client->dev,
					"not find hdr mode:%d config\n",
					hdr->hdr_mode);
			ret = -EINVAL;
		} else {
			mode = imx219->cur_mode;
			w = mode->hts_def - mode->width;
			h = mode->vts_def - mode->height;
			__v4l2_ctrl_modify_range(imx219->hblank, w, w, 1, w);
			__v4l2_ctrl_modify_range(imx219->vblank, h,
				IMX219_VTS_MAX - mode->height,
				1, h);
			dst_link_freq = mode->freq_idx;
			fps = DIV_ROUND_CLOSEST(mode->max_fps.denominator,
					mode->max_fps.numerator);
			dst_pixel_rate = mode->vts_def * mode->hts_def * fps;
			__v4l2_ctrl_s_ctrl_int64(imx219->pixel_rate,
					dst_pixel_rate);
			imx219->cur_vts = mode->vts_def;
		}
		break;
	case RKMODULE_SET_QUICK_STREAM:
		stream = *((u32 *)arg);
		if (stream)
			ret = reg_write_table(client, start);
		else
			ret = reg_write_table(client, stop);
		break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long imx219_compat_ioctl32(struct v4l2_subdev *sd,
				  unsigned int cmd, unsigned long arg)
{
	void __user *up = compat_ptr(arg);
	struct rkmodule_inf *inf;
	struct rkmodule_hdr_cfg *hdr;
	long ret;
	u32 stream = 0;

	switch (cmd) {
	case RKMODULE_GET_MODULE_INFO:
		inf = kzalloc(sizeof(*inf), GFP_KERNEL);
		if (!inf) {
			ret = -ENOMEM;
			return ret;
		}

		ret = imx219_ioctl(sd, cmd, inf);
		if (!ret)
			ret = copy_to_user(up, inf, sizeof(*inf));
		kfree(inf);
		break;
	case RKMODULE_GET_HDR_CFG:
		hdr = kzalloc(sizeof(*hdr), GFP_KERNEL);
		if (!hdr) {
			ret = -ENOMEM;
			return ret;
		}

		ret = imx219_ioctl(sd, cmd, hdr);
		if (!ret)
			ret = copy_to_user(up, hdr, sizeof(*hdr));
		kfree(hdr);
		break;
	case RKMODULE_SET_HDR_CFG:
		hdr = kzalloc(sizeof(*hdr), GFP_KERNEL);
		if (!hdr) {
			ret = -ENOMEM;
			return ret;
		}

		ret = copy_from_user(hdr, up, sizeof(*hdr));
		if (!ret)
			ret = imx219_ioctl(sd, cmd, hdr);
		kfree(hdr);
		break;
	case RKMODULE_SET_QUICK_STREAM:
		ret = copy_from_user(&stream, up, sizeof(*up));
		if (!ret)
			ret = imx219_ioctl(sd, cmd, &stream);
		break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}
#endif

static int imx219_enum_frame_interval(struct v4l2_subdev *sd,
				       struct v4l2_subdev_pad_config *cfg,
				       struct v4l2_subdev_frame_interval_enum *fie)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx219 *priv = to_imx219(client);

	if (fie->index >= priv->cfg_num)
		return -EINVAL;

	if (fie->code != MEDIA_BUS_FMT_SRGGB10_1X10)
		return -EINVAL;

	mutex_lock(&priv->mutex);
	fie->width = supported_modes[fie->index].width;
	fie->height = supported_modes[fie->index].height;
	fie->interval = supported_modes[fie->index].max_fps;
	fie->reserved[0] = supported_modes[fie->index].hdr_mode;
	mutex_unlock(&priv->mutex);
	return 0;
}

static int imx219_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *config)
{
	u32 val = 0;

	val = 1 << (IMX219_LANES - 1) |
	      V4L2_MBUS_CSI2_CHANNEL_0 |
	      V4L2_MBUS_CSI2_CONTINUOUS_CLOCK;
	config->type = V4L2_MBUS_CSI2;
	config->flags = val;

	return 0;
}

/* Various V4L2 operations tables */
static struct v4l2_subdev_video_ops imx219_subdev_video_ops = {
	.s_stream = imx219_s_stream,
	.g_frame_interval = imx219_g_frame_interval,
	.g_mbus_config = imx219_g_mbus_config,
};

static struct v4l2_subdev_core_ops imx219_subdev_core_ops = {
	.s_power = imx219_s_power,
	.ioctl = imx219_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = imx219_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_pad_ops imx219_subdev_pad_ops = {
	.enum_mbus_code = imx219_enum_mbus_code,
	.enum_frame_size = imx219_enum_frame_sizes,
	.enum_frame_interval = imx219_enum_frame_interval,
	.set_fmt = imx219_set_fmt,
	.get_fmt = imx219_get_fmt,
};

static struct v4l2_subdev_ops imx219_subdev_ops = {
	.core = &imx219_subdev_core_ops,
	.video = &imx219_subdev_video_ops,
	.pad = &imx219_subdev_pad_ops,
};

static const struct v4l2_ctrl_ops imx219_ctrl_ops = {
	.s_ctrl = imx219_s_ctrl,
};

static int imx219_video_probe(struct i2c_client *client)
{
	struct v4l2_subdev *subdev = i2c_get_clientdata(client);
	u32 model_id, lot_id, chip_id, val;
	int ret;

	ret = imx219_s_power(subdev, 1);
	if (ret < 0)
		return ret;

	/* Check and show model, lot, and chip ID. */
	ret = reg_read(client, IMX219_REG_MODEL_ID,
			REG_VALUE_16BIT, &model_id);

	if (ret) {
		dev_err(&client->dev, "Failure to read Model ID %x\n",
				0x0000);
		goto done;
	}

	ret = reg_read(client, IMX219_REG_LOT_ID_H,
			REG_VALUE_08BIT, &val);

	if (ret) {
		dev_err(&client->dev, "Failure to read Lot ID (high byte)\n");
		goto done;
	}
	lot_id = val << 16;

	ret = reg_read(client, IMX219_REG_LOT_ID_M,
			REG_VALUE_08BIT, &val);

	if (ret) {
		dev_err(&client->dev, "Failure to read Lot ID (mid byte)\n");
		goto done;
	}
	lot_id |= val << 8;

	ret = reg_read(client, IMX219_REG_LOT_ID_L,
			REG_VALUE_08BIT, &val);

	if (ret) {
		dev_err(&client->dev, "Failure to read Lot ID (low byte)\n");
		goto done;
	}
	lot_id |= val;

	ret = reg_read(client, IMX219_REG_CHIP_ID,
			REG_VALUE_16BIT, &chip_id);

	if (ret) {
		dev_err(&client->dev, "Failure to read Chip ID\n");
		goto done;
	}

	if (model_id != IMX219_MODEL_ID) {
		dev_err(&client->dev, "Model ID: %x not supported!\n",
			model_id);
		ret = -ENODEV;
		goto done;
	}
	dev_info(&client->dev,
		 "Model ID 0x%04x, Lot ID 0x%06x, Chip ID 0x%04x\n",
		 model_id, lot_id, chip_id);
done:
	imx219_s_power(subdev, 0);
	return ret;
}

static int imx219_ctrls_init(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx219 *priv = to_imx219(client);
	const struct imx219_mode *mode = priv->cur_mode;
	s64 pixel_rate, h_blank, v_blank;
	int ret;
	u32 fps = 0;
	int i;

	v4l2_ctrl_handler_init(&priv->ctrl_handler, 14);

	priv->ctrl_handler.lock = &priv->mutex;

	v4l2_ctrl_new_std(&priv->ctrl_handler, &imx219_ctrl_ops,
			  V4L2_CID_HFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&priv->ctrl_handler, &imx219_ctrl_ops,
			  V4L2_CID_VFLIP, 0, 1, 1, 0);

	/* exposure */
	v4l2_ctrl_new_std(&priv->ctrl_handler, &imx219_ctrl_ops,
			  V4L2_CID_ANALOGUE_GAIN,
			  IMX219_ANALOGUE_GAIN_MIN,
			  IMX219_ANALOGUE_GAIN_MAX,
			  1, IMX219_ANALOGUE_GAIN_DEFAULT);
	v4l2_ctrl_new_std(&priv->ctrl_handler, &imx219_ctrl_ops,
			  V4L2_CID_GAIN,
			  IMX219_DIGITAL_GAIN_MIN,
			  IMX219_DIGITAL_GAIN_MAX, 1,
			  IMX219_DIGITAL_GAIN_DEFAULT);
	v4l2_ctrl_new_std(&priv->ctrl_handler, &imx219_ctrl_ops,
			  V4L2_CID_EXPOSURE,
			  IMX219_DIGITAL_EXPOSURE_MIN,
			  IMX219_DIGITAL_EXPOSURE_MAX, 1,
			  IMX219_DIGITAL_EXPOSURE_DEFAULT);

	/* blank */
	h_blank = mode->hts_def - mode->width;
	priv->hblank = v4l2_ctrl_new_std(&priv->ctrl_handler, NULL, V4L2_CID_HBLANK,
			  h_blank, h_blank, 1, h_blank);
	v_blank = mode->vts_def - mode->height;
	priv->vblank = v4l2_ctrl_new_std(&priv->ctrl_handler, NULL, V4L2_CID_VBLANK,
			  v_blank, v_blank, 1, v_blank);

	/* freq */
	v4l2_ctrl_new_int_menu(&priv->ctrl_handler, NULL, V4L2_CID_LINK_FREQ,
			       0, 0, link_freq_menu_items);
	fps = DIV_ROUND_CLOSEST(mode->max_fps.denominator,
		mode->max_fps.numerator);
	pixel_rate = mode->vts_def * mode->hts_def * fps;
	priv->pixel_rate = v4l2_ctrl_new_std(&priv->ctrl_handler, NULL,
			     V4L2_CID_PIXEL_RATE,
			     0, pixel_rate, 1, pixel_rate);

	v4l2_ctrl_new_std_menu_items(&priv->ctrl_handler, &imx219_ctrl_ops,
				     V4L2_CID_TEST_PATTERN,
				     ARRAY_SIZE(test_pattern_menu) - 1, 0, 0, test_pattern_menu);

	for (i = 0; i < 4; i++) {
		v4l2_ctrl_new_std(&priv->ctrl_handler, &imx219_ctrl_ops,
				V4L2_CID_TEST_PATTERN_RED + i,
				IMX219_TESTP_COLOUR_MIN,
				IMX219_TESTP_COLOUR_MAX,
				IMX219_TESTP_COLOUR_STEP,
				IMX219_TESTP_COLOUR_MAX);
	}
	priv->subdev.ctrl_handler = &priv->ctrl_handler;
	if (priv->ctrl_handler.error) {
		dev_err(&client->dev, "Error %d adding controls\n",
			priv->ctrl_handler.error);
		ret = priv->ctrl_handler.error;
		goto error;
	}

	ret = v4l2_ctrl_handler_setup(&priv->ctrl_handler);
	if (ret < 0) {
		dev_err(&client->dev, "Error %d setting default controls\n",
			ret);
		goto error;
	}

	return 0;
error:
	v4l2_ctrl_handler_free(&priv->ctrl_handler);
	return ret;
}

static int imx219_probe(struct i2c_client *client,
			const struct i2c_device_id *did)
{
	struct imx219 *priv;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct device *dev = &client->dev;
	struct device_node *node = dev->of_node;
	struct v4l2_subdev *sd;
	char facing[2];
	int ret;

	dev_info(dev, "driver version: %02x.%02x.%02x",
		DRIVER_VERSION >> 16,
		(DRIVER_VERSION & 0xff00) >> 8,
		DRIVER_VERSION & 0x00ff);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_warn(&adapter->dev,
			 "I2C-Adapter doesn't support I2C_FUNC_SMBUS_BYTE\n");
		return -EIO;
	}
	priv = devm_kzalloc(&client->dev, sizeof(struct imx219), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	ret = of_property_read_u32(node, RKMODULE_CAMERA_MODULE_INDEX,
				   &priv->module_index);
	ret |= of_property_read_string(node, RKMODULE_CAMERA_MODULE_FACING,
				       &priv->module_facing);
	ret |= of_property_read_string(node, RKMODULE_CAMERA_MODULE_NAME,
				       &priv->module_name);
	ret |= of_property_read_string(node, RKMODULE_CAMERA_LENS_NAME,
				       &priv->len_name);
	if (ret) {
		dev_err(dev, "could not get module information!\n");
		return -EINVAL;
	}

	priv->clk = devm_clk_get(&client->dev, NULL);
	if (IS_ERR(priv->clk)) {
		dev_info(&client->dev, "Error %ld getting clock\n",
			 PTR_ERR(priv->clk));
		return -EPROBE_DEFER;
	}

	/* 1920 * 1080 by default */
	priv->cur_mode = &supported_modes[0];
	priv->cfg_num = ARRAY_SIZE(supported_modes);

	v4l2_i2c_subdev_init(&priv->subdev, client, &imx219_subdev_ops);
	ret = imx219_ctrls_init(&priv->subdev);
	if (ret < 0)
		return ret;
	ret = imx219_video_probe(client);
	if (ret < 0)
		return ret;

	priv->subdev.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
		     V4L2_SUBDEV_FL_HAS_EVENTS;

	priv->pad.flags = MEDIA_PAD_FL_SOURCE;
	priv->subdev.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&priv->subdev.entity, 1, &priv->pad);
	if (ret < 0)
		return ret;

	mutex_init(&priv->mutex);

	sd = &priv->subdev;
	memset(facing, 0, sizeof(facing));
	if (strcmp(priv->module_facing, "back") == 0)
		facing[0] = 'b';
	else
		facing[0] = 'f';

	snprintf(sd->name, sizeof(sd->name), "m%02d_%s_%s %s",
		 priv->module_index, facing,
		 IMX219_NAME, dev_name(sd->dev));
	ret = v4l2_async_register_subdev_sensor_common(sd);
	if (ret < 0)
		goto err_destroy_mutex;

	return ret;
err_destroy_mutex:
	mutex_destroy(&priv->mutex);

	return ret;
}

static int imx219_remove(struct i2c_client *client)
{
	struct imx219 *priv = to_imx219(client);

	v4l2_async_unregister_subdev(&priv->subdev);
	media_entity_cleanup(&priv->subdev.entity);
	v4l2_ctrl_handler_free(&priv->ctrl_handler);

	mutex_destroy(&priv->mutex);
	return 0;
}

static const struct i2c_device_id imx219_id[] = {
	{"imx219", 0},
	{}
};

static const struct of_device_id imx219_of_match[] = {
	{ .compatible = "sony,imx219" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, imx219_of_match);

MODULE_DEVICE_TABLE(i2c, imx219_id);
static struct i2c_driver imx219_i2c_driver = {
	.driver = {
		.of_match_table = of_match_ptr(imx219_of_match),
		.name = IMX219_NAME,
	},
	.probe = imx219_probe,
	.remove = imx219_remove,
	.id_table = imx219_id,
};

module_i2c_driver(imx219_i2c_driver);
MODULE_DESCRIPTION("Sony IMX219 Camera driver");
MODULE_AUTHOR("Guennadi Liakhovetski <g.liakhovetski@gmx.de>");
MODULE_LICENSE("GPL v2");
