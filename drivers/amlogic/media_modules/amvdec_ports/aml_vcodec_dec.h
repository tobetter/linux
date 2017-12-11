#ifndef _AML_VCODEC_DEC_H_
#define _AML_VCODEC_DEC_H_

#include <media/videobuf2-core.h>
#include <media/videobuf2-v4l2.h>
#include <linux/amlogic/media/codec_mm/codec_mm.h>

#define VCODEC_CAPABILITY_4K_DISABLED	0x10
#define VCODEC_DEC_4K_CODED_WIDTH	4096U
#define VCODEC_DEC_4K_CODED_HEIGHT	2304U
#define AML_VDEC_MAX_W			2048U
#define AML_VDEC_MAX_H			1088U

#define AML_VDEC_IRQ_STATUS_DEC_SUCCESS	0x10000
#define V4L2_BUF_FLAG_LAST		0x00100000

/**
 * struct vdec_fb  - decoder frame buffer
 * @base_y	: Y plane memory info
 * @base_c	: C plane memory info
 * @status      : frame buffer status (vdec_fb_status)
 */
struct vdec_fb {
	unsigned long vf_handle;
	struct aml_vcodec_mem	base_y;
	struct aml_vcodec_mem	base_c;
	unsigned int	status;
};

/**
 * struct aml_video_dec_buf - Private data related to each VB2 buffer.
 * @b:		VB2 buffer
 * @list:	link list
 * @used:	Capture buffer contain decoded frame data and keep in
 *			codec data structure
 * @ready_to_display:	Capture buffer not display yet
 * @queued_in_vb2:	Capture buffer is queue in vb2
 * @queued_in_v4l2:	Capture buffer is in v4l2 driver, but not in vb2
 *			queue yet
 * @lastframe:		Intput buffer is last buffer - EOS
 * @error:		An unrecoverable error occurs on this buffer.
 * @frame_buffer:	Decode status, and buffer information of Capture buffer
 *
 * Note : These status information help us track and debug buffer state
 */
struct aml_video_dec_buf {
	struct vb2_v4l2_buffer vb;
	struct list_head list;

	struct vdec_fb frame_buffer;
	struct codec_mm_s *mem[2];
	bool used;
	bool ready_to_display;
	bool queued_in_vb2;
	bool queued_in_v4l2;
	bool lastframe;
	bool error;
};

extern const struct v4l2_ioctl_ops aml_vdec_ioctl_ops;
extern const struct v4l2_m2m_ops aml_vdec_m2m_ops;


/*
 * aml_vdec_lock/aml_vdec_unlock are for ctx instance to
 * get/release lock before/after access decoder hw.
 * aml_vdec_lock get decoder hw lock and set curr_ctx
 * to ctx instance that get lock
 */
void aml_vdec_unlock(struct aml_vcodec_ctx *ctx);
void aml_vdec_lock(struct aml_vcodec_ctx *ctx);
int aml_vcodec_dec_queue_init(void *priv, struct vb2_queue *src_vq,
			   struct vb2_queue *dst_vq);
void aml_vcodec_dec_set_default_params(struct aml_vcodec_ctx *ctx);
void aml_vcodec_dec_release(struct aml_vcodec_ctx *ctx);
int aml_vcodec_dec_ctrls_setup(struct aml_vcodec_ctx *ctx);

void vdec_device_vf_run(struct aml_vcodec_ctx *ctx);

#endif /* _AML_VCODEC_DEC_H_ */
