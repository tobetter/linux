/*
 * drivers/amlogic/media/vout/hdmitx/hdmi_common/hdmi_parameters.c
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

#include <linux/kernel.h>
#include <linux/amlogic/media/vout/hdmi_tx/hdmi_common.h>
#include <linux/amlogic/media/vout/hdmi_tx/hdmi_tx_module.h>
#include <linux/string.h>

struct modeline_table {
	/* resolutions */
	unsigned int horpixels;
	unsigned int verpixels;
	/* clock and frequency */
	unsigned int pixel_clock;
	unsigned int hor_freq;
	unsigned int ver_freq;
	/* htimings */
	unsigned int hdisp;
	unsigned int hsyncstart;
	unsigned int hsyncend;
	unsigned int htotal;
	/* vtiminigs */
	unsigned int vdisp;
	unsigned int vsyncstart;
	unsigned int vsyncend;
	unsigned int vtotal;
	/* polarity and scan mode */
	unsigned int hsync_polarity; /* 1:+hsync, 0:-hsync */
	unsigned int vsync_polarity; /* 1:+vsync, 0:-vsync */
	unsigned int progress_mode; /* 1:progress, 0:interlaced */
};

static struct hdmi_format_para fmt_para_1920x1080p60_16x9 = {
	.vic = HDMI_1920x1080p60_16x9,
	.name = "1920x1080p60hz",
	.sname = "1080p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 148500,
	.timing = {
		.pixel_freq = 148500,
		.frac_freq = 148352,
		.h_freq = 67500,
		.v_freq = 60000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 1920,
		.h_total = 2200,
		.h_blank = 280,
		.h_front = 88,
		.h_sync = 44,
		.h_back = 148,
		.v_active = 1080,
		.v_total = 1125,
		.v_blank = 45,
		.v_front = 4,
		.v_sync = 5,
		.v_back = 36,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "1080p60hz",
		.mode              = VMODE_HDMI,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 148500000,
		.htotal            = 2200,
		.vtotal            = 1125,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_1920x1080p30_16x9 = {
	.vic = HDMI_1920x1080p30_16x9,
	.name = "1920x1080p30hz",
	.sname = "1080p30hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 74250,
	.timing = {
		.pixel_freq = 74250,
		.frac_freq = 74176,
		.h_freq = 67500,
		.v_freq = 30000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 1920,
		.h_total = 2200,
		.h_blank = 280,
		.h_front = 88,
		.h_sync = 44,
		.h_back = 148,
		.v_active = 1080,
		.v_total = 1125,
		.v_blank = 45,
		.v_front = 4,
		.v_sync = 5,
		.v_back = 36,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "1080p30hz",
		.mode              = VMODE_HDMI,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 30,
		.sync_duration_den = 1,
		.video_clk         = 74250000,
		.htotal            = 2200,
		.vtotal            = 1125,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_1920x1080p50_16x9 = {
	.vic = HDMI_1920x1080p50_16x9,
	.name = "1920x1080p50hz",
	.sname = "1080p50hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 148500,
	.timing = {
		.pixel_freq = 148500,
		.h_freq = 56250,
		.v_freq = 50000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 1920,
		.h_total = 2640,
		.h_blank = 720,
		.h_front = 528,
		.h_sync = 44,
		.h_back = 148,
		.v_active = 1080,
		.v_total = 1125,
		.v_blank = 45,
		.v_front = 4,
		.v_sync = 5,
		.v_back = 36,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "1080p50hz",
		.mode              = VMODE_HDMI,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 148500000,
		.htotal            = 2640,
		.vtotal            = 1125,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_1920x1080p25_16x9 = {
	.vic = HDMI_1920x1080p25_16x9,
	.name = "1920x1080p25hz",
	.sname = "1080p25hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 74250,
	.timing = {
		.pixel_freq = 74250,
		.h_freq = 56250,
		.v_freq = 50000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 1920,
		.h_total = 2640,
		.h_blank = 720,
		.h_front = 528,
		.h_sync = 44,
		.h_back = 148,
		.v_active = 1080,
		.v_total = 1125,
		.v_blank = 45,
		.v_front = 4,
		.v_sync = 5,
		.v_back = 36,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "1080p25hz",
		.mode              = VMODE_HDMI,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 25,
		.sync_duration_den = 1,
		.video_clk         = 74250000,
		.htotal            = 2640,
		.vtotal            = 1125,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_1920x1080p24_16x9 = {
	.vic = HDMI_1920x1080p24_16x9,
	.name = "1920x1080p24hz",
	.sname = "1080p24hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 74250,
	.timing = {
		.pixel_freq = 74250,
		.frac_freq = 74176,
		.h_freq = 27000,
		.v_freq = 24000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 1920,
		.h_total = 2750,
		.h_blank = 830,
		.h_front = 638,
		.h_sync = 44,
		.h_back = 148,
		.v_active = 1080,
		.v_total = 1125,
		.v_blank = 45,
		.v_front = 4,
		.v_sync = 5,
		.v_back = 36,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "1080p24hz",
		.mode              = VMODE_HDMI,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 24,
		.sync_duration_den = 1,
		.video_clk         = 74250000,
		.htotal            = 2750,
		.vtotal            = 1125,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_3840x2160p30_16x9 = {
	.vic = HDMI_3840x2160p30_16x9,
	.name = "3840x2160p30hz",
	.sname = "2160p30hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 297000,
	.timing = {
		.pixel_freq = 297000,
		.frac_freq = 296703,
		.h_freq = 67500,
		.v_freq = 30000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 3840,
		.h_total = 4400,
		.h_blank = 560,
		.h_front = 176,
		.h_sync = 88,
		.h_back = 296,
		.v_active = 2160,
		.v_total = 2250,
		.v_blank = 90,
		.v_front = 8,
		.v_sync = 10,
		.v_back = 72,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "2160p30hz",
		.mode              = VMODE_HDMI,
		.width             = 3840,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 30,
		.sync_duration_den = 1,
		.video_clk         = 297000000,
		.htotal            = 4400,
		.vtotal            = 2250,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_3840x2160p60_16x9 = {
	.vic = HDMI_3840x2160p60_16x9,
	.name = "3840x2160p60hz",
	.sname = "2160p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 1,
	.tmds_clk_div40 = 1,
	.tmds_clk = 594000,
	.timing = {
		.pixel_freq = 594000,
		.frac_freq = 593407,
		.h_freq = 135000,
		.v_freq = 60000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 3840,
		.h_total = 4400,
		.h_blank = 560,
		.h_front = 176,
		.h_sync = 88,
		.h_back = 296,
		.v_active = 2160,
		.v_total = 2250,
		.v_blank = 90,
		.v_front = 8,
		.v_sync = 10,
		.v_back = 72,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "2160p60hz",
		.mode              = VMODE_HDMI,
		.width             = 3840,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.htotal            = 4400,
		.vtotal            = 2250,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_3840x2160p50_16x9 = {
	.vic = HDMI_3840x2160p50_16x9,
	.name = "3840x2160p50hz",
	.sname = "2160p50hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 1,
	.tmds_clk_div40 = 1,
	.tmds_clk = 594000,
	.timing = {
		.pixel_freq = 594000,
		.h_freq = 112500,
		.v_freq = 50000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 3840,
		.h_total = 5280,
		.h_blank = 1440,
		.h_front = 1056,
		.h_sync = 88,
		.h_back = 296,
		.v_active = 2160,
		.v_total = 2250,
		.v_blank = 90,
		.v_front = 8,
		.v_sync = 10,
		.v_back = 72,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "2160p50hz",
		.mode              = VMODE_HDMI,
		.width             = 3840,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.htotal            = 5280,
		.vtotal            = 2250,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_3840x2160p25_16x9 = {
	.vic = HDMI_3840x2160p25_16x9,
	.name = "3840x2160p25hz",
	.sname = "2160p25hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 297000,
	.timing = {
		.pixel_freq = 297000,
		.h_freq = 56250,
		.v_freq = 25000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 3840,
		.h_total = 5280,
		.h_blank = 1440,
		.h_front = 1056,
		.h_sync = 88,
		.h_back = 296,
		.v_active = 2160,
		.v_total = 2250,
		.v_blank = 90,
		.v_front = 8,
		.v_sync = 10,
		.v_back = 72,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "2160p25hz",
		.mode              = VMODE_HDMI,
		.width             = 3840,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 25,
		.sync_duration_den = 1,
		.video_clk         = 297000000,
		.htotal            = 5280,
		.vtotal            = 2250,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_3840x2160p24_16x9 = {
	.vic = HDMI_3840x2160p24_16x9,
	.name = "3840x2160p24hz",
	.sname = "2160p24hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 297000,
	.timing = {
		.pixel_freq = 297000,
		.frac_freq = 296703,
		.h_freq = 54000,
		.v_freq = 24000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 3840,
		.h_total = 5500,
		.h_blank = 1660,
		.h_front = 1276,
		.h_sync = 88,
		.h_back = 296,
		.v_active = 2160,
		.v_total = 2250,
		.v_blank = 90,
		.v_front = 8,
		.v_sync = 10,
		.v_back = 72,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "2160p24hz",
		.mode              = VMODE_HDMI,
		.width             = 3840,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 24,
		.sync_duration_den = 1,
		.video_clk         = 297000000,
		.htotal            = 5500,
		.vtotal            = 2250,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_4096x2160p24_256x135 = {
	.vic = HDMI_4096x2160p24_256x135,
	.name = "4096x2160p24hz",
	.sname = "smpte24hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 297000,
	.timing = {
		.pixel_freq = 297000,
		.frac_freq = 296703,
		.h_freq = 54000,
		.v_freq = 24000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 4096,
		.h_total = 5500,
		.h_blank = 1404,
		.h_front = 1020,
		.h_sync = 88,
		.h_back = 296,
		.v_active = 2160,
		.v_total = 2250,
		.v_blank = 90,
		.v_front = 8,
		.v_sync = 10,
		.v_back = 72,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "smpte24hz",
		.mode              = VMODE_HDMI,
		.width             = 4096,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 24,
		.sync_duration_den = 1,
		.video_clk         = 297000000,
		.htotal            = 5500,
		.vtotal            = 2250,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_4096x2160p25_256x135 = {
	.vic = HDMI_4096x2160p25_256x135,
	.name = "4096x2160p25hz",
	.sname = "smpte25hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 297000,
	.timing = {
		.pixel_freq = 297000,
		.h_freq = 56250,
		.v_freq = 25000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 4096,
		.h_total = 5280,
		.h_blank = 1184,
		.h_front = 968,
		.h_sync = 88,
		.h_back = 128,
		.v_active = 2160,
		.v_total = 2250,
		.v_blank = 90,
		.v_front = 8,
		.v_sync = 10,
		.v_back = 72,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "smpte25hz",
		.mode              = VMODE_HDMI,
		.width             = 4096,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 25,
		.sync_duration_den = 1,
		.video_clk         = 297000000,
		.htotal            = 5280,
		.vtotal            = 2250,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_4096x2160p30_256x135 = {
	.vic = HDMI_4096x2160p30_256x135,
	.name = "4096x2160p30hz",
	.sname = "smpte30hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 297000,
	.timing = {
		.pixel_freq = 297000,
		.frac_freq = 296703,
		.h_freq = 67500,
		.v_freq = 30000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 4096,
		.h_total = 4400,
		.h_blank = 304,
		.h_front = 88,
		.h_sync = 88,
		.h_back = 128,
		.v_active = 2160,
		.v_total = 2250,
		.v_blank = 90,
		.v_front = 8,
		.v_sync = 10,
		.v_back = 72,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "smpte30hz",
		.mode              = VMODE_HDMI,
		.width             = 4096,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 30,
		.sync_duration_den = 1,
		.video_clk         = 297000000,
		.htotal            = 4400,
		.vtotal            = 2250,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_4096x2160p50_256x135 = {
	.vic = HDMI_4096x2160p50_256x135,
	.name = "4096x2160p50hz",
	.sname = "smpte50hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 1,
	.tmds_clk_div40 = 1,
	.tmds_clk = 594000,
	.timing = {
		.pixel_freq = 594000,
		.h_freq = 112500,
		.v_freq = 50000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 4096,
		.h_total = 5280,
		.h_blank = 1184,
		.h_front = 968,
		.h_sync = 88,
		.h_back = 128,
		.v_active = 2160,
		.v_total = 2250,
		.v_blank = 90,
		.v_front = 8,
		.v_sync = 10,
		.v_back = 72,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "smpte50hz",
		.mode              = VMODE_HDMI,
		.width             = 4096,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.htotal            = 5280,
		.vtotal            = 2250,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_4096x2160p60_256x135 = {
	.vic = HDMI_4096x2160p60_256x135,
	.name = "4096x2160p60hz",
	.sname = "smpte60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 1,
	.tmds_clk_div40 = 1,
	.tmds_clk = 594000,
	.timing = {
		.pixel_freq = 594000,
		.frac_freq = 593407,
		.h_freq = 135000,
		.v_freq = 60000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 4096,
		.h_total = 4400,
		.h_blank = 304,
		.h_front = 88,
		.h_sync = 88,
		.h_back = 128,
		.v_active = 2160,
		.v_total = 2250,
		.v_blank = 90,
		.v_front = 8,
		.v_sync = 10,
		.v_back = 72,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "smpte60hz",
		.mode              = VMODE_HDMI,
		.width             = 4096,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.htotal            = 4400,
		.vtotal            = 2250,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_1920x1080i60_16x9 = {
	.vic = HDMI_1920x1080i60_16x9,
	.name = "1920x1080i60hz",
	.sname = "1080i60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 0,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 74250,
	.timing = {
		.pixel_freq = 74250,
		.frac_freq = 74176,
		.h_freq = 33750,
		.v_freq = 60000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 1920,
		.h_total = 2200,
		.h_blank = 280,
		.h_front = 88,
		.h_sync = 44,
		.h_back = 148,
		.v_active = 1080/2,
		.v_total = 1125,
		.v_blank = 45/2,
		.v_front = 2,
		.v_sync = 5,
		.v_back = 15,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "1080i60hz",
		.mode              = VMODE_HDMI,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 540,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 148500000,
		.htotal            = 2200,
		.vtotal            = 1125,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_1920x1080i50_16x9 = {
	.vic = HDMI_1920x1080i50_16x9,
	.name = "1920x1080i50hz",
	.sname = "1080i50hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 0,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 74250,
	.timing = {
		.pixel_freq = 74250,
		.h_freq = 28125,
		.v_freq = 50000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 1920,
		.h_total = 2640,
		.h_blank = 720,
		.h_front = 528,
		.h_sync = 44,
		.h_back = 148,
		.v_active = 1080/2,
		.v_total = 1125,
		.v_blank = 45/2,
		.v_front = 2,
		.v_sync = 5,
		.v_back = 15,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "1080i50hz",
		.mode              = VMODE_HDMI,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 540,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 148500000,
		.htotal            = 2640,
		.vtotal            = 1125,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_1280x720p60_16x9 = {
	.vic = HDMI_1280x720p60_16x9,
	.name = "1280x720p60hz",
	.sname = "720p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 74250,
	.timing = {
		.pixel_freq = 74250,
		.frac_freq = 74176,
		.h_freq = 45000,
		.v_freq = 60000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 1280,
		.h_total = 1650,
		.h_blank = 370,
		.h_front = 110,
		.h_sync = 40,
		.h_back = 220,
		.v_active = 720,
		.v_total = 750,
		.v_blank = 30,
		.v_front = 5,
		.v_sync = 5,
		.v_back = 20,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "720p60hz",
		.mode              = VMODE_HDMI,
		.width             = 1280,
		.height            = 720,
		.field_height      = 720,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 74250000,
		.htotal            = 1650,
		.vtotal            = 750,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_1280x720p50_16x9 = {
	.vic = HDMI_1280x720p50_16x9,
	.name = "1280x720p50hz",
	.sname = "720p50hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 74250,
	.timing = {
		.pixel_freq = 74250,
		.h_freq = 37500,
		.v_freq = 50000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 1280,
		.h_total = 1980,
		.h_blank = 700,
		.h_front = 440,
		.h_sync = 40,
		.h_back = 220,
		.v_active = 720,
		.v_total = 750,
		.v_blank = 30,
		.v_front = 5,
		.v_sync = 5,
		.v_back = 20,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "720p50hz",
		.mode              = VMODE_HDMI,
		.width             = 1280,
		.height            = 720,
		.field_height      = 720,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 74250000,
		.htotal            = 1980,
		.vtotal            = 750,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_720x480p60_16x9 = {
	.vic = HDMI_720x480p60_16x9,
	.name = "720x480p60hz",
	.sname = "480p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 27027,
	.timing = {
		.pixel_freq = 27027,
		.frac_freq = 27000,
		.h_freq = 31469,
		.v_freq = 59940,
		.vsync_polarity = 0,
		.hsync_polarity = 0,
		.h_active = 720,
		.h_total = 858,
		.h_blank = 138,
		.h_front = 16,
		.h_sync = 62,
		.h_back = 60,
		.v_active = 480,
		.v_total = 525,
		.v_blank = 45,
		.v_front = 9,
		.v_sync = 6,
		.v_back = 30,
		.v_sync_ln = 7,
	},
	.hdmitx_vinfo = {
		.name              = "480p60hz",
		.mode              = VMODE_HDMI,
		.width             = 720,
		.height            = 480,
		.field_height      = 480,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 27000000,
		.htotal            = 858,
		.vtotal            = 525,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_720x480i60_16x9 = {
	.vic = HDMI_720x480i60_16x9,
	.name = "720x480i60hz",
	.sname = "480i60hz",
	.pixel_repetition_factor = 1,
	.progress_mode = 0,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 27027,
	.timing = {
		.pixel_freq = 27027,
		.frac_freq = 27000,
		.h_freq = 15734,
		.v_freq = 59940,
		.vsync_polarity = 0,
		.hsync_polarity = 0,
		.h_active = 1440,
		.h_total = 1716,
		.h_blank = 276,
		.h_front = 38,
		.h_sync = 124,
		.h_back = 114,
		.v_active = 480/2,
		.v_total = 525,
		.v_blank = 45/2,
		.v_front = 4,
		.v_sync = 3,
		.v_back = 15,
		.v_sync_ln = 4,
	},
	.hdmitx_vinfo = {
		.name              = "480i60hz",
		.mode              = VMODE_HDMI,
		.width             = 720,
		.height            = 480,
		.field_height      = 240,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 27000000,
		.htotal            = 1716,
		.vtotal            = 525,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCI,
	},
};

static struct hdmi_format_para fmt_para_720x576p50_16x9 = {
	.vic = HDMI_720x576p50_16x9,
	.name = "720x576p50hz",
	.sname = "576p50hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 27000,
	.timing = {
		.pixel_freq = 27000,
		.h_freq = 31250,
		.v_freq = 50000,
		.vsync_polarity = 0,
		.hsync_polarity = 0,
		.h_active = 720,
		.h_total = 864,
		.h_blank = 144,
		.h_front = 12,
		.h_sync = 64,
		.h_back = 68,
		.v_active = 576,
		.v_total = 625,
		.v_blank = 49,
		.v_front = 5,
		.v_sync = 5,
		.v_back = 39,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "576p50hz",
		.mode              = VMODE_HDMI,
		.width             = 720,
		.height            = 576,
		.field_height      = 576,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 27000000,
		.htotal            = 864,
		.vtotal            = 625,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_720x576i50_16x9 = {
	.vic = HDMI_720x576i50_16x9,
	.name = "720x576i50hz",
	.sname = "576i50hz",
	.pixel_repetition_factor = 1,
	.progress_mode = 0,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 27000,
	.timing = {
		.pixel_freq = 27000,
		.h_freq = 15625,
		.v_freq = 50000,
		.vsync_polarity = 0,
		.hsync_polarity = 0,
		.h_active = 1440,
		.h_total = 1728,
		.h_blank = 288,
		.h_front = 24,
		.h_sync = 126,
		.h_back = 138,
		.v_active = 576/2,
		.v_total = 625,
		.v_blank = 49/2,
		.v_front = 2,
		.v_sync = 3,
		.v_back = 19,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "576i50hz",
		.mode              = VMODE_HDMI,
		.width             = 720,
		.height            = 576,
		.field_height      = 288,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 27000000,
		.htotal            = 1728,
		.vtotal            = 625,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCI,
	},
};

/* the following are for Y420 mode*/

static struct hdmi_format_para fmt_para_3840x2160p50_16x9_y420 = {
	.vic = HDMI_3840x2160p50_16x9_Y420,
	.name = "3840x2160p50hz420",
	.sname = "2160p50hz420",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 594000,
	.timing = {
		.pixel_freq = 594000,
		.h_freq = 112500,
		.v_freq = 50000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 3840,
		.h_total = 5280,
		.h_blank = 1440,
		.h_front = 1056,
		.h_sync = 88,
		.h_back = 296,
		.v_active = 2160,
		.v_total = 2250,
		.v_blank = 90,
		.v_front = 8,
		.v_sync = 10,
		.v_back = 72,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "2160p50hz420",
		.mode              = VMODE_HDMI,
		.width             = 3840,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.htotal            = 5280,
		.vtotal            = 2250,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_4096x2160p50_256x135_y420 = {
	.vic = HDMI_4096x2160p50_256x135_Y420,
	.name = "4096x2160p50hz420",
	.sname = "smpte50hz420",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 594000,
	.timing = {
		.pixel_freq = 594000,
		.h_freq = 112500,
		.v_freq = 50000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 4096,
		.h_total = 5280,
		.h_blank = 1184,
		.h_front = 968,
		.h_sync = 88,
		.h_back = 296,
		.v_active = 2160,
		.v_total = 2250,
		.v_blank = 90,
		.v_front = 8,
		.v_sync = 10,
		.v_back = 72,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "smpte50hz420",
		.mode              = VMODE_HDMI,
		.width             = 4096,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.htotal            = 5280,
		.vtotal            = 2250,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_3840x2160p60_16x9_y420 = {
	.vic = HDMI_3840x2160p60_16x9_Y420,
	.name = "3840x2160p60hz420",
	.sname = "2160p60hz420",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 594000,
	.timing = {
		.pixel_freq = 594000,
		.frac_freq = 593407,
		.h_freq = 135000,
		.v_freq = 60000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 3840,
		.h_total = 4400,
		.h_blank = 560,
		.h_front = 176,
		.h_sync = 88,
		.h_back = 296,
		.v_active = 2160,
		.v_total = 2250,
		.v_blank = 90,
		.v_front = 8,
		.v_sync = 10,
		.v_back = 72,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "2160p60hz420",
		.mode              = VMODE_HDMI,
		.width             = 3840,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.htotal            = 4400,
		.vtotal            = 2250,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};


static struct hdmi_format_para fmt_para_4096x2160p60_256x135_y420 = {
	.vic = HDMI_4096x2160p60_256x135_Y420,
	.name = "4096x2160p60hz420",
	.sname = "smpte60hz420",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 594000,
	.timing = {
		.pixel_freq = 594000,
		.frac_freq = 593407,
		.h_freq = 135000,
		.v_freq = 60000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 4096,
		.h_total = 4400,
		.h_blank = 304,
		.h_front = 88,
		.h_sync = 88,
		.h_back = 128,
		.v_active = 2160,
		.v_total = 2250,
		.v_blank = 90,
		.v_front = 8,
		.v_sync = 10,
		.v_back = 72,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "smpte60hz420",
		.mode              = VMODE_HDMI,
		.width             = 4096,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.htotal            = 4400,
		.vtotal            = 2250,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

/* new display modes for odroid-n2 */
static struct hdmi_format_para fmt_para_2560x1600p60_8x5 = {
	.vic = HDMI_2560x1600p60_8x5,
	.name = "2560x1600p60hz",
	.sname = "2560x1600p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk =  268500,
	.timing = {
		.pixel_freq = 268500,
		.frac_freq = 268500,
		.h_freq = 98700,
		.v_freq = 60000,
		.vsync_polarity = 0,
		.hsync_polarity = 1,
		.h_active = 2560,
		.h_total = 2720,
		.h_blank = 160,
		.h_front = 48,
		.h_sync = 32,
		.h_back = 80,
		.v_active = 1600,
		.v_total = 1646,
		.v_blank = 46,
		.v_front = 3,
		.v_sync = 6,
		.v_back = 38,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "2560x1600p60hz",
		.mode              = VMODE_HDMI,
		.width             = 2560,
		.height            = 1600,
		.field_height      = 1600,
		.aspect_ratio_num  = 8,
		.aspect_ratio_den  = 5,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 268500000,
		.htotal            = 2720,
		.vtotal            = 1646,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_2560x1440p60_16x9 = {
	.vic = HDMI_2560x1440p60_16x9,
	.name = "2560x1440p60hz",
	.sname = "2560x1440p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk =  241500,
	.timing = {
		.pixel_freq = 241500,
		.frac_freq = 241500,
		.h_freq = 88790,
		.v_freq = 60000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 2560,
		.h_total = 2720,
		.h_blank = 160,
		.h_front = 48,
		.h_sync = 32,
		.h_back = 80,
		.v_active = 1440,
		.v_total = 1481,
		.v_blank = 41,
		.v_front = 2,
		.v_sync = 5,
		.v_back = 34,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "2560x1440p60hz",
		.mode              = VMODE_HDMI,
		.width             = 2560,
		.height            = 1440,
		.field_height      = 1440,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 241500000,
		.htotal            = 2720,
		.vtotal            = 1481,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_2560x1080p60_64x27 = {
	.vic = HDMI_2560x1080p60_64x27,
	.name = "2560x1080p60hz",
	.sname = "2560x1080p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 185580,
	.timing = {
		.pixel_freq = 185580,
		.frac_freq = 185580,
		.h_freq = 66659,
		.v_freq = 60000,
		.vsync_polarity = 0,
		.hsync_polarity = 1,
		.h_active = 2560,
		.h_total = 2784,
		.h_blank = 224,
		.h_front = 64,
		.h_sync = 64,
		.h_back = 96,
		.v_active = 1080,
		.v_total = 1111,
		.v_blank = 31,
		.v_front = 3,
		.v_sync = 10,
		.v_back = 18,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "2560x1080p60hz",
		.mode              = VMODE_HDMI,
		.width             = 2560,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 64,
		.aspect_ratio_den  = 27,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 185580000,
		.htotal            = 2784,
		.vtotal            = 1111,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_1920x1200p60_8x5 = {
	.vic = HDMI_1920x1200p60_8x5,
	.name = "1920x1200p60hz",
	.sname = "1920x1200p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 154000,
	.timing = {
		.pixel_freq = 154000,
		.frac_freq = 154000,
		.h_freq = 74040,
		.v_freq = 60000,
		.vsync_polarity = 0,
		.hsync_polarity = 1,
		.h_active = 1920,
		.h_total = 2080,
		.h_blank = 160,
		.h_front = 48,
		.h_sync = 32,
		.h_back = 80,
		.v_active = 1200,
		.v_total = 1235,
		.v_blank = 35,
		.v_front = 3,
		.v_sync = 6,
		.v_back = 26,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "1920x1200p60hz",
		.mode              = VMODE_HDMI,
		.width             = 1920,
		.height            = 1200,
		.field_height      = 1200,
		.aspect_ratio_num  = 8,
		.aspect_ratio_den  = 5,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 154000000,
		.htotal            = 2080,
		.vtotal            = 1235,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_1680x1050p60_8x5 = {
	.vic = HDMI_1680x1050p60_8x5,
	.name = "1680x1050p60hz",
	.sname = "1680x1050p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 146250,
	.timing = {
		.pixel_freq = 146250,
		.frac_freq = 146250,
		.h_freq = 65340,
		.v_freq = 60000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 1680,
		.h_total = 2240,
		.h_blank = 560,
		.h_front = 104,
		.h_sync = 176,
		.h_back = 280,
		.v_active = 1050,
		.v_total = 1089,
		.v_blank = 39,
		.v_front = 3,
		.v_sync = 6,
		.v_back = 30,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "1680x1050p60hz",
		.mode              = VMODE_HDMI,
		.width             = 1680,
		.height            = 1050,
		.field_height      = 1050,
		.aspect_ratio_num  = 8,
		.aspect_ratio_den  = 5,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 146250000,
		.htotal            = 2240,
		.vtotal            = 1089,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_1600x1200p60_4x3 = {
	.vic = HDMI_1600x1200p60_4x3,
	.name = "1600x1200p60hz",
	.sname = "1600x1200p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 156000,
	.timing = {
		.pixel_freq = 156000,
		.frac_freq = 156000,
		.h_freq = 76200,
		.v_freq = 60000,
		.vsync_polarity = 0,
		.hsync_polarity = 0,
		.h_active = 1600,
		.h_total = 2048,
		.h_blank = 448,
		.h_front = 32,
		.h_sync = 160,
		.h_back = 256,
		.v_active = 1200,
		.v_total = 1270,
		.v_blank = 70,
		.v_front = 10,
		.v_sync = 8,
		.v_back = 52,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "1600x1200p60hz",
		.mode              = VMODE_HDMI,
		.width             = 1600,
		.height            = 1200,
		.field_height      = 1200,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 156000000,
		.htotal            = 2048,
		.vtotal            = 1270,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_1600x900p60_16x9 = {
	.vic = HDMI_1600x900p60_16x9,
	.name = "1600x900p60hz",
	.sname = "1600x900p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 108000,
	.timing = {
		.pixel_freq = 108000,
		.frac_freq = 108000,
		.h_freq = 60000,
		.v_freq = 60000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 1600,
		.h_total = 1800,
		.h_blank = 200,
		.h_front = 24,
		.h_sync = 80,
		.h_back = 96,
		.v_active = 900,
		.v_total = 1000,
		.v_blank = 100,
		.v_front = 1,
		.v_sync = 3,
		.v_back = 96,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "1600x900p60hz",
		.mode              = VMODE_HDMI,
		.width             = 1600,
		.height            = 900,
		.field_height      = 900,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 108000000,
		.htotal            = 1800,
		.vtotal            = 1000,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_1440x900p60_8x5 = {
	.vic = HDMI_1440x900p60_8x5,
	.name = "1440x900p60hz",
	.sname = "1440x900p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 106500,
	.timing = {
		.pixel_freq = 106500,
		.frac_freq = 106500,
		.h_freq = 56040,
		.v_freq = 60000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 1440,
		.h_total = 1904,
		.h_blank = 464,
		.h_front = 80,
		.h_sync = 152,
		.h_back = 232,
		.v_active = 900,
		.v_total = 934,
		.v_blank = 34,
		.v_front = 3,
		.v_sync = 6,
		.v_back = 25,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "1440x900p60hz",
		.mode              = VMODE_HDMI,
		.width             = 1440,
		.height            = 900,
		.field_height      = 900,
		.aspect_ratio_num  = 8,
		.aspect_ratio_den  = 5,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 106500000,
		.htotal            = 1904,
		.vtotal            = 934,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_1360x768p60_16x9 = {
	.vic = HDMI_1360x768p60_16x9,
	.name = "1360x768p60hz",
	.sname = "1360x768p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 85500,
	.timing = {
		.pixel_freq = 85500,
		.frac_freq = 85500,
		.h_freq = 47700,
		.v_freq = 60000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 1360,
		.h_total = 1792,
		.h_blank = 432,
		.h_front = 64,
		.h_sync = 112,
		.h_back = 256,
		.v_active = 768,
		.v_total = 795,
		.v_blank = 27,
		.v_front = 3,
		.v_sync = 6,
		.v_back = 18,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "1360x768p60hz",
		.mode              = VMODE_HDMI,
		.width             = 1360,
		.height            = 768,
		.field_height      = 768,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 85500000,
		.htotal            = 1792,
		.vtotal            = 795,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_1280x1024p60_5x4 = {
	.vic = HDMI_1280x1024p60_5x4,
	.name = "1280x1024p60hz",
	.sname = "1280x1024p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 108000,
	.timing = {
		.pixel_freq = 108000,
		.frac_freq = 108000,
		.h_freq = 64080,
		.v_freq = 60000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 1280,
		.h_total = 1688,
		.h_blank = 408,
		.h_front = 48,
		.h_sync = 112,
		.h_back = 248,
		.v_active = 1024,
		.v_total = 1068,
		.v_blank = 42,
		.v_front = 1,
		.v_sync = 3,
		.v_back = 38,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "1280x1024p60hz",
		.mode              = VMODE_HDMI,
		.width             = 1280,
		.height            = 1024,
		.field_height      = 1024,
		.aspect_ratio_num  = 5,
		.aspect_ratio_den  = 4,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 108000000,
		.htotal            = 1688,
		.vtotal            = 1068,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_1280x800p60_8x5 = {
	.vic = HDMI_1280x800p60_8x5,
	.name = "1280x800p60hz",
	.sname = "1280x800p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 83500,
	.timing = {
		.pixel_freq = 83500,
		.frac_freq = 83500,
		.h_freq = 49380,
		.v_freq = 60000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 1280,
		.h_total = 1440,
		.h_blank = 160,
		.h_front = 48,
		.h_sync = 32,
		.h_back = 80,
		.v_active = 800,
		.v_total = 823,
		.v_blank = 23,
		.v_front = 3,
		.v_sync = 6,
		.v_back = 14,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "1280x800p60hz",
		.mode              = VMODE_HDMI,
		.width             = 1280,
		.height            = 800,
		.field_height      = 800,
		.aspect_ratio_num  = 8,
		.aspect_ratio_den  = 5,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 83500000,
		.htotal            = 1440,
		.vtotal            = 823,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_1024x768p60_4x3 = {
	.vic = HDMI_1024x768p60_4x3,
	.name = "1024x768p60hz",
	.sname = "1024x768p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 79500,
	.timing = {
		.pixel_freq = 79500,
		.frac_freq = 79500,
		.h_freq = 48360,
		.v_freq = 60000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 1024,
		.h_total = 1344,
		.h_blank = 320,
		.h_front = 24,
		.h_sync = 136,
		.h_back = 160,
		.v_active = 768,
		.v_total = 806,
		.v_blank = 38,
		.v_front = 3,
		.v_sync = 6,
		.v_back = 29,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "1024x768p60hz",
		.mode              = VMODE_HDMI,
		.width             = 1024,
		.height            = 768,
		.field_height      = 768,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 79500000,
		.htotal            = 1344,
		.vtotal            = 806,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_1024x600p60_16x9 = {
	.vic = HDMI_1024x600p60_16x9,
	.name = "1024x600p60hz",
	.sname = "1024x600p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 50400,
	.timing = {
		.pixel_freq = 50400,
		.frac_freq = 50400,
		.h_freq = 38280,
		.v_freq = 60000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 1024,
		.h_total = 1344,
		.h_blank = 320,
		.h_front = 24,
		.h_sync = 136,
		.h_back = 160,
		.v_active = 600,
		.v_total = 638,
		.v_blank = 38,
		.v_front = 3,
		.v_sync = 6,
		.v_back = 29,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "1024x600p60hz",
		.mode              = VMODE_HDMI,
		.width             = 1024,
		.height            = 600,
		.field_height      = 600,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 50400000,
		.htotal            = 1344,
		.vtotal            = 638,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_800x600p60_4x3 = {
	.vic = HDMI_800x600p60_4x3,
	.name = "800x600p60hz",
	.sname = "800x600p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 39790,
	.timing = {
		.pixel_freq = 39790,
		.frac_freq = 39790,
		.h_freq = 37879,
		.v_freq = 60000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 800,
		.h_total = 1056,
		.h_blank = 256,
		.h_front = 40,
		.h_sync = 128,
		.h_back = 88,
		.v_active = 600,
		.v_total = 628,
		.v_blank = 28,
		.v_front = 1,
		.v_sync = 4,
		.v_back = 23,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "800x600p60hz",
		.mode              = VMODE_HDMI,
		.width             = 800,
		.height            = 600,
		.field_height      = 600,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 40000000,
		.htotal            = 1056,
		.vtotal            = 628,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_800x480p60_5x3 = {
	.vic = HDMI_800x480p60_5x3,
	.name = "800x480p60hz",
	.sname = "800x480p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 29760,
	.timing = {
		.pixel_freq = 29760,
		.frac_freq = 29760,
		.h_freq = 30000,
		.v_freq = 60000,
		.vsync_polarity = 1,
		.hsync_polarity = 1,
		.h_active = 800,
		.h_total = 992,
		.h_blank = 192,
		.h_front = 24,
		.h_sync = 72,
		.h_back = 96,
		.v_active = 480,
		.v_total = 500,
		.v_blank = 20,
		.v_front = 3,
		.v_sync = 7,
		.v_back = 10,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "800x480p60hz",
		.mode              = VMODE_HDMI,
		.width             = 800,
		.height            = 480,
		.field_height      = 480,
		.aspect_ratio_num  = 5,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 29760000,
		.htotal            = 992,
		.vtotal            = 500,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_640x480p60_4x3 = {
	.vic = HDMI_640x480p60_4x3,
	.name = "640x480p60hz",
	.sname = "640x480p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 25175,
	.timing = {
		.pixel_freq = 25175,
		.frac_freq = 25175,
		.h_freq = 26218,
		.v_freq = 59940,
		.vsync_polarity = 0,
		.hsync_polarity = 0,
		.h_active = 640,
		.h_total = 800,
		.h_blank = 160,
		.h_front = 16,
		.h_sync = 96,
		.h_back = 48,
		.v_active = 480,
		.v_total = 525,
		.v_blank = 45,
		.v_front = 10,
		.v_sync = 2,
		.v_back = 33,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "640x480p60hz",
		.mode              = VMODE_HDMI,
		.width             = 640,
		.height            = 480,
		.field_height      = 480,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 25175000,
		.htotal            = 800,
		.vtotal            = 525,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_480x320p60_4x3 = {
	.vic = HDMI_480x320p60_4x3,
	.name = "480x320p60hz",
	.sname = "480x320p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 25200,
	.timing = {
		.pixel_freq = 25200,
		.frac_freq = 25200,
		.h_freq = 31500,
		.v_freq = 60000,
		.vsync_polarity = 0, /* -VSync */
		.hsync_polarity = 0, /* -HSync */
		.h_active = 480,
		.h_total = 800,
		.h_blank = 320,
		.h_front = 120,
		.h_sync = 100,
		.h_back = 100,
		.v_active = 320,
		.v_total = 525,
		.v_blank = 205,
		.v_front = 8,
		.v_sync = 8,
		.v_back = 189,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "480x320p60hz",
		.mode              = VMODE_HDMI,
		.width             = 480,
		.height            = 320,
		.field_height      = 320,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 25200000,
		.htotal            = 800,
		.vtotal            = 525,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_480x272p60_4x3 = {
	.vic = HDMI_480x272p60_4x3,
	.name = "480x272p60hz",
	.sname = "480x272p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 25200,
	.timing = {
		.pixel_freq = 25200,
		.frac_freq = 25200,
		.h_freq = 31500,
		.v_freq = 60000,
		.vsync_polarity = 0, /* -VSync */
		.hsync_polarity = 0, /* -HSync */
		.h_active = 480,
		.h_total = 800,
		.h_blank = 320,
		.h_front = 120,
		.h_sync = 100,
		.h_back = 100,
		.v_active = 272,
		.v_total = 525,
		.v_blank = 253,
		.v_front = 8,
		.v_sync = 7,
		.v_back = 238,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "480x272p60hz",
		.mode              = VMODE_HDMI,
		.width             = 480,
		.height            = 272,
		.field_height      = 272,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 25200000,
		.htotal            = 800,
		.vtotal            = 525,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_480x800p60_4x3 = {
	.vic = HDMI_480x800p60_4x3,
	.name = "480x800p60hz",
	.sname = "480x800p60hz",
	.pixel_repetition_factor = 0,
	.progress_mode = 1,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.tmds_clk = 32000,
	.timing = {
		.pixel_freq = 32000,
		.frac_freq = 32000,
		.h_freq = 52600,
		.v_freq = 62300,
		.vsync_polarity = 0,
		.hsync_polarity = 0,
		.h_active = 480,
		.h_total = 608,
		.h_blank = 128,
		.h_front = 40,
		.h_sync = 48,
		.h_back = 40,
		.v_active = 800,
		.v_total = 845,
		.v_blank = 45,
		.v_front = 13,
		.v_sync = 3,
		.v_back = 29,
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name              = "480x800p60hz",
		.mode              = VMODE_HDMI,
		.width             = 480,
		.height            = 800,
		.field_height      = 800,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 32000000,
		.htotal            = 608,
		.vtotal            = 845,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_custombuilt = {
	.vic = HDMI_CUSTOMBUILT,
	.name = "custombuilt",
	.sname = "custombuilt",
	.pixel_repetition_factor = 0,
	.scrambler_en = 0,
	.tmds_clk_div40 = 0,
	.timing = {
		.v_sync_ln = 1,
	},
	.hdmitx_vinfo = {
		.name = "custombuilt",
		.mode = VMODE_HDMI,
		.aspect_ratio_num = 16,
		.aspect_ratio_den = 9,
		.sync_duration_den = 1,
		.viu_color_fmt = COLOR_FMT_YUV444,
		.viu_mux = VIU_MUX_ENCP,
	},
};

static struct hdmi_format_para fmt_para_non_hdmi_fmt = {
	.vic = HDMI_Unknown,
	.name = "invalid",
	.sname = "invalid",
	.hdmitx_vinfo = {
		.name              = "invalid",
		.mode              = VMODE_MAX,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 148500000,
		.htotal            = 2200,
		.vtotal            = 1125,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

/* null mode is used for HDMI, such as current mode is 1080p24hz
 * but want to switch to 1080p 23.976hz
 * so 'echo null > /sys/class/display/mode' firstly, then set
 * 'echo 1 > /sys/class/amhdmitx/amhdmitx0/frac_rate_policy'
 * and 'echo 1080p24hz > /sys/class/display/mode'
 */
static struct hdmi_format_para fmt_para_null_hdmi_fmt = {
	.vic = HDMI_Unknown,
	.name = "null",
	.sname = "null",
	.hdmitx_vinfo = {
		.name              = "null",
		.mode              = VMODE_HDMI,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 148500000,
		.htotal            = 2200,
		.vtotal            = 1125,
		.fr_adj_type       = VOUT_FR_ADJ_HDMI,
		.viu_color_fmt     = COLOR_FMT_YUV444,
		.viu_mux           = VIU_MUX_ENCP,
	},
};

/* end of Y420 modes*/

static struct hdmi_format_para *all_fmt_paras[] = {
	&fmt_para_3840x2160p60_16x9,
	&fmt_para_3840x2160p50_16x9,
	&fmt_para_3840x2160p30_16x9,
	&fmt_para_3840x2160p25_16x9,
	&fmt_para_3840x2160p24_16x9,
	&fmt_para_4096x2160p24_256x135,
	&fmt_para_4096x2160p25_256x135,
	&fmt_para_4096x2160p30_256x135,
	&fmt_para_4096x2160p50_256x135,
	&fmt_para_4096x2160p60_256x135,
	&fmt_para_1920x1080p25_16x9,
	&fmt_para_1920x1080p30_16x9,
	&fmt_para_1920x1080p50_16x9,
	&fmt_para_1920x1080p60_16x9,
	&fmt_para_1920x1080p24_16x9,
	&fmt_para_1920x1080i60_16x9,
	&fmt_para_1920x1080i50_16x9,
	&fmt_para_1280x720p60_16x9,
	&fmt_para_1280x720p50_16x9,
	&fmt_para_720x480p60_16x9,
	&fmt_para_720x480i60_16x9,
	&fmt_para_720x576p50_16x9,
	&fmt_para_720x576i50_16x9,
	&fmt_para_3840x2160p60_16x9_y420,
	&fmt_para_4096x2160p60_256x135_y420,
	&fmt_para_3840x2160p50_16x9_y420,
	&fmt_para_4096x2160p50_256x135_y420,
	&fmt_para_2560x1600p60_8x5,
	&fmt_para_2560x1440p60_16x9,
	&fmt_para_2560x1080p60_64x27,
	&fmt_para_1920x1200p60_8x5,
	&fmt_para_1680x1050p60_8x5,
	&fmt_para_1600x1200p60_4x3,
	&fmt_para_1600x900p60_16x9,
	&fmt_para_1440x900p60_8x5,
	&fmt_para_1360x768p60_16x9,
	&fmt_para_1280x1024p60_5x4,
	&fmt_para_1280x800p60_8x5,
	&fmt_para_1024x768p60_4x3,
	&fmt_para_1024x600p60_16x9,
	&fmt_para_800x600p60_4x3,
	&fmt_para_800x480p60_5x3,
	&fmt_para_640x480p60_4x3,
	&fmt_para_480x320p60_4x3,
	&fmt_para_480x272p60_4x3,
	&fmt_para_480x800p60_4x3,
	&fmt_para_custombuilt,
	&fmt_para_null_hdmi_fmt,
	&fmt_para_non_hdmi_fmt,
	NULL,
};

struct hdmi_format_para *hdmi_get_fmt_paras(enum hdmi_vic vic)
{
	int i;

	for (i = 0; all_fmt_paras[i] != NULL; i++) {
		if (vic == all_fmt_paras[i]->vic)
			return all_fmt_paras[i];
	}
	return &fmt_para_non_hdmi_fmt;
}

/*
void debug_modeline(struct modeline_table tbl)
{
	pr_info("modeline - horpixels %d\n", tbl.horpixels);
	pr_info("modeline - verpixels %d\n", tbl.verpixels);
	pr_info("modeline - pixel_clock %d\n", tbl.pixel_clock);
	pr_info("modeline - hor_freq %d\n", tbl.hor_freq);
	pr_info("modeline - ver_freq %d\n", tbl.ver_freq);
	pr_info("modeline - hdisp %d\n", tbl.hdisp);
	pr_info("modeline - hsyncstart %d\n", tbl.hsyncstart);
	pr_info("modeline - hsyncend %d\n", tbl.hsyncend);
	pr_info("modeline - htotal %d\n", tbl.htotal);
	pr_info("modeline - vdisp %d\n", tbl.vdisp);
	pr_info("modeline - vsyncstart %d\n", tbl.vsyncstart);
	pr_info("modeline - vsyncend %d\n", tbl.vsyncend);
	pr_info("modeline - vtotal %d\n", tbl.vtotal);
	pr_info("modeline - hsync_polarity %d\n", tbl.hsync_polarity);
	pr_info("modeline - vsync_polarity %d\n", tbl.vsync_polarity);
	pr_info("modeline - progress_mode %d\n", tbl.progress_mode);
}
*/

void debug_hdmi_fmt_param(struct hdmi_format_para param)
{
	/* timing */
	pr_info("fmt_para.timing\n");
	pr_info("   - pixel_freq %d, frac_freq %d\n",
			param.timing.pixel_freq, param.timing.frac_freq);
	pr_info("   - h_freq %d, v_freq %d\n",
			param.timing.h_freq, param.timing.v_freq);
	pr_info("   - hsync_polarity %d, vsync_polarity %d\n",
			param.timing.hsync_polarity,
			param.timing.vsync_polarity);
	pr_info("   - h_active %d, h_total %d\n",
			param.timing.h_active, param.timing.h_total);
	pr_info("   - h_blank %d, h_front %d, h_sync %d, h_back %d\n",
			param.timing.h_blank, param.timing.h_front,
			param.timing.h_sync, param.timing.h_back);
	pr_info("   - v_active %d, v_total %d\n",
			param.timing.v_active, param.timing.v_total);
	pr_info("   - v_blank %d, v_front %d, v_sync %d, v_back %d\n",
			param.timing.v_blank, param.timing.v_front,
			param.timing.v_sync, param.timing.v_back);
	pr_info("   - v_sync_ln %d\n", param.timing.v_sync_ln);

	/* hdmitx_vinfo */
	pr_info("fmt_para.hdmitx_vinfo\n");
	pr_info("   - name %s, mode %d\n",
			param.hdmitx_vinfo.name, param.hdmitx_vinfo.mode);
	pr_info("   - width %d, height %d, field_height %d\n",
			param.hdmitx_vinfo.width, param.hdmitx_vinfo.height,
			param.hdmitx_vinfo.field_height);
	pr_info("   - aspect_ratio_num %d, aspect_ratio_den %d\n",
			param.hdmitx_vinfo.aspect_ratio_num,
			param.hdmitx_vinfo.aspect_ratio_den);
	pr_info("   - video_clk %d\n", param.hdmitx_vinfo.video_clk);
	pr_info("   - htotal %d, vtotal %d\n",
			param.hdmitx_vinfo.htotal, param.hdmitx_vinfo.vtotal);
	pr_info("   - viu_color_fmt %d, viu_mux %d\n",
			param.hdmitx_vinfo.viu_color_fmt,
			param.hdmitx_vinfo.viu_mux);
}

/*
 * assuming modeline information from command line is as following.
 * setenv modeline
 * "horpixels,verpixels,pixel_clock,hor_freq,ver_freq
 * ,hdisp,hsyncstart,hsyncend,htotal,vdisp,vsyncstart,vsyncend,vtotal
 * ,hsync_polarity,vsync_polarity,progress_mode"
 */
static int __init setup_modeline(char *s)
{
	struct hdmi_cea_timing *custom_timing;
	struct modeline_table tbl;
	unsigned int *buf;
	char *item = NULL;
	unsigned long temp = 0;
	int ret;
	int i = 0;

	/* 1. parsing modeline information from command line */
	buf = (unsigned int *)&(tbl.horpixels);

	while (s != NULL) {
		item = strsep(&s, ",");
		ret = kstrtoul(item, 0, &temp);
		*(buf + i) = temp;
		i++;
	}

	/* check parameters */
	// debug_modeline(tbl);

	/* 2. build hdmi_format_para */
	fmt_para_custombuilt.progress_mode = tbl.progress_mode;
	fmt_para_custombuilt.tmds_clk = tbl.pixel_clock;

	/* timing */
	fmt_para_custombuilt.timing.pixel_freq = tbl.pixel_clock;
	fmt_para_custombuilt.timing.frac_freq = tbl.pixel_clock;
	fmt_para_custombuilt.timing.h_freq = tbl.hor_freq;
	fmt_para_custombuilt.timing.v_freq = (tbl.ver_freq * 1000);
	fmt_para_custombuilt.timing.hsync_polarity = tbl.hsync_polarity;
	fmt_para_custombuilt.timing.vsync_polarity = tbl.vsync_polarity;
	/* h_active = hdisp */
	fmt_para_custombuilt.timing.h_active = tbl.hdisp;
	/* h_total = htotal */
	fmt_para_custombuilt.timing.h_total =  tbl.htotal;
	/* h_blank = htotal - hdisp */
	fmt_para_custombuilt.timing.h_blank = tbl.htotal - tbl.hdisp;
	/* h_front = hsyncstart - hdisp */
	fmt_para_custombuilt.timing.h_front = tbl.hsyncstart - tbl.hdisp;
	/* h_sync = hsyncend - hsyncstart */
	fmt_para_custombuilt.timing.h_sync = tbl.hsyncend - tbl.hsyncstart;
	/* h_back = (h_blank - (h_front + h_sync))*/
	fmt_para_custombuilt.timing.h_back
		= fmt_para_custombuilt.timing.h_blank
		- fmt_para_custombuilt.timing.h_front
		- fmt_para_custombuilt.timing.h_sync;
	/* v_active = vdisp */
	fmt_para_custombuilt.timing.v_active = tbl.vdisp;
	/* v_total = vtotal */
	fmt_para_custombuilt.timing.v_total = tbl.vtotal;
	/* v_blank = vtotal - vdisp */
	fmt_para_custombuilt.timing.v_blank = tbl.vtotal - tbl.vdisp;
	/* v_front = vsyncstart - vdisp */
	fmt_para_custombuilt.timing.v_front = tbl.vsyncstart - tbl.vdisp;
	/* v_sync = vsyncend - vsyncstart */
	fmt_para_custombuilt.timing.v_sync = tbl.vsyncend - tbl.vsyncstart;
	/* v_back = (v_blank - (v_front + v_sync)) */
	fmt_para_custombuilt.timing.v_back
		= fmt_para_custombuilt.timing.v_blank
		- fmt_para_custombuilt.timing.v_front
		- fmt_para_custombuilt.timing.v_sync;
	fmt_para_custombuilt.timing.v_sync_ln = 1;

	/* hdmitx_vinfo */
	fmt_para_custombuilt.hdmitx_vinfo.width = tbl.hdisp;
	fmt_para_custombuilt.hdmitx_vinfo.height = tbl.vdisp;
	fmt_para_custombuilt.hdmitx_vinfo.field_height = tbl.vdisp;
	fmt_para_custombuilt.hdmitx_vinfo.sync_duration_num = tbl.ver_freq;
	fmt_para_custombuilt.hdmitx_vinfo.video_clk = (tbl.pixel_clock * 1000);
	fmt_para_custombuilt.hdmitx_vinfo.htotal = tbl.htotal;
	fmt_para_custombuilt.hdmitx_vinfo.vtotal = tbl.vtotal;

	/* check parameters */
	debug_hdmi_fmt_param(fmt_para_custombuilt);

	/* 3. copy custom-built timing information for backup */
	custom_timing = get_custom_timing();
	memcpy(custom_timing, &fmt_para_custombuilt.timing,
		sizeof(fmt_para_custombuilt.timing));

	return 0;
}
__setup("modeline=", setup_modeline);

struct hdmi_format_para *hdmi_match_dtd_paras(struct dtd *t)
{
	int i;

	if (!t)
		return NULL;
	for (i = 0; all_fmt_paras[i]; i++) {
		if ((abs(all_fmt_paras[i]->timing.pixel_freq / 10
		    - t->pixel_clock) <= (t->pixel_clock + 1000) / 1000) &&
		    (t->h_active == all_fmt_paras[i]->timing.h_active) &&
		    (t->h_blank == all_fmt_paras[i]->timing.h_blank) &&
		    (t->v_active == all_fmt_paras[i]->timing.v_active) &&
		    (t->v_blank == all_fmt_paras[i]->timing.v_blank) &&
		    (t->h_sync_offset == all_fmt_paras[i]->timing.h_front) &&
		    (t->h_sync == all_fmt_paras[i]->timing.h_sync) &&
		    (t->v_sync_offset == all_fmt_paras[i]->timing.v_front) &&
		    (t->v_sync == all_fmt_paras[i]->timing.v_sync)
		    )
			return all_fmt_paras[i];
	}

	return NULL;
}

static struct parse_cd parse_cd_[] = {
	{COLORDEPTH_24B, "8bit",},
	{COLORDEPTH_30B, "10bit"},
	{COLORDEPTH_36B, "12bit"},
	{COLORDEPTH_48B, "16bit"},
};

static struct parse_cs parse_cs_[] = {
	{COLORSPACE_RGB444, "rgb",},
	{COLORSPACE_YUV422, "422",},
	{COLORSPACE_YUV444, "444",},
	{COLORSPACE_YUV420, "420",},
};

static struct parse_cr parse_cr_[] = {
	{COLORRANGE_LIM, "limit",},
	{COLORRANGE_FUL, "full",},
};

const char *hdmi_get_str_cd(struct hdmi_format_para *para)
{
	int i;

	for (i = 0; i < sizeof(parse_cd_) / sizeof(struct parse_cd); i++) {
		if (para->cd == parse_cd_[i].cd)
			return parse_cd_[i].name;
	}
	return NULL;
}

const char *hdmi_get_str_cs(struct hdmi_format_para *para)
{
	int i;

	for (i = 0; i < sizeof(parse_cs_) / sizeof(struct parse_cs); i++) {
		if (para->cs == parse_cs_[i].cs)
			return parse_cs_[i].name;
	}
	return NULL;
}

const char *hdmi_get_str_cr(struct hdmi_format_para *para)
{
	int i;

	for (i = 0; i < sizeof(parse_cr_) / sizeof(struct parse_cr); i++) {
		if (para->cr == parse_cr_[i].cr)
			return parse_cr_[i].name;
	}
	return NULL;
}

/* parse the string from "dhmitx output FORMAT" */
static void hdmi_parse_attr(struct hdmi_format_para *para, char const *name)
{
	int i;

	/* parse color depth */
	for (i = 0; i < sizeof(parse_cd_) / sizeof(struct parse_cd); i++) {
		if (strstr(name, parse_cd_[i].name)) {
			para->cd = parse_cd_[i].cd;
			break;
		}
	}
	/* set default value */
	if (i == sizeof(parse_cd_) / sizeof(struct parse_cd))
		para->cd = COLORDEPTH_24B;

	/* parse color space */
	for (i = 0; i < sizeof(parse_cs_) / sizeof(struct parse_cs); i++) {
		if (strstr(name, parse_cs_[i].name)) {
			para->cs = parse_cs_[i].cs;
			break;
		}
	}
	/* set default value */
	if (i == sizeof(parse_cs_) / sizeof(struct parse_cs))
		para->cs = COLORSPACE_YUV444;

	/* parse color range */
	for (i = 0; i < sizeof(parse_cr_) / sizeof(struct parse_cr); i++) {
		if (strstr(name, parse_cr_[i].name)) {
			para->cr = parse_cr_[i].cr;
			break;
		}
	}
	/* set default value */
	if (i == sizeof(parse_cr_) / sizeof(struct parse_cr))
		para->cr = COLORRANGE_FUL;
}

/*
 * Parameter 'name' can be 1080p60hz, or 1920x1080p60hz
 * or 3840x2160p60hz, 2160p60hz
 * or 3840x2160p60hz420, 2160p60hz420 (Y420 mode)
 */
struct hdmi_format_para *hdmi_get_fmt_name(char const *name, char const *attr)
{
	int i;
	char *lname;
	enum hdmi_vic vic = HDMI_Unknown;
	struct hdmi_format_para *para = &fmt_para_non_hdmi_fmt;

	if (!name)
		return para;

	for (i = 0; all_fmt_paras[i]; i++) {
		lname = all_fmt_paras[i]->name;
		if (lname && (strncmp(name, lname, strlen(lname)) == 0)) {
			vic = all_fmt_paras[i]->vic;
			break;
		}
		lname = all_fmt_paras[i]->sname;
		if (lname && (strncmp(name, lname, strlen(lname)) == 0)) {
			vic = all_fmt_paras[i]->vic;
			break;
		}
	}
	if ((vic != HDMI_Unknown) && (i != sizeof(all_fmt_paras) /
		sizeof(struct hdmi_format_para *))) {
		para = all_fmt_paras[i];
		memset(&para->ext_name[0], 0, sizeof(para->ext_name));
		memcpy(&para->ext_name[0], name, sizeof(para->ext_name));
		hdmi_parse_attr(para, name);
		hdmi_parse_attr(para, attr);
	} else {
		para = &fmt_para_non_hdmi_fmt;
		hdmi_parse_attr(para, name);
		hdmi_parse_attr(para, attr);
	}
	if (strstr(name, "420"))
		para->cs = COLORSPACE_YUV420;

	/* only 2160p60/50hz smpte60/50hz have Y420 mode */
	if (para->cs == COLORSPACE_YUV420) {
		switch ((para->vic) & 0xff) {
		case HDMI_3840x2160p50_16x9:
		case HDMI_3840x2160p60_16x9:
		case HDMI_4096x2160p50_256x135:
		case HDMI_4096x2160p60_256x135:
		case HDMI_3840x2160p50_64x27:
		case HDMI_3840x2160p60_64x27:
			break;
		default:
			para = &fmt_para_non_hdmi_fmt;
			break;
		}
	}

	return para;
}

struct vinfo_s *hdmi_get_valid_vinfo(char *mode)
{
	int i;
	struct vinfo_s *info = NULL;
	char mode_[32];

	if (strlen(mode)) {
		/* the string of mode contains char NF */
		memset(mode_, 0, sizeof(mode_));
		strncpy(mode_, mode, sizeof(mode_));

		/* skip "f", 1080fp60hz -> 1080p60hz for 3d */
		mode_[31] = '\0';
		if (strstr(mode_, "fp")) {
			int i = 0;

			for (; mode_[i]; i++) {
				if ((mode_[i] == 'f') &&
					(mode_[i + 1] == 'p')) {
					do {
						mode_[i] = mode_[i + 1];
						i++;
					} while (mode_[i]);
					break;
				}
			}
		}

		for (i = 0; i < sizeof(mode_); i++)
			if (mode_[i] == 10)
				mode_[i] = 0;

		for (i = 0; all_fmt_paras[i]; i++) {
			if (all_fmt_paras[i]->hdmitx_vinfo.mode == VMODE_MAX)
				continue;
			if (strncmp(all_fmt_paras[i]->hdmitx_vinfo.name, mode_,
				strlen(mode_)) == 0) {
				info = &all_fmt_paras[i]->hdmitx_vinfo;
				break;
			}
		}
	}
	return info;
}

/* For check all format parameters only */
void check_detail_fmt(void)
{
	int i;
	struct hdmi_format_para *p;
	struct hdmi_cea_timing *t;

	pr_warn("VIC Hactive Vactive I/P Htotal Hblank Vtotal Vblank Hfreq Vfreq Pfreq\n");
	for (i = 0; all_fmt_paras[i] != NULL; i++) {
		p = all_fmt_paras[i];
		t = &p->timing;
		pr_warn("%s[%d] %d %d %c %d %d %d %d %d %d %d\n",
			all_fmt_paras[i]->name, all_fmt_paras[i]->vic,
			t->h_active, t->v_active,
			(p->progress_mode) ? 'P' : 'I',
			t->h_total, t->h_blank, t->v_total, t->v_blank,
			t->h_freq, t->v_freq, t->pixel_freq);
	}

	pr_warn("\nVIC Hfront Hsync Hback Hpol Vfront Vsync Vback Vpol Ln\n");
	for (i = 0; all_fmt_paras[i] != NULL; i++) {
		p = all_fmt_paras[i];
		t = &p->timing;
	pr_warn("%s[%d] %d %d %d %c %d %d %d %c %d\n",
		all_fmt_paras[i]->name, all_fmt_paras[i]->vic,
		t->h_front, t->h_sync, t->h_back,
		(t->hsync_polarity) ? 'P' : 'N',
		t->v_front, t->v_sync, t->v_back,
		(t->vsync_polarity) ? 'P' : 'N',
		t->v_sync_ln);
	}

	pr_warn("\nCheck Horizon parameter\n");
	for (i = 0; all_fmt_paras[i] != NULL; i++) {
		p = all_fmt_paras[i];
		t = &p->timing;
	if (t->h_total != (t->h_active + t->h_blank))
		pr_warn("VIC[%d] Ht[%d] != (Ha[%d] + Hb[%d])\n",
		all_fmt_paras[i]->vic, t->h_total, t->h_active,
		t->h_blank);
	if (t->h_blank != (t->h_front + t->h_sync + t->h_back))
		pr_warn("VIC[%d] Hb[%d] != (Hf[%d] + Hs[%d] + Hb[%d])\n",
			all_fmt_paras[i]->vic, t->h_blank,
			t->h_front, t->h_sync, t->h_back);
	}

	pr_warn("\nCheck Vertical parameter\n");
	for (i = 0; all_fmt_paras[i] != NULL; i++) {
		p = all_fmt_paras[i];
		t = &p->timing;
		if (t->v_total != (t->v_active + t->v_blank))
			pr_warn("VIC[%d] Vt[%d] != (Va[%d] + Vb[%d]\n",
				all_fmt_paras[i]->vic, t->v_total, t->v_active,
				t->v_blank);
	if ((t->v_blank != (t->v_front + t->v_sync + t->v_back))
		& (p->progress_mode == 1))
		pr_warn("VIC[%d] Vb[%d] != (Vf[%d] + Vs[%d] + Vb[%d])\n",
			all_fmt_paras[i]->vic, t->v_blank,
			t->v_front, t->v_sync, t->v_back);
	if ((t->v_blank/2 != (t->v_front + t->v_sync + t->v_back))
		& (p->progress_mode == 0))
		pr_warn("VIC[%d] Vb[%d] != (Vf[%d] + Vs[%d] + Vb[%d])\n",
			all_fmt_paras[i]->vic, t->v_blank, t->v_front,
			t->v_sync, t->v_back);
	}
}

/* Recommended N and Expected CTS for 32kHz */
struct hdmi_audio_fs_ncts aud_32k_para = {
	.array[0] = {
		.tmds_clk = 25175,
		.n = 4576,
		.cts = 28125,
		.n_36bit = 9152,
		.cts_36bit = 84375,
		.n_48bit = 4576,
		.cts_48bit = 56250,
	},
	.array[1] = {
		.tmds_clk = 25200,
		.n = 4096,
		.cts = 25200,
		.n_36bit = 4096,
		.cts_36bit = 37800,
		.n_48bit = 4096,
		.cts_48bit = 50400,
	},
	.array[2] = {
		.tmds_clk = 27000,
		.n = 4096,
		.cts = 27000,
		.n_36bit = 4096,
		.cts_36bit = 40500,
		.n_48bit = 4096,
		.cts_48bit = 54000,
	},
	.array[3] = {
		.tmds_clk = 27027,
		.n = 4096,
		.cts = 27027,
		.n_36bit = 8192,
		.cts_36bit = 81081,
		.n_48bit = 4096,
		.cts_48bit = 54054,
	},
	.array[4] = {
		.tmds_clk = 54000,
		.n = 4096,
		.cts = 54000,
		.n_36bit = 4096,
		.cts_36bit = 81000,
		.n_48bit = 4096,
		.cts_48bit = 108000,
	},
	.array[5] = {
		.tmds_clk = 54054,
		.n = 4096,
		.cts = 54054,
		.n_36bit = 4096,
		.cts_36bit = 81081,
		.n_48bit = 4096,
		.cts_48bit = 108108,
	},
	.array[6] = {
		.tmds_clk = 74176,
		.n = 11648,
		.cts = 210937,
		.n_36bit = 11648,
		.cts_36bit = 316406,
		.n_48bit = 11648,
		.cts_48bit = 421875,
	},
	.array[7] = {
		.tmds_clk = 74250,
		.n = 4096,
		.cts = 74250,
		.n_36bit = 4096,
		.cts_36bit = 111375,
		.n_48bit = 4096,
		.cts_48bit = 148500,
	},
	.array[8] = {
		.tmds_clk = 148352,
		.n = 11648,
		.cts = 421875,
		.n_36bit = 11648,
		.cts_36bit = 632812,
		.n_48bit = 11648,
		.cts_48bit = 843750,
	},
	.array[9] = {
		.tmds_clk = 148500,
		.n = 4096,
		.cts = 148500,
		.n_36bit = 4096,
		.cts_36bit = 222750,
		.n_48bit = 4096,
		.cts_48bit = 297000,
	},
	.array[10] = {
		.tmds_clk = 296703,
		.n = 5824,
		.cts = 421875,
	},
	.array[11] = {
		.tmds_clk = 297000,
		.n = 3072,
		.cts = 222750,
	},
	.array[12] = {
		.tmds_clk = 593407,
		.n = 5824,
		.cts = 843750,
	},
	.array[13] = {
		.tmds_clk = 594000,
		.n = 3072,
		.cts = 445500,
	},
	.def_n = 4096,
};

/* Recommended N and Expected CTS for 44.1kHz and Multiples */
struct hdmi_audio_fs_ncts aud_44k1_para = {
	.array[0] = {
		.tmds_clk = 25175,
		.n = 7007,
		.cts = 31250,
		.n_36bit = 7007,
		.cts_36bit = 46875,
		.n_48bit = 7007,
		.cts_48bit = 62500,
	},
	.array[1] = {
		.tmds_clk = 25200,
		.n = 6272,
		.cts = 28000,
		.n_36bit = 6272,
		.cts_36bit = 42000,
		.n_48bit = 6272,
		.cts_48bit = 56000,
	},
	.array[2] = {
		.tmds_clk = 27000,
		.n = 6272,
		.cts = 30000,
		.n_36bit = 6272,
		.cts_36bit = 45000,
		.n_48bit = 6272,
		.cts_48bit = 60000,
	},
	.array[3] = {
		.tmds_clk = 27027,
		.n = 6272,
		.cts = 30030,
		.n_36bit = 6272,
		.cts_36bit = 45045,
		.n_48bit = 6272,
		.cts_48bit = 60060,
	},
	.array[4] = {
		.tmds_clk = 54000,
		.n = 6272,
		.cts = 60000,
		.n_36bit = 6272,
		.cts_36bit = 90000,
		.n_48bit = 6272,
		.cts_48bit = 120000,
	},
	.array[5] = {
		.tmds_clk = 54054,
		.n = 6272,
		.cts = 60060,
		.n_36bit = 6272,
		.cts_36bit = 90090,
		.n_48bit = 6272,
		.cts_48bit = 120120,
	},
	.array[6] = {
		.tmds_clk = 74176,
		.n = 17836,
		.cts = 234375,
		.n_36bit = 17836,
		.cts_36bit = 351562,
		.n_48bit = 17836,
		.cts_48bit = 468750,
	},
	.array[7] = {
		.tmds_clk = 74250,
		.n = 6272,
		.cts = 82500,
		.n_36bit = 6272,
		.cts_36bit = 123750,
		.n_48bit = 6272,
		.cts_48bit = 165000,
	},
	.array[8] = {
		.tmds_clk = 148352,
		.n = 8918,
		.cts = 234375,
		.n_36bit = 17836,
		.cts_36bit = 703125,
		.n_48bit = 8918,
		.cts_48bit = 468750,
	},
	.array[9] = {
		.tmds_clk = 148500,
		.n = 6272,
		.cts = 165000,
		.n_36bit = 6272,
		.cts_36bit = 247500,
		.n_48bit = 6272,
		.cts_48bit = 330000,
	},
	.array[10] = {
		.tmds_clk = 296703,
		.n = 4459,
		.cts = 234375,
	},
	.array[11] = {
		.tmds_clk = 297000,
		.n = 4707,
		.cts = 247500,
	},
	.array[12] = {
		.tmds_clk = 593407,
		.n = 8918,
		.cts = 937500,
	},
	.array[13] = {
		.tmds_clk = 594000,
		.n = 9408,
		.cts = 990000,
	},
	.def_n = 6272,
};

/* Recommended N and Expected CTS for 48kHz and Multiples */
struct hdmi_audio_fs_ncts aud_48k_para = {
	.array[0] = {
		.tmds_clk = 25175,
		.n = 6864,
		.cts = 28125,
		.n_36bit = 9152,
		.cts_36bit = 58250,
		.n_48bit = 6864,
		.cts_48bit = 56250,
	},
	.array[1] = {
		.tmds_clk = 25200,
		.n = 6144,
		.cts = 25200,
		.n_36bit = 6144,
		.cts_36bit = 37800,
		.n_48bit = 6144,
		.cts_48bit = 50400,
	},
	.array[2] = {
		.tmds_clk = 27000,
		.n = 6144,
		.cts = 27000,
		.n_36bit = 6144,
		.cts_36bit = 40500,
		.n_48bit = 6144,
		.cts_48bit = 54000,
	},
	.array[3] = {
		.tmds_clk = 27027,
		.n = 6144,
		.cts = 27027,
		.n_36bit = 8192,
		.cts_36bit = 54054,
		.n_48bit = 6144,
		.cts_48bit = 54054,
	},
	.array[4] = {
		.tmds_clk = 54000,
		.n = 6144,
		.cts = 54000,
		.n_36bit = 6144,
		.cts_36bit = 81000,
		.n_48bit = 6144,
		.cts_48bit = 108000,
	},
	.array[5] = {
		.tmds_clk = 54054,
		.n = 6144,
		.cts = 54054,
		.n_36bit = 6144,
		.cts_36bit = 81081,
		.n_48bit = 6144,
		.cts_48bit = 108108,
	},
	.array[6] = {
		.tmds_clk = 74176,
		.n = 11648,
		.cts = 140625,
		.n_36bit = 11648,
		.cts_36bit = 210937,
		.n_48bit = 11648,
		.cts_48bit = 281250,
	},
	.array[7] = {
		.tmds_clk = 74250,
		.n = 6144,
		.cts = 74250,
		.n_36bit = 6144,
		.cts_36bit = 111375,
		.n_48bit = 6144,
		.cts_48bit = 148500,
	},
	.array[8] = {
		.tmds_clk = 148352,
		.n = 5824,
		.cts = 140625,
		.n_36bit = 11648,
		.cts_36bit = 421875,
		.n_48bit = 5824,
		.cts_48bit = 281250,
	},
	.array[9] = {
		.tmds_clk = 148500,
		.n = 6144,
		.cts = 148500,
		.n_36bit = 6144,
		.cts_36bit = 222750,
		.n_48bit = 6144,
		.cts_48bit = 297000,
	},
	.array[10] = {
		.tmds_clk = 296703,
		.n = 5824,
		.cts = 281250,
	},
	.array[11] = {
		.tmds_clk = 297000,
		.n = 5120,
		.cts = 247500,
	},
	.array[12] = {
		.tmds_clk = 593407,
		.n = 5824,
		.cts = 562500,
	},
	.array[13] = {
		.tmds_clk = 594000,
		.n = 6144,
		.cts = 594000,
	},
	.def_n = 6144,
};

static struct hdmi_audio_fs_ncts *all_aud_paras[] = {
	&aud_32k_para,
	&aud_44k1_para,
	&aud_48k_para,
	NULL,
};

unsigned int hdmi_get_aud_n_paras(enum hdmi_audio_fs fs,
	enum hdmi_color_depth cd, unsigned int tmds_clk)
{
	struct hdmi_audio_fs_ncts *p = NULL;
	unsigned int i, n;
	unsigned int N_multiples = 1;

	pr_info("hdmitx: fs = %d, cd = %d, tmds_clk = %d\n", fs, cd, tmds_clk);
	switch (fs) {
	case FS_32K:
		p = all_aud_paras[0];
		N_multiples = 1;
		break;
	case FS_44K1:
		p = all_aud_paras[1];
		N_multiples = 1;
		break;
	case FS_88K2:
		p = all_aud_paras[1];
		N_multiples = 2;
		break;
	case FS_176K4:
		p = all_aud_paras[1];
		N_multiples = 4;
		break;
	case FS_48K:
		p = all_aud_paras[2];
		N_multiples = 1;
		break;
	case FS_96K:
		p = all_aud_paras[2];
		N_multiples = 2;
		break;
	case FS_192K:
		p = all_aud_paras[2];
		N_multiples = 4;
		break;
	default: /* Default as FS_48K */
		p = all_aud_paras[2];
		N_multiples = 1;
		break;
	}
	for (i = 0; i < AUDIO_PARA_MAX_NUM; i++) {
		if (tmds_clk == p->array[i].tmds_clk)
			break;
	}

	if (i < AUDIO_PARA_MAX_NUM)
		if ((cd == COLORDEPTH_24B) || (cd == COLORDEPTH_30B))
			n = p->array[i].n ? p->array[i].n : p->def_n;
		else if (cd == COLORDEPTH_36B)
			n = p->array[i].n_36bit ?
				p->array[i].n_36bit : p->def_n;
		else if (cd == COLORDEPTH_48B)
			n = p->array[i].n_48bit ?
				p->array[i].n_48bit : p->def_n;
		else
			n = p->array[i].n ? p->array[i].n : p->def_n;
	else
		n = p->def_n;
	return n * N_multiples;
}
/*--------------------------------------------------------------*/
/* for csc coef */

static unsigned char coef_yc444_rgb_24bit_601[] = {
	0x20, 0x00, 0x69, 0x26, 0x74, 0xfd, 0x01, 0x0e,
	0x20, 0x00, 0x2c, 0xdd, 0x00, 0x00, 0x7e, 0x9a,
	0x20, 0x00, 0x00, 0x00, 0x38, 0xb4, 0x7e, 0x3b
};

static unsigned char coef_yc444_rgb_24bit_709[] = {
	0x20, 0x00, 0x71, 0x06, 0x7a, 0x02, 0x00, 0xa7,
	0x20, 0x00, 0x32, 0x64, 0x00, 0x00, 0x7e, 0x6d,
	0x20, 0x00, 0x00, 0x00, 0x3b, 0x61, 0x7e, 0x25
};


static struct hdmi_csc_coef_table hdmi_csc_coef[] = {
	{COLORSPACE_YUV444, COLORSPACE_RGB444, COLORDEPTH_24B, 0,
		sizeof(coef_yc444_rgb_24bit_601), coef_yc444_rgb_24bit_601},
	{COLORSPACE_YUV444, COLORSPACE_RGB444, COLORDEPTH_24B, 1,
		sizeof(coef_yc444_rgb_24bit_709), coef_yc444_rgb_24bit_709},
};

unsigned int hdmi_get_csc_coef(
	unsigned int input_format, unsigned int output_format,
	unsigned int color_depth, unsigned int color_format,
	unsigned char **coef_array, unsigned int *coef_length)
{
	unsigned int i = 0, max = 0;

	max = sizeof(hdmi_csc_coef)/sizeof(struct hdmi_csc_coef_table);

	for (i = 0; i < max; i++) {
		if ((input_format == hdmi_csc_coef[i].input_format) &&
			(output_format == hdmi_csc_coef[i].output_format) &&
			(color_depth == hdmi_csc_coef[i].color_depth) &&
			(color_format == hdmi_csc_coef[i].color_format)) {
			*coef_array = hdmi_csc_coef[i].coef;
			*coef_length = hdmi_csc_coef[i].coef_length;
			return 0;
		}
	}

	coef_array = NULL;
	*coef_length = 0;

	return 1;
}

bool is_hdmi14_4k(enum hdmi_vic vic)
{
	bool ret = 0;

	switch (vic) {
	case HDMI_3840x2160p24_16x9:
	case HDMI_3840x2160p25_16x9:
	case HDMI_3840x2160p30_16x9:
	case HDMI_4096x2160p24_256x135:
		ret = 1;
		break;
	default:
		ret = 0;
		break;
	}

	return ret;
}

