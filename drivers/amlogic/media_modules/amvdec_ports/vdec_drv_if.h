#ifndef _VDEC_DRV_IF_H_
#define _VDEC_DRV_IF_H_

#include "aml_vcodec_drv.h"
#include "aml_vcodec_dec.h"
#include "aml_vcodec_util.h"


/**
 * struct vdec_fb_status  - decoder frame buffer status
 * @FB_ST_NORMAL	: initial state
 * @FB_ST_DISPLAY	: frmae buffer is ready to be displayed
 * @FB_ST_FREE		: frame buffer is not used by decoder any more
 */
enum vdec_fb_status {
	FB_ST_NORMAL,
	FB_ST_DISPLAY,
	FB_ST_FREE
};

/* For GET_PARAM_DISP_FRAME_BUFFER and GET_PARAM_FREE_FRAME_BUFFER,
 * the caller does not own the returned buffer. The buffer will not be
 *				released before vdec_if_deinit.
 * GET_PARAM_DISP_FRAME_BUFFER	: get next displayable frame buffer,
 *				struct vdec_fb**
 * GET_PARAM_FREE_FRAME_BUFFER	: get non-referenced framebuffer, vdec_fb**
 * GET_PARAM_PIC_INFO		: get picture info, struct vdec_pic_info*
 * GET_PARAM_CROP_INFO		: get crop info, struct v4l2_crop*
 * GET_PARAM_DPB_SIZE		: get dpb size, unsigned int*
 */
enum vdec_get_param_type {
	GET_PARAM_DISP_FRAME_BUFFER,
	GET_PARAM_FREE_FRAME_BUFFER,
	GET_PARAM_PIC_INFO,
	GET_PARAM_CROP_INFO,
	GET_PARAM_DPB_SIZE
};

/**
 * struct vdec_fb_node  - decoder frame buffer node
 * @list	: list to hold this node
 * @fb	: point to frame buffer (vdec_fb), fb could point to frame buffer and
 *	working buffer this is for maintain buffers in different state
 */
struct vdec_fb_node {
	struct list_head list;
	struct vdec_fb *fb;
};

/**
 * vdec_if_init() - initialize decode driver
 * @ctx	: [in] v4l2 context
 * @fourcc	: [in] video format fourcc, V4L2_PIX_FMT_H264/VP8/VP9..
 */
int vdec_if_init(struct aml_vcodec_ctx *ctx, unsigned int fourcc);

int vdec_if_probe(struct aml_vcodec_ctx *ctx,
	struct aml_vcodec_mem *bs, void *out);

/**
 * vdec_if_deinit() - deinitialize decode driver
 * @ctx	: [in] v4l2 context
 *
 */
void vdec_if_deinit(struct aml_vcodec_ctx *ctx);

/**
 * vdec_if_decode() - trigger decode
 * @ctx	: [in] v4l2 context
 * @bs	: [in] input bitstream
 * @fb	: [in] frame buffer to store decoded frame, when null menas parse
 *	header only
 * @res_chg	: [out] resolution change happens if current bs have different
 *	picture width/height
 * Note: To flush the decoder when reaching EOF, set input bitstream as NULL.
 *
 * Return: 0 on success. -EIO on unrecoverable error.
 */
int vdec_if_decode(struct aml_vcodec_ctx *ctx, struct aml_vcodec_mem *bs,
			unsigned long int timestamp, bool *res_chg);

/**
 * vdec_if_get_param() - get driver's parameter
 * @ctx	: [in] v4l2 context
 * @type	: [in] input parameter type
 * @out	: [out] buffer to store query result
 */
int vdec_if_get_param(struct aml_vcodec_ctx *ctx, enum vdec_get_param_type type,
		      void *out);

#endif
