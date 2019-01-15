/*
 * drivers/amlogic/media/enhancement/amvecm/amcm.h
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

#ifndef __AM_LC_H
#define __AM_LC_H


#include <linux/amlogic/media/vfm/vframe.h>

/*V1.0: Local_contrast Basic function, iir algorithm, debug interface for tool*/
#define LC_VER		"Ref.2019/02/27-V1.0"

enum lc_mtx_sel_e {
	INP_MTX = 0x1,
	OUTP_MTX = 0x2,
	STAT_MTX = 0x4,
	MAX_MTX
};

enum lc_mtx_csc_e {
	LC_MTX_NULL = 0,
	LC_MTX_YUV709L_RGB = 0x1,
	LC_MTX_RGB_YUV709L = 0x2,
	LC_MTX_MAX
};

enum lc_reg_lut_e {
	SATUR_LUT = 0x1,
	YMINVAL_LMT = 0x2,
	YPKBV_YMAXVAL_LMT = 0x4,
	YPKBV_RAT = 0x8,
	YPKBV_SLP_LMT = 0x10,
	CNTST_LMT = 0x20,
	MAX_REG_LUT
};

extern int amlc_debug;
extern int lc_en;
extern int lc_demo_mode;
extern unsigned int lc_hist_vs;
extern unsigned int lc_hist_ve;
extern unsigned int lc_hist_hs;
extern unsigned int lc_hist_he;
extern unsigned int lc_hist_prcnt;
extern unsigned int lc_curve_prcnt;
extern int osd_iir_en;
extern int amlc_iir_debug_en;
/*osd related setting */
extern int vnum_start_below;
extern int vnum_end_below;
extern int vnum_start_above;
extern int vnum_end_above;
extern int invalid_blk;
/*u10,7000/21600=0.324*1024=331 */
extern int min_bv_percent_th;
/*control the refresh speed*/
extern int alpha1;
extern int alpha2;
extern int refresh_bit;
extern int ts;
extern int scene_change_th;
extern bool lc_curve_fresh;
extern int *lc_szcurve;/*12*8*6+4*/
extern int *curve_nodes_cur;
extern int *lc_hist;/*12*8*17*/


extern void lc_init(void);
extern void lc_process(struct vframe_s *vf,
	unsigned int sps_h_en,
	unsigned int sps_v_en);
extern void lc_free(void);
#endif

