#ifndef __AML_VCODEC_VFM_H_
#define __AML_VCODEC_VFM_H_

#include "aml_vcodec_vfq.h"
#include "aml_vcodec_drv.h"
#include "aml_vcodec_dec.h"
#include <linux/amlogic/media/vfm/vframe_provider.h>
#include <linux/amlogic/media/vfm/vframe_receiver.h>

#define VF_NAME_SIZE	(32)
#define POOL_SIZE	(16)

struct vcodec_vfm_s {
	struct aml_vcodec_ctx *ctx;
	struct vfq_s vf_que;
	struct vframe_s *vf;
	struct vframe_s *pool[POOL_SIZE + 1];
	char recv_name[VF_NAME_SIZE];
	char prov_name[VF_NAME_SIZE];
	struct vframe_provider_s vf_prov;
	struct vframe_receiver_s vf_recv;
};

int vcodec_vfm_init(struct vcodec_vfm_s *vfm);

void vcodec_vfm_release(struct vcodec_vfm_s *vfm);

struct vframe_s *peek_video_frame(struct vcodec_vfm_s *vfm);

struct vframe_s *get_video_frame(struct vcodec_vfm_s *vfm);

int get_fb_from_queue(struct aml_vcodec_ctx *ctx, struct vdec_fb **out_fb);
int put_fb_to_queue(struct aml_vcodec_ctx *ctx, struct vdec_fb *in_fb);

void video_vf_put(struct vdec_fb *fb);

#endif /* __AML_VCODEC_VFM_H_ */
