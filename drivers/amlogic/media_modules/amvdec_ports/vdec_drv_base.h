/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef _VDEC_DRV_BASE_
#define _VDEC_DRV_BASE_

#include "aml_vcodec_drv.h"

#include "vdec_drv_if.h"

struct vdec_common_if {
	/**
	 * (*init)() - initialize decode driver
	 * @ctx     : [in] aml v4l2 context
	 * @h_vdec  : [out] driver handle
	 */
	int (*init)(struct aml_vcodec_ctx *ctx, unsigned long *h_vdec);

	int (*probe)(unsigned long h_vdec,
		struct aml_vcodec_mem *bs, void *out);

	/**
	 * (*decode)() - trigger decode
	 * @h_vdec  : [in] driver handle
	 * @bs      : [in] input bitstream
	 * @fb      : [in] frame buffer to store decoded frame
	 * @res_chg : [out] resolution change happen
	 */
	int (*decode)(unsigned long h_vdec, struct aml_vcodec_mem *bs,
		unsigned long int pts, bool *res_chg);

	/**
	 * (*get_param)() - get driver's parameter
	 * @h_vdec : [in] driver handle
	 * @type   : [in] input parameter type
	 * @out    : [out] buffer to store query result
	 */
	int (*get_param)(unsigned long h_vdec,
		enum vdec_get_param_type type, void *out);

	/**
	 * (*deinit)() - deinitialize driver.
	 * @h_vdec : [in] driver handle to be deinit
	 */
	void (*deinit)(unsigned long h_vdec);
};

#endif
