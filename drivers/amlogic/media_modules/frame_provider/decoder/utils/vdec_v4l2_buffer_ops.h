#ifndef _AML_VDEC_V4L2_BUFFER_H_
#define _AML_VDEC_V4L2_BUFFER_H_

#include "../../../amvdec_ports/vdec_drv_base.h"
#include "../../../amvdec_ports/aml_vcodec_adapt.h"

int vdec_v4l_get_buffer(
	struct aml_vcodec_ctx *ctx,
	struct vdec_v4l2_buffer **out);

int vdec_v4l_set_pic_infos(
	struct aml_vcodec_ctx *ctx,
	struct aml_vdec_pic_infos *info);

int vdec_v4l_write_frame_sync(
	struct aml_vcodec_ctx *ctx);

int vdec_v4l_binding_fd_and_vf(
	ulong v4l_handle,
	void *vf);

#endif
