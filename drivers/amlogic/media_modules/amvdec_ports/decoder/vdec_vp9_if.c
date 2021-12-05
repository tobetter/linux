/*
* Copyright (C) 2017 Amlogic, Inc. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Description:
*/
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <uapi/linux/swab.h>
#include "../vdec_drv_if.h"
#include "../aml_vcodec_util.h"
#include "../aml_vcodec_dec.h"
#include "../aml_vcodec_adapt.h"
#include "../vdec_drv_base.h"
#include "../aml_vcodec_vfm.h"
#include "aml_vp9_parser.h"
#include "vdec_vp9_trigger.h"

#define PREFIX_SIZE	(16)

#define NAL_TYPE(value)				((value) & 0x1F)
#define HEADER_BUFFER_SIZE			(32 * 1024)
#define SYNC_CODE				(0x498342)

extern int vp9_need_prefix;
bool need_trigger;
int dump_cnt = 0;

/**
 * struct vp9_fb - vp9 decode frame buffer information
 * @vdec_fb_va  : virtual address of struct vdec_fb
 * @y_fb_dma    : dma address of Y frame buffer (luma)
 * @c_fb_dma    : dma address of C frame buffer (chroma)
 * @poc         : picture order count of frame buffer
 * @reserved    : for 8 bytes alignment
 */
struct vp9_fb {
	uint64_t vdec_fb_va;
	uint64_t y_fb_dma;
	uint64_t c_fb_dma;
	int32_t poc;
	uint32_t reserved;
};

/**
 * struct vdec_vp9_dec_info - decode information
 * @dpb_sz		: decoding picture buffer size
 * @resolution_changed  : resoltion change happen
 * @reserved		: for 8 bytes alignment
 * @bs_dma		: Input bit-stream buffer dma address
 * @y_fb_dma		: Y frame buffer dma address
 * @c_fb_dma		: C frame buffer dma address
 * @vdec_fb_va		: VDEC frame buffer struct virtual address
 */
struct vdec_vp9_dec_info {
	uint32_t dpb_sz;
	uint32_t resolution_changed;
	uint32_t reserved;
	uint64_t bs_dma;
	uint64_t y_fb_dma;
	uint64_t c_fb_dma;
	uint64_t vdec_fb_va;
};

/**
 * struct vdec_vp9_vsi - shared memory for decode information exchange
 *                        between VPU and Host.
 *                        The memory is allocated by VPU then mapping to Host
 *                        in vpu_dec_init() and freed in vpu_dec_deinit()
 *                        by VPU.
 *                        AP-W/R : AP is writer/reader on this item
 *                        VPU-W/R: VPU is write/reader on this item
 * @hdr_buf      : Header parsing buffer (AP-W, VPU-R)
 * @list_free    : free frame buffer ring list (AP-W/R, VPU-W)
 * @list_disp    : display frame buffer ring list (AP-R, VPU-W)
 * @dec          : decode information (AP-R, VPU-W)
 * @pic          : picture information (AP-R, VPU-W)
 * @crop         : crop information (AP-R, VPU-W)
 */
struct vdec_vp9_vsi {
	char *header_buf;
	int sps_size;
	int pps_size;
	int sei_size;
	int head_offset;
	struct vdec_vp9_dec_info dec;
	struct vdec_pic_info pic;
	struct vdec_pic_info cur_pic;
	struct v4l2_rect crop;
	bool is_combine;
	int nalu_pos;
	struct vp9_param_sets ps;
};

/**
 * struct vdec_vp9_inst - vp9 decoder instance
 * @num_nalu : how many nalus be decoded
 * @ctx      : point to aml_vcodec_ctx
 * @vsi      : VPU shared information
 */
struct vdec_vp9_inst {
	unsigned int num_nalu;
	struct aml_vcodec_ctx *ctx;
	struct aml_vdec_adapt vdec;
	struct vdec_vp9_vsi *vsi;
	struct vcodec_vfm_s vfm;
	struct completion comp;
};

static int vdec_write_nalu(struct vdec_vp9_inst *inst,
	u8 *buf, u32 size, u64 ts);

static void get_pic_info(struct vdec_vp9_inst *inst,
			 struct vdec_pic_info *pic)
{
	*pic = inst->vsi->pic;

	aml_vcodec_debug(inst, "pic(%d, %d), buf(%d, %d)",
			 pic->visible_width, pic->visible_height,
			 pic->coded_width, pic->coded_height);
	aml_vcodec_debug(inst, "Y(%d, %d), C(%d, %d)", pic->y_bs_sz,
			 pic->y_len_sz, pic->c_bs_sz, pic->c_len_sz);
}

static void get_crop_info(struct vdec_vp9_inst *inst, struct v4l2_rect *cr)
{
	cr->left = inst->vsi->crop.left;
	cr->top = inst->vsi->crop.top;
	cr->width = inst->vsi->crop.width;
	cr->height = inst->vsi->crop.height;

	aml_vcodec_debug(inst, "l=%d, t=%d, w=%d, h=%d",
			 cr->left, cr->top, cr->width, cr->height);
}

static void get_dpb_size(struct vdec_vp9_inst *inst, unsigned int *dpb_sz)
{
	*dpb_sz = inst->vsi->dec.dpb_sz;
	aml_vcodec_debug(inst, "sz=%d", *dpb_sz);
}

static int vdec_vp9_init(struct aml_vcodec_ctx *ctx, unsigned long *h_vdec)
{
	struct vdec_vp9_inst *inst = NULL;
	int ret = -1;

	inst = kzalloc(sizeof(*inst), GFP_KERNEL);
	if (!inst)
		return -ENOMEM;

	inst->ctx = ctx;

	inst->vdec.video_type = VFORMAT_VP9;
	inst->vdec.dev	= ctx->dev->vpu_plat_dev;
	inst->vdec.filp	= ctx->dev->filp;
	inst->vdec.ctx	= ctx;

	/* set play mode.*/
	if (ctx->is_drm_mode)
		inst->vdec.port.flag |= PORT_FLAG_DRM;

	/* to eable vp9 hw.*/
	inst->vdec.port.type = PORT_TYPE_HEVC;

	/* init vfm */
	inst->vfm.ctx	= ctx;
	inst->vfm.ada_ctx = &inst->vdec;
	vcodec_vfm_init(&inst->vfm);

	/* probe info from the stream */
	inst->vsi = kzalloc(sizeof(struct vdec_vp9_vsi), GFP_KERNEL);
	if (!inst->vsi) {
		ret = -ENOMEM;
		goto error_free_inst;
	}

	/* alloc the header buffer to be used cache sps or spp etc.*/
	inst->vsi->header_buf = kzalloc(HEADER_BUFFER_SIZE, GFP_KERNEL);
	if (!inst->vsi) {
		ret = -ENOMEM;
		goto error_free_vsi;
	}

	inst->vsi->pic.visible_width	= 1920;
	inst->vsi->pic.visible_height	= 1080;
	inst->vsi->pic.coded_width	= 1920;
	inst->vsi->pic.coded_height	= 1088;
	inst->vsi->pic.y_bs_sz	= 0;
	inst->vsi->pic.y_len_sz	= (1920 * 1088);
	inst->vsi->pic.c_bs_sz	= 0;
	inst->vsi->pic.c_len_sz	= (1920 * 1088 / 2);

	init_completion(&inst->comp);

	aml_vcodec_debug(inst, "vp9 Instance >> %p", inst);

	ctx->ada_ctx = &inst->vdec;
	*h_vdec = (unsigned long)inst;

	/* init decoder. */
	ret = video_decoder_init(&inst->vdec);
	if (ret) {
		aml_vcodec_err(inst, "vdec_vp9 init err=%d", ret);
		goto error_free_inst;
	}

	//dump_init();

	return 0;

error_free_vsi:
	kfree(inst->vsi);
error_free_inst:
	kfree(inst);
	*h_vdec = 0;

	return ret;
}

#if 0
static int refer_buffer_num(int level_idc, int poc_cnt,
	int mb_width, int mb_height)
{
	return 20;
}
#endif

static void fill_vdec_params(struct vdec_vp9_inst *inst,
	struct VP9Context *vp9_ctx)
{
	struct vdec_pic_info *pic = &inst->vsi->pic;
	struct vdec_vp9_dec_info *dec = &inst->vsi->dec;
	struct v4l2_rect *rect = &inst->vsi->crop;

	/* fill visible area size that be used for EGL. */
	pic->visible_width	= vp9_ctx->render_width;
	pic->visible_height	= vp9_ctx->render_height;

	/* calc visible ares. */
	rect->left		= 0;
	rect->top		= 0;
	rect->width		= pic->visible_width;
	rect->height		= pic->visible_height;

	/* config canvas size that be used for decoder. */
	pic->coded_width	= ALIGN(vp9_ctx->width, 32);
	pic->coded_height	= ALIGN(vp9_ctx->height, 32);

	pic->y_len_sz		= pic->coded_width * pic->coded_height;
	pic->c_len_sz		= pic->y_len_sz >> 1;

	/* calc DPB size */
	dec->dpb_sz = 5;//refer_buffer_num(sps->level_idc, poc_cnt, mb_w, mb_h);

	aml_vcodec_debug(inst, "[%d] The stream infos, coded:(%d x %d), visible:(%d x %d), DPB: %d\n",
		inst->ctx->id, pic->coded_width, pic->coded_height,
		pic->visible_width, pic->visible_height, dec->dpb_sz);
}

static int stream_parse_by_ucode(struct vdec_vp9_inst *inst, u8 *buf, u32 size)
{
	int ret = 0;

	ret = vdec_write_nalu(inst, buf, size, 0);
	if (ret < 0) {
		pr_err("write frame data failed. err: %d\n", ret);
		return ret;
	}

	/* wait ucode parse ending. */
	wait_for_completion_timeout(&inst->comp,
		msecs_to_jiffies(1000));

	return inst->vsi->dec.dpb_sz ? 0 : -1;
}

static int stream_parse(struct vdec_vp9_inst *inst, u8 *buf, u32 size)
{
	int ret = 0;
	struct vp9_param_sets *ps = NULL;

	ps = kzalloc(sizeof(struct vp9_param_sets), GFP_KERNEL);
	if (ps == NULL)
		return -ENOMEM;

	ret = vp9_decode_extradata_ps(buf, size, ps);
	if (ret) {
		pr_err("parse extra data failed. err: %d\n", ret);
		goto out;
	}

	if (ps->head_parsed)
		fill_vdec_params(inst, &ps->ctx);

	ret = ps->head_parsed ? 0 : -1;
out:
	kfree(ps);

	return ret;
}

static int vdec_vp9_probe(unsigned long h_vdec,
	struct aml_vcodec_mem *bs, void *out)
{
	struct vdec_vp9_inst *inst =
		(struct vdec_vp9_inst *)h_vdec;
	struct stream_info *st;
	u8 *buf = (u8 *)bs->vaddr;
	u32 size = bs->size;
	int ret = 0;

	st = (struct stream_info *)buf;
	if (inst->ctx->is_drm_mode && (st->magic == DRMe || st->magic == DRMn))
		return 0;

	if (st->magic == NORe || st->magic == NORn)
		ret = stream_parse(inst, st->data, st->length);
	else {
		if (inst->ctx->param_sets_from_ucode)
			ret = stream_parse_by_ucode(inst, buf, size);
		else
			ret = stream_parse(inst, buf, size);
	}

	inst->vsi->cur_pic = inst->vsi->pic;

	return ret;
}

static void vdec_vp9_deinit(unsigned long h_vdec)
{
	ulong flags;
	struct vdec_vp9_inst *inst = (struct vdec_vp9_inst *)h_vdec;
	struct aml_vcodec_ctx *ctx = inst->ctx;

	aml_vcodec_debug_enter(inst);

	video_decoder_release(&inst->vdec);

	vcodec_vfm_release(&inst->vfm);

	//dump_deinit();

	spin_lock_irqsave(&ctx->slock, flags);
	if (inst->vsi && inst->vsi->header_buf)
		kfree(inst->vsi->header_buf);

	if (inst->vsi)
		kfree(inst->vsi);

	kfree(inst);

	ctx->drv_handle = 0;
	spin_unlock_irqrestore(&ctx->slock, flags);

	need_trigger = false;
	dump_cnt = 0;
}

static int vdec_vp9_get_fb(struct vdec_vp9_inst *inst, struct vdec_v4l2_buffer **out)
{
	return get_fb_from_queue(inst->ctx, out);
}

static void vdec_vp9_get_vf(struct vdec_vp9_inst *inst, struct vdec_v4l2_buffer **out)
{
	struct vframe_s *vf = NULL;
	struct vdec_v4l2_buffer *fb = NULL;

	vf = peek_video_frame(&inst->vfm);
	if (!vf) {
		aml_vcodec_debug(inst, "there is no vframe.");
		*out = NULL;
		return;
	}

	vf = get_video_frame(&inst->vfm);
	if (!vf) {
		aml_vcodec_debug(inst, "the vframe is avalid.");
		*out = NULL;
		return;
	}

	atomic_set(&vf->use_cnt, 1);

	fb = (struct vdec_v4l2_buffer *)vf->v4l_mem_handle;
	fb->vf_handle = (unsigned long)vf;
	fb->status = FB_ST_DISPLAY;

	*out = fb;

	//pr_info("%s, %d\n", __func__, fb->base_y.bytes_used);
	//dump_write(fb->base_y.vaddr, fb->base_y.bytes_used);
	//dump_write(fb->base_c.vaddr, fb->base_c.bytes_used);

	/* convert yuv format. */
	//swap_uv(fb->base_c.vaddr, fb->base_c.size);
}

static void add_prefix_data(struct vp9_superframe_split *s,
	u8 **out, u32 *out_size)
{
	int i;
	u8 *p = NULL;
	u32 length;

	length = s->size + s->nb_frames * PREFIX_SIZE;
	p = vzalloc(length);
	if (!p) {
		pr_err("alloc size %d failed.\n" ,length);
		return;
	}

	memcpy(p, s->data, s->size);
	p += s->size;

	for (i = s->nb_frames; i > 0; i--) {
		u32 frame_size = s->sizes[i - 1];
		u8 *prefix = NULL;

		p -= frame_size;
		memmove(p + PREFIX_SIZE * i, p, frame_size);
		prefix = p + PREFIX_SIZE * (i - 1);

		/*add amlogic frame headers.*/
		frame_size += 16;
		prefix[0]  = (frame_size >> 24) & 0xff;
		prefix[1]  = (frame_size >> 16) & 0xff;
		prefix[2]  = (frame_size >> 8 ) & 0xff;
		prefix[3]  = (frame_size >> 0 ) & 0xff;
		prefix[4]  = ((frame_size >> 24) & 0xff) ^ 0xff;
		prefix[5]  = ((frame_size >> 16) & 0xff) ^ 0xff;
		prefix[6]  = ((frame_size >> 8 ) & 0xff) ^ 0xff;
		prefix[7]  = ((frame_size >> 0 ) & 0xff) ^ 0xff;
		prefix[8]  = 0;
		prefix[9]  = 0;
		prefix[10] = 0;
		prefix[11] = 1;
		prefix[12] = 'A';
		prefix[13] = 'M';
		prefix[14] = 'L';
		prefix[15] = 'V';
		frame_size -= 16;
	}

	*out = p;
	*out_size = length;
}

static void trigger_decoder(struct aml_vdec_adapt *vdec)
{
	int i, ret;
	u32 frame_size = 0;
	u8 *p = vp9_trigger_header;

	for (i = 0; i < ARRAY_SIZE(vp9_trigger_framesize); i++) {
		frame_size = vp9_trigger_framesize[i];
		ret = vdec_vframe_write(vdec, p,
			frame_size, 0);
		pr_err("write trigger frame %d\n", ret);
		p += frame_size;
	}
}

static int vdec_write_nalu(struct vdec_vp9_inst *inst,
	u8 *buf, u32 size, u64 ts)
{
	int ret = 0;
	struct aml_vdec_adapt *vdec = &inst->vdec;
	struct vp9_superframe_split s;
	u8 *data = NULL;
	u32 length = 0;
	bool need_prefix = vp9_need_prefix;

	memset(&s, 0, sizeof(s));

	/*trigger.*/
	if (0 && !need_trigger) {
		trigger_decoder(vdec);
		need_trigger = true;
	}

	if (need_prefix) {
		/*parse superframe.*/
		s.data = buf;
		s.data_size = size;
		ret = vp9_superframe_split_filter(&s);
		if (ret) {
			pr_err("parse frames failed.\n");
			return ret;
		}

		/*add headers.*/
		add_prefix_data(&s, &data, &length);
		ret = vdec_vframe_write(vdec, data, length, ts);
		vfree(data);
	} else {
		ret = vdec_vframe_write(vdec, buf, size, ts);
	}

	return ret;
}

static bool monitor_res_change(struct vdec_vp9_inst *inst, u8 *buf, u32 size)
{
	int ret = -1;
	u8 *p = buf;
	int len = size;
	u32 synccode = vp9_need_prefix ?
		((p[1] << 16) | (p[2] << 8) | p[3]) :
		((p[17] << 16) | (p[18] << 8) | p[19]);

	if (synccode == SYNC_CODE) {
		ret = stream_parse(inst, p, len);
		if (!ret && (inst->vsi->cur_pic.coded_width !=
			inst->vsi->pic.coded_width ||
			inst->vsi->cur_pic.coded_height !=
			inst->vsi->pic.coded_height)) {
			inst->vsi->cur_pic = inst->vsi->pic;
			return true;
		}
	}

	return false;
}

static int vdec_vp9_decode(unsigned long h_vdec, struct aml_vcodec_mem *bs,
			 u64 timestamp, bool *res_chg)
{
	struct vdec_vp9_inst *inst = (struct vdec_vp9_inst *)h_vdec;
	struct aml_vdec_adapt *vdec = &inst->vdec;
	struct stream_info *st;
	u8 *buf;
	u32 size;
	int ret = -1;

	/* bs NULL means flush decoder */
	if (bs == NULL)
		return -1;

	buf = (u8 *)bs->vaddr;
	size = bs->size;
	st = (struct stream_info *)buf;

	if (inst->ctx->is_drm_mode && (st->magic == DRMe || st->magic == DRMn))
		ret = vdec_vbuf_write(vdec, st->m.buf, sizeof(st->m.drm));
	else if (st->magic == NORe)
		ret = vdec_vbuf_write(vdec, st->data, st->length);
	else if (st->magic == NORn)
		ret = vdec_write_nalu(inst, st->data, st->length, timestamp);
	else if (inst->ctx->is_stream_mode)
		ret = vdec_vbuf_write(vdec, buf, size);
	else {
		/*checked whether the resolution changes.*/
		if ((*res_chg = monitor_res_change(inst, buf, size)))
			return 0;

		ret = vdec_write_nalu(inst, buf, size, timestamp);
	}

	return ret;
}

static int vdec_vp9_get_param(unsigned long h_vdec,
			       enum vdec_get_param_type type, void *out)
{
	int ret = 0;
	struct vdec_vp9_inst *inst = (struct vdec_vp9_inst *)h_vdec;

	if (!inst) {
		pr_err("the vp9 inst of dec is invalid.\n");
		return -1;
	}

	switch (type) {
	case GET_PARAM_DISP_FRAME_BUFFER:
		vdec_vp9_get_vf(inst, out);
		break;

	case GET_PARAM_FREE_FRAME_BUFFER:
		ret = vdec_vp9_get_fb(inst, out);
		break;

	case GET_PARAM_PIC_INFO:
		get_pic_info(inst, out);
		break;

	case GET_PARAM_DPB_SIZE:
		get_dpb_size(inst, out);
		break;

	case GET_PARAM_CROP_INFO:
		get_crop_info(inst, out);
		break;

	default:
		aml_vcodec_err(inst, "invalid get parameter type=%d", type);
		ret = -EINVAL;
	}

	return ret;
}

static void set_param_write_sync(struct vdec_vp9_inst *inst)
{
	complete(&inst->comp);
}

static void set_param_pic_info(struct vdec_vp9_inst *inst,
	struct aml_vdec_pic_infos *info)
{
	struct vdec_pic_info *pic = &inst->vsi->pic;
	struct vdec_vp9_dec_info *dec = &inst->vsi->dec;
	struct v4l2_rect *rect = &inst->vsi->crop;

	/* fill visible area size that be used for EGL. */
	pic->visible_width	= info->visible_width;
	pic->visible_height	= info->visible_height;

	/* calc visible ares. */
	rect->left		= 0;
	rect->top		= 0;
	rect->width		= pic->visible_width;
	rect->height		= pic->visible_height;

	/* config canvas size that be used for decoder. */
	pic->coded_width	= info->coded_width;
	pic->coded_height	= info->coded_height;

	pic->y_len_sz		= pic->coded_width * pic->coded_height;
	pic->c_len_sz		= pic->y_len_sz >> 1;

	/* calc DPB size */
	dec->dpb_sz = 5;

	/*wake up*/
	complete(&inst->comp);

	pr_info("Parse from ucode, crop(%d x %d), coded(%d x %d) dpb: %d\n",
		info->visible_width, info->visible_height,
		info->coded_width, info->coded_height,
		info->dpb_size);
}

static int vdec_vp9_set_param(unsigned long h_vdec,
	enum vdec_set_param_type type, void *in)
{
	int ret = 0;
	struct vdec_vp9_inst *inst = (struct vdec_vp9_inst *)h_vdec;

	if (!inst) {
		pr_err("the vp9 inst of dec is invalid.\n");
		return -1;
	}

	switch (type) {
	case SET_PARAM_WRITE_FRAME_SYNC:
		set_param_write_sync(inst);
		break;

	case SET_PARAM_PIC_INFO:
		set_param_pic_info(inst, in);
		break;

	default:
		aml_vcodec_err(inst, "invalid set parameter type=%d", type);
		ret = -EINVAL;
	}

	return ret;
}

static struct vdec_common_if vdec_vp9_if = {
	.init		= vdec_vp9_init,
	.probe		= vdec_vp9_probe,
	.decode		= vdec_vp9_decode,
	.get_param	= vdec_vp9_get_param,
	.set_param	= vdec_vp9_set_param,
	.deinit		= vdec_vp9_deinit,
};

struct vdec_common_if *get_vp9_dec_comm_if(void);

struct vdec_common_if *get_vp9_dec_comm_if(void)
{
	return &vdec_vp9_if;
}

