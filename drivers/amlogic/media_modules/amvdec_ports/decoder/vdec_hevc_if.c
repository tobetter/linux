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
#include "aml_hevc_parser.h"

#define HEVC_NAL_TYPE(value)				((value >> 1) & 0x3F)
#define HEADER_BUFFER_SIZE			(32 * 1024)

/**
 * struct hevc_fb - hevc decode frame buffer information
 * @vdec_fb_va  : virtual address of struct vdec_fb
 * @y_fb_dma    : dma address of Y frame buffer (luma)
 * @c_fb_dma    : dma address of C frame buffer (chroma)
 * @poc         : picture order count of frame buffer
 * @reserved    : for 8 bytes alignment
 */
struct hevc_fb {
	uint64_t vdec_fb_va;
	uint64_t y_fb_dma;
	uint64_t c_fb_dma;
	int32_t poc;
	uint32_t reserved;
};

/**
 * struct vdec_hevc_dec_info - decode information
 * @dpb_sz		: decoding picture buffer size
 * @resolution_changed  : resoltion change happen
 * @reserved		: for 8 bytes alignment
 * @bs_dma		: Input bit-stream buffer dma address
 * @y_fb_dma		: Y frame buffer dma address
 * @c_fb_dma		: C frame buffer dma address
 * @vdec_fb_va		: VDEC frame buffer struct virtual address
 */
struct vdec_hevc_dec_info {
	uint32_t dpb_sz;
	uint32_t resolution_changed;
	uint32_t reserved;
	uint64_t bs_dma;
	uint64_t y_fb_dma;
	uint64_t c_fb_dma;
	uint64_t vdec_fb_va;
};

/**
 * struct vdec_hevc_vsi - shared memory for decode information exchange
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
struct vdec_hevc_vsi {
	char *header_buf;
	int sps_size;
	int pps_size;
	int sei_size;
	int head_offset;
	struct vdec_hevc_dec_info dec;
	struct vdec_pic_info pic;
	struct vdec_pic_info cur_pic;
	struct v4l2_rect crop;
	bool is_combine;
	int nalu_pos;
	struct h265_param_sets ps;
};

/**
 * struct vdec_hevc_inst - hevc decoder instance
 * @num_nalu : how many nalus be decoded
 * @ctx      : point to aml_vcodec_ctx
 * @vsi      : VPU shared information
 */
struct vdec_hevc_inst {
	unsigned int num_nalu;
	struct aml_vcodec_ctx *ctx;
	struct aml_vdec_adapt vdec;
	struct vdec_hevc_vsi *vsi;
	struct vcodec_vfm_s vfm;
	struct completion comp;
};

static void get_pic_info(struct vdec_hevc_inst *inst,
			 struct vdec_pic_info *pic)
{
	*pic = inst->vsi->pic;

	aml_vcodec_debug(inst, "pic(%d, %d), buf(%d, %d)",
			 pic->visible_width, pic->visible_height,
			 pic->coded_width, pic->coded_height);
	aml_vcodec_debug(inst, "Y(%d, %d), C(%d, %d)", pic->y_bs_sz,
			 pic->y_len_sz, pic->c_bs_sz, pic->c_len_sz);
}

static void get_crop_info(struct vdec_hevc_inst *inst, struct v4l2_rect *cr)
{
	cr->left = inst->vsi->crop.left;
	cr->top = inst->vsi->crop.top;
	cr->width = inst->vsi->crop.width;
	cr->height = inst->vsi->crop.height;

	aml_vcodec_debug(inst, "l=%d, t=%d, w=%d, h=%d",
			 cr->left, cr->top, cr->width, cr->height);
}

static void get_dpb_size(struct vdec_hevc_inst *inst, unsigned int *dpb_sz)
{
	*dpb_sz = inst->vsi->dec.dpb_sz;
	aml_vcodec_debug(inst, "sz=%d", *dpb_sz);
}

static int vdec_hevc_init(struct aml_vcodec_ctx *ctx, unsigned long *h_vdec)
{
	struct vdec_hevc_inst *inst = NULL;
	int ret = -1;

	inst = kzalloc(sizeof(*inst), GFP_KERNEL);
	if (!inst)
		return -ENOMEM;

	inst->ctx = ctx;

	inst->vdec.video_type = VFORMAT_HEVC;
	inst->vdec.dev	= ctx->dev->vpu_plat_dev;
	inst->vdec.filp	= ctx->dev->filp;
	inst->vdec.ctx	= ctx;

	/* set play mode.*/
	if (ctx->is_drm_mode)
		inst->vdec.port.flag |= PORT_FLAG_DRM;

	/* to eable hevc hw.*/
	inst->vdec.port.type = PORT_TYPE_HEVC;

	/* init vfm */
	inst->vfm.ctx	= ctx;
	inst->vfm.ada_ctx = &inst->vdec;
	vcodec_vfm_init(&inst->vfm);

	ret = video_decoder_init(&inst->vdec);
	if (ret) {
		aml_vcodec_err(inst, "vdec_hevc init err=%d", ret);
		goto error_free_inst;
	}

	/* probe info from the stream */
	inst->vsi = kzalloc(sizeof(struct vdec_hevc_vsi), GFP_KERNEL);
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

	aml_vcodec_debug(inst, "hevc Instance >> %p", inst);

	ctx->ada_ctx = &inst->vdec;
	*h_vdec = (unsigned long)inst;

	//dump_init();

	return 0;

error_free_vsi:
	kfree(inst->vsi);
error_free_inst:
	kfree(inst);
	*h_vdec = 0;

	return ret;
}


static int refer_buffer_num(struct h265_SPS_t *sps)
{
	int used_buf_num = 0;
	int sps_pic_buf_diff = 0;

	if ((!sps->temporal_layer[0].num_reorder_pics) &&
		(sps->temporal_layer[0].max_dec_pic_buffering)) {
		/* the range of sps_num_reorder_pics_0 is in
		[0, sps_max_dec_pic_buffering_minus1_0] */
		used_buf_num = sps->temporal_layer[0].max_dec_pic_buffering;
	} else
		used_buf_num = sps->temporal_layer[0].num_reorder_pics;

	sps_pic_buf_diff = sps->temporal_layer[0].max_dec_pic_buffering -
		sps->temporal_layer[0].num_reorder_pics + 1;

	if (sps_pic_buf_diff >= 4)
		used_buf_num += 1;

	/*need one more for multi instance, as
	apply_ref_pic_set() has no chanch to run to
	to clear referenced flag in some case */
	used_buf_num++;

	/* for eos add more buffer to flush.*/
	used_buf_num++;

	return used_buf_num;
}

static void fill_vdec_params(struct vdec_hevc_inst *inst, struct h265_SPS_t *sps)
{
	struct vdec_pic_info *pic = &inst->vsi->pic;
	struct vdec_hevc_dec_info *dec = &inst->vsi->dec;
	struct v4l2_rect *rect = &inst->vsi->crop;

	/* fill visible area size that be used for EGL. */
	pic->visible_width = sps->width - (sps->output_window.left_offset +
		sps->output_window.right_offset);
	pic->visible_height = sps->height - (sps->output_window.top_offset +
		sps->output_window.bottom_offset);

	/* calc visible ares. */
	rect->left		= 0;
	rect->top		= 0;
	rect->width		= pic->visible_width;
	rect->height		= pic->visible_height;

	/* config canvas size that be used for decoder. */
	pic->coded_width	= ALIGN(sps->width, 32);
	pic->coded_height	= ALIGN(sps->height, 32);

	pic->y_len_sz		= pic->coded_width * pic->coded_height;
	pic->c_len_sz		= pic->y_len_sz >> 1;

	/* calc DPB size */
	dec->dpb_sz = refer_buffer_num(sps);

	pr_info("[%d] The stream infos, coded:(%d x %d), visible:(%d x %d), DPB: %d\n",
		inst->ctx->id, pic->coded_width, pic->coded_height,
		pic->visible_width, pic->visible_height, dec->dpb_sz);
}

static int stream_parse_by_ucode(struct vdec_hevc_inst *inst, u8 *buf, u32 size)
{
	int ret = 0;
	struct aml_vdec_adapt *vdec = &inst->vdec;

	ret = vdec_vframe_write(vdec, buf, size, 0);
	if (ret < 0) {
		pr_err("write frame data failed. err: %d\n", ret);
		return ret;
	}

	/* wait ucode parse ending. */
	wait_for_completion_timeout(&inst->comp,
		msecs_to_jiffies(1000));

	return inst->vsi->dec.dpb_sz ? 0 : -1;
}

static int stream_parse(struct vdec_hevc_inst *inst, u8 *buf, u32 size)
{
	int ret = 0;
	struct h265_param_sets *ps = NULL;

	ps = kzalloc(sizeof(struct h265_param_sets), GFP_KERNEL);
	if (ps == NULL)
		return -ENOMEM;

	ret = h265_decode_extradata_ps(buf, size, ps);
	if (ret) {
		pr_err("parse extra data failed. err: %d\n", ret);
		goto out;
	}

	if (ps->sps_parsed)
		fill_vdec_params(inst, &ps->sps);

	ret = ps->sps_parsed ? 0 : -1;
out:
	kfree(ps);

	return ret;
}

static int vdec_hevc_probe(unsigned long h_vdec,
	struct aml_vcodec_mem *bs, void *out)
{
	struct vdec_hevc_inst *inst =
		(struct vdec_hevc_inst *)h_vdec;
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

static void vdec_hevc_deinit(unsigned long h_vdec)
{
	ulong flags;
	struct vdec_hevc_inst *inst = (struct vdec_hevc_inst *)h_vdec;
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
}

static int vdec_hevc_get_fb(struct vdec_hevc_inst *inst, struct vdec_v4l2_buffer **out)
{
	return get_fb_from_queue(inst->ctx, out);
}

static void vdec_hevc_get_vf(struct vdec_hevc_inst *inst, struct vdec_v4l2_buffer **out)
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

static int vdec_write_nalu(struct vdec_hevc_inst *inst,
	u8 *buf, u32 size, u64 ts)
{
	int ret = 0;
	struct aml_vdec_adapt *vdec = &inst->vdec;

	ret = vdec_vframe_write(vdec, buf, size, ts);

	return ret;
}

static bool monitor_res_change(struct vdec_hevc_inst *inst, u8 *buf, u32 size)
{
	int ret = 0, i = 0, j = 0;
	u8 *p = buf;
	int len = size;
	u32 type;

	for (i = 4; i < size; i++) {
		j = find_start_code(p, len);
		if (j > 0) {
			len = size - (p - buf);
			type = HEVC_NAL_TYPE(p[j]);
			if (type != HEVC_NAL_AUD &&
				(type > HEVC_NAL_PPS || type < HEVC_NAL_VPS))
				break;

			if (type == HEVC_NAL_SPS) {
				ret = stream_parse(inst, p, len);
				if (!ret && (inst->vsi->cur_pic.coded_width !=
					inst->vsi->pic.coded_width ||
					inst->vsi->cur_pic.coded_height !=
					inst->vsi->pic.coded_height)) {
					inst->vsi->cur_pic = inst->vsi->pic;
					return true;
				}
			}
			p += j;
		}
		p++;
	}

	return false;
}

static int vdec_hevc_decode(unsigned long h_vdec, struct aml_vcodec_mem *bs,
			 u64 timestamp, bool *res_chg)
{
	struct vdec_hevc_inst *inst = (struct vdec_hevc_inst *)h_vdec;
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

static int vdec_hevc_get_param(unsigned long h_vdec,
			       enum vdec_get_param_type type, void *out)
{
	int ret = 0;
	struct vdec_hevc_inst *inst = (struct vdec_hevc_inst *)h_vdec;

	if (!inst) {
		pr_err("the hevc inst of dec is invalid.\n");
		return -1;
	}

	switch (type) {
	case GET_PARAM_DISP_FRAME_BUFFER:
		vdec_hevc_get_vf(inst, out);
		break;

	case GET_PARAM_FREE_FRAME_BUFFER:
		ret = vdec_hevc_get_fb(inst, out);
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

static void set_param_write_sync(struct vdec_hevc_inst *inst)
{
	complete(&inst->comp);
}

static void set_param_pic_info(struct vdec_hevc_inst *inst,
	struct aml_vdec_pic_infos *info)
{
	struct vdec_pic_info *pic = &inst->vsi->pic;
	struct vdec_hevc_dec_info *dec = &inst->vsi->dec;
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
	pic->coded_width	= ALIGN(info->coded_width, 64);
	pic->coded_height	= ALIGN(info->coded_height, 64);
	pic->y_len_sz		= pic->coded_width * pic->coded_height;
	pic->c_len_sz		= pic->y_len_sz >> 1;

	dec->dpb_sz = info->dpb_size;

	/*wake up*/
	complete(&inst->comp);

	pr_info("Parse from ucode, crop(%d x %d), coded(%d x %d) dpb: %d\n",
		pic->visible_width, pic->visible_height,
		pic->coded_width, pic->coded_height,
		dec->dpb_sz);
}

static int vdec_hevc_set_param(unsigned long h_vdec,
	enum vdec_set_param_type type, void *in)
{
	int ret = 0;
	struct vdec_hevc_inst *inst = (struct vdec_hevc_inst *)h_vdec;

	if (!inst) {
		pr_err("the hevc inst of dec is invalid.\n");
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

static struct vdec_common_if vdec_hevc_if = {
	.init		= vdec_hevc_init,
	.probe		= vdec_hevc_probe,
	.decode		= vdec_hevc_decode,
	.get_param	= vdec_hevc_get_param,
	.set_param	= vdec_hevc_set_param,
	.deinit		= vdec_hevc_deinit,
};

struct vdec_common_if *get_hevc_dec_comm_if(void);

struct vdec_common_if *get_hevc_dec_comm_if(void)
{
	return &vdec_hevc_if;
}
