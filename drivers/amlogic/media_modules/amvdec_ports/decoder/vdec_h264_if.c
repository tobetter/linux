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
#include "aml_h264_parser.h"
#include "../utils/common.h"

/* h264 NALU type */
#define NAL_NON_IDR_SLICE			0x01
#define NAL_IDR_SLICE				0x05
#define NAL_H264_SEI				0x06
#define NAL_H264_SPS				0x07
#define NAL_H264_PPS				0x08
#define NAL_H264_AUD				0x09

#define AVC_NAL_TYPE(value)				((value) & 0x1F)

#define BUF_PREDICTION_SZ			(64 * 1024)//(32 * 1024)

#define MB_UNIT_LEN				16

/* motion vector size (bytes) for every macro block */
#define HW_MB_STORE_SZ				64

#define H264_MAX_FB_NUM				17
#define HDR_PARSING_BUF_SZ			1024

#define HEADER_BUFFER_SIZE			(128 * 1024)

/**
 * struct h264_fb - h264 decode frame buffer information
 * @vdec_fb_va  : virtual address of struct vdec_fb
 * @y_fb_dma    : dma address of Y frame buffer (luma)
 * @c_fb_dma    : dma address of C frame buffer (chroma)
 * @poc         : picture order count of frame buffer
 * @reserved    : for 8 bytes alignment
 */
struct h264_fb {
	uint64_t vdec_fb_va;
	uint64_t y_fb_dma;
	uint64_t c_fb_dma;
	int32_t poc;
	uint32_t reserved;
};

/**
 * struct h264_ring_fb_list - ring frame buffer list
 * @fb_list   : frame buffer arrary
 * @read_idx  : read index
 * @write_idx : write index
 * @count     : buffer count in list
 */
struct h264_ring_fb_list {
	struct h264_fb fb_list[H264_MAX_FB_NUM];
	unsigned int read_idx;
	unsigned int write_idx;
	unsigned int count;
	unsigned int reserved;
};

/**
 * struct vdec_h264_dec_info - decode information
 * @dpb_sz		: decoding picture buffer size
 * @realloc_mv_buf	: flag to notify driver to re-allocate mv buffer
 * @reserved		: for 8 bytes alignment
 * @bs_dma		: Input bit-stream buffer dma address
 * @y_fb_dma		: Y frame buffer dma address
 * @c_fb_dma		: C frame buffer dma address
 * @vdec_fb_va		: VDEC frame buffer struct virtual address
 */
struct vdec_h264_dec_info {
	uint32_t dpb_sz;
	uint32_t realloc_mv_buf;
	uint32_t reserved;
	uint64_t bs_dma;
	uint64_t y_fb_dma;
	uint64_t c_fb_dma;
	uint64_t vdec_fb_va;
};

/**
 * struct vdec_h264_vsi - shared memory for decode information exchange
 *                        between VPU and Host.
 *                        The memory is allocated by VPU then mapping to Host
 *                        in vpu_dec_init() and freed in vpu_dec_deinit()
 *                        by VPU.
 *                        AP-W/R : AP is writer/reader on this item
 *                        VPU-W/R: VPU is write/reader on this item
 * @dec          : decode information (AP-R, VPU-W)
 * @pic          : picture information (AP-R, VPU-W)
 * @crop         : crop information (AP-R, VPU-W)
 */
struct vdec_h264_vsi {
	unsigned char hdr_buf[HDR_PARSING_BUF_SZ];
	char *header_buf;
	int sps_size;
	int pps_size;
	int sei_size;
	int head_offset;
	struct vdec_h264_dec_info dec;
	struct vdec_pic_info pic;
	struct vdec_pic_info cur_pic;
	struct v4l2_rect crop;
	bool is_combine;
	int nalu_pos;
};

/**
 * struct vdec_h264_inst - h264 decoder instance
 * @num_nalu : how many nalus be decoded
 * @ctx      : point to aml_vcodec_ctx
 * @pred_buf : HW working predication buffer
 * @mv_buf   : HW working motion vector buffer
 * @vpu      : VPU instance
 * @vsi      : VPU shared information
 */
struct vdec_h264_inst {
	unsigned int num_nalu;
	struct aml_vcodec_ctx *ctx;
	struct aml_vcodec_mem pred_buf;
	struct aml_vcodec_mem mv_buf[H264_MAX_FB_NUM];
	struct aml_vdec_adapt vdec;
	struct vdec_h264_vsi *vsi;
	struct vcodec_vfm_s vfm;
	struct completion comp;
};

#if 0
#define DUMP_FILE_NAME "/data/dump/dump.tmp"
static struct file *filp;
static loff_t file_pos;

void dump_write(const char __user *buf, size_t count)
{
	mm_segment_t old_fs;

	if (!filp)
		return;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if (count != vfs_write(filp, buf, count, &file_pos))
		pr_err("Failed to write file\n");

	set_fs(old_fs);
}

void dump_init(void)
{
	filp = filp_open(DUMP_FILE_NAME, O_CREAT | O_RDWR, 0644);
	if (IS_ERR(filp)) {
		pr_err("open dump file failed\n");
		filp = NULL;
	}
}

void dump_deinit(void)
{
	if (filp) {
		filp_close(filp, current->files);
		filp = NULL;
		file_pos = 0;
	}
}

void swap_uv(void *uv, int size)
{
	int i;
	__u16 *p = uv;

	size /= 2;

	for (i = 0; i < size; i++, p++)
		*p = __swab16(*p);
}
#endif

static void get_pic_info(struct vdec_h264_inst *inst,
			 struct vdec_pic_info *pic)
{
	*pic = inst->vsi->pic;

	aml_vcodec_debug(inst, "pic(%d, %d), buf(%d, %d)",
			 pic->visible_width, pic->visible_height,
			 pic->coded_width, pic->coded_height);
	aml_vcodec_debug(inst, "Y(%d, %d), C(%d, %d)", pic->y_bs_sz,
			 pic->y_len_sz, pic->c_bs_sz, pic->c_len_sz);
}

static void get_crop_info(struct vdec_h264_inst *inst, struct v4l2_rect *cr)
{
	cr->left = inst->vsi->crop.left;
	cr->top = inst->vsi->crop.top;
	cr->width = inst->vsi->crop.width;
	cr->height = inst->vsi->crop.height;

	aml_vcodec_debug(inst, "l=%d, t=%d, w=%d, h=%d",
			 cr->left, cr->top, cr->width, cr->height);
}

static void get_dpb_size(struct vdec_h264_inst *inst, unsigned int *dpb_sz)
{
	*dpb_sz = inst->vsi->dec.dpb_sz;
	aml_vcodec_debug(inst, "sz=%d", *dpb_sz);
}

static void skip_aud_data(u8 **data, u32 *size)
{
	int i;

	i = find_start_code(*data, *size);
	if (i > 0 && (*data)[i++] == 0x9 && (*data)[i++] == 0xf0) {
		*size -= i;
		*data += i;
	}
}

static int vdec_h264_init(struct aml_vcodec_ctx *ctx, unsigned long *h_vdec)
{
	struct vdec_h264_inst *inst = NULL;
	int ret = -1;

	inst = kzalloc(sizeof(*inst), GFP_KERNEL);
	if (!inst)
		return -ENOMEM;

	inst->ctx = ctx;

	inst->vdec.video_type = VFORMAT_H264;
	inst->vdec.dev	= ctx->dev->vpu_plat_dev;
	inst->vdec.filp	= ctx->dev->filp;
	inst->vdec.ctx	= ctx;

	/* set play mode.*/
	if (ctx->is_drm_mode)
		inst->vdec.port.flag |= PORT_FLAG_DRM;

	/* init vfm */
	inst->vfm.ctx	= ctx;
	inst->vfm.ada_ctx = &inst->vdec;
	vcodec_vfm_init(&inst->vfm);

	ret = video_decoder_init(&inst->vdec);
	if (ret) {
		aml_vcodec_err(inst, "vdec_h264 init err=%d", ret);
		goto error_free_inst;
	}

	/* probe info from the stream */
	inst->vsi = kzalloc(sizeof(struct vdec_h264_vsi), GFP_KERNEL);
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

	aml_vcodec_debug(inst, "H264 Instance >> %p", inst);

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

#if 0
static int refer_buffer_num(int level_idc, int max_poc_cnt,
	int mb_width, int mb_height)
{
	int size;
	int pic_size = mb_width * mb_height * 384;

	switch (level_idc) {
	case 9:
		size = 152064;
		break;
	case 10:
		size = 152064;
		break;
	case 11:
		size = 345600;
		break;
	case 12:
		size = 912384;
		break;
	case 13:
		size = 912384;
		break;
	case 20:
		size = 912384;
		break;
	case 21:
		size = 1824768;
		break;
	case 22:
		size = 3110400;
		break;
	case 30:
		size = 3110400;
		break;
	case 31:
		size = 6912000;
		break;
	case 32:
		size = 7864320;
		break;
	case 40:
		size = 12582912;
		break;
	case 41:
		size = 12582912;
		break;
	case 42:
		size = 13369344;
		break;
	case 50:
		size = 42393600;
		break;
	case 51:
	case 52:
	default:
		size = 70778880;
		break;
	}

	size /= pic_size;
	size = size + 1; /* need more buffers */

	if (size > max_poc_cnt)
		size = max_poc_cnt;

	return size;
}
#endif

static void fill_vdec_params(struct vdec_h264_inst *inst, struct h264_SPS_t *sps)
{
	struct vdec_pic_info *pic = &inst->vsi->pic;
	struct vdec_h264_dec_info *dec = &inst->vsi->dec;
	struct v4l2_rect *rect = &inst->vsi->crop;
	u32 mb_w, mb_h, width, height;

	mb_w = sps->mb_width;
	mb_h = sps->mb_height;

	width  = mb_w << 4;
	height = mb_h << 4;

	width  -= (sps->crop_left + sps->crop_right);
	height -= (sps->crop_top + sps->crop_bottom);

	/* fill visible area size that be used for EGL. */
	pic->visible_width	= width;
	pic->visible_height	= height;

	/* calc visible ares. */
	rect->left		= 0;
	rect->top		= 0;
	rect->width		= pic->visible_width;
	rect->height		= pic->visible_height;

	/* config canvas size that be used for decoder. */
	pic->coded_width	= ALIGN(mb_w, 4) << 4;
	pic->coded_height	= ALIGN(mb_h, 4) << 4;
	pic->y_len_sz		= pic->coded_width * pic->coded_height;
	pic->c_len_sz		= pic->y_len_sz >> 1;

	/* calc DPB size */
	dec->dpb_sz = sps->ref_frame_count;

	aml_vcodec_debug(inst, "[%d] The stream infos, coded:(%d x %d), visible:(%d x %d), DPB: %d\n",
		inst->ctx->id, pic->coded_width, pic->coded_height,
		pic->visible_width, pic->visible_height, dec->dpb_sz);
}

static bool check_frame_combine(u8 *buf, u32 size, int *pos)
{
	bool combine = false;
	int i = 0, j = 0, cnt = 0;
	u8 *p = buf;

	for (i = 4; i < size; i++) {
		j = find_start_code(p, 7);
		if (j > 0) {
			if (++cnt > 1) {
				combine = true;
				break;
			}

			*pos = p - buf + j;
			p += j;
			i += j;
		}
		p++;
	}

	//pr_info("nal pos: %d, is_combine: %d\n",*pos, *is_combine);
	return combine;
}

static int stream_parse_by_ucode(struct vdec_h264_inst *inst, u8 *buf, u32 size)
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

static int stream_parse(struct vdec_h264_inst *inst, u8 *buf, u32 size)
{
	int ret = 0;
	struct h264_param_sets *ps;
	u32 nal_type;
	int nal_idx = 0;
	bool is_combine = false;

	is_combine = check_frame_combine(buf, size, &nal_idx);
	if (nal_idx < 0)
		return -1;

	nal_type = AVC_NAL_TYPE(buf[nal_idx]);
	if (nal_type != NAL_H264_SPS)
		return -1;

	/* if the st compose from csd + slice that is the combine data. */
	inst->vsi->is_combine = is_combine;
	inst->vsi->nalu_pos = nal_idx;

	ps = kzalloc(sizeof(struct h264_param_sets), GFP_KERNEL);
	if (ps == NULL)
		return -ENOMEM;

	ret = h264_decode_extradata_ps(buf, size, ps);
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

static int vdec_h264_probe(unsigned long h_vdec,
	struct aml_vcodec_mem *bs, void *out)
{
	struct vdec_h264_inst *inst =
		(struct vdec_h264_inst *)h_vdec;
	struct stream_info *st;
	u8 *buf = (u8 *)bs->vaddr;
	u32 size = bs->size;
	int ret = 0;

	st = (struct stream_info *)buf;
	if (inst->ctx->is_drm_mode && (st->magic == DRMe || st->magic == DRMn))
		return 0;

	if (st->magic == NORe || st->magic == NORn) {
		buf = st->data;
		size = st->length;
	}

	skip_aud_data(&buf, &size);

	if (inst->ctx->param_sets_from_ucode)
		ret = stream_parse_by_ucode(inst, buf, size);
	else
		ret = stream_parse(inst, buf, size);

	inst->vsi->cur_pic = inst->vsi->pic;

	return ret;
}

static void vdec_h264_deinit(unsigned long h_vdec)
{
	ulong flags;
	struct vdec_h264_inst *inst = (struct vdec_h264_inst *)h_vdec;
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

static int vdec_h264_get_fb(struct vdec_h264_inst *inst, struct vdec_v4l2_buffer **out)
{
	return get_fb_from_queue(inst->ctx, out);
}

static void vdec_h264_get_vf(struct vdec_h264_inst *inst, struct vdec_v4l2_buffer **out)
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

static int vdec_write_nalu(struct vdec_h264_inst *inst,
	u8 *buf, u32 size, u64 ts)
{
	int ret = -1;
	struct aml_vdec_adapt *vdec = &inst->vdec;
	bool is_combine = inst->vsi->is_combine;
	int nalu_pos;
	u32 nal_type;

	nalu_pos = find_start_code(buf, size);
	if (nalu_pos < 0)
		goto err;

	nal_type = AVC_NAL_TYPE(buf[nalu_pos]);
	//aml_vcodec_debug(inst, "NALU type: %d, size: %u", nal_type, size);

	if (nal_type == NAL_H264_SPS && !is_combine) {
		if (inst->vsi->head_offset + size > HEADER_BUFFER_SIZE) {
			ret = -EILSEQ;
			goto err;
		}
		inst->vsi->sps_size = size;
		memcpy(inst->vsi->header_buf + inst->vsi->head_offset, buf, size);
		inst->vsi->head_offset += inst->vsi->sps_size;
		ret = size;
	} else if (nal_type == NAL_H264_PPS && !is_combine) {
			//buf_sz -= nal_start_idx;
		if (inst->vsi->head_offset + size > HEADER_BUFFER_SIZE) {
			ret = -EILSEQ;
			goto err;
		}
		inst->vsi->pps_size = size;
		memcpy(inst->vsi->header_buf + inst->vsi->head_offset, buf, size);
		inst->vsi->head_offset += inst->vsi->pps_size;
		ret = size;
	} else if (nal_type == NAL_H264_SEI && !is_combine) {
		if (inst->vsi->head_offset + size > HEADER_BUFFER_SIZE) {
			ret = -EILSEQ;
			goto err;
		}
		inst->vsi->sei_size = size;
		memcpy(inst->vsi->header_buf + inst->vsi->head_offset, buf, size);
		inst->vsi->head_offset += inst->vsi->sei_size;
		ret = size;
	} else if (inst->vsi->head_offset == 0) {
		ret = vdec_vframe_write(vdec, buf, size, ts);
	} else {
		char *write_buf = vmalloc(inst->vsi->head_offset + size);
		if (!write_buf) {
			ret = -ENOMEM;
			goto err;
		}

		memcpy(write_buf, inst->vsi->header_buf, inst->vsi->head_offset);
		memcpy(write_buf + inst->vsi->head_offset, buf, size);

		ret = vdec_vframe_write(vdec, write_buf,
			inst->vsi->head_offset + size, ts);

		memset(inst->vsi->header_buf, 0, HEADER_BUFFER_SIZE);
		inst->vsi->head_offset = 0;
		inst->vsi->sps_size = 0;
		inst->vsi->pps_size = 0;
		inst->vsi->sei_size = 0;

		vfree(write_buf);
	}

	return ret;
err:
	aml_vcodec_err(inst, "%s err(%d)", __func__, ret);
	return ret;
}

static bool monitor_res_change(struct vdec_h264_inst *inst, u8 *buf, u32 size)
{
	int ret = 0, i = 0, j = 0;
	u8 *p = buf;
	int len = size;
	u32 type;

	for (i = 4; i < size; i++) {
		j = find_start_code(p, len);
		if (j > 0) {
			len = size - (p - buf);
			type = AVC_NAL_TYPE(p[j]);
			if (type != NAL_H264_AUD &&
				(type > NAL_H264_PPS || type < NAL_H264_SEI))
				break;

			if (type == NAL_H264_SPS) {
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

static int vdec_h264_decode(unsigned long h_vdec, struct aml_vcodec_mem *bs,
			 u64 timestamp, bool *res_chg)
{
	struct vdec_h264_inst *inst = (struct vdec_h264_inst *)h_vdec;
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

static int vdec_h264_get_param(unsigned long h_vdec,
			       enum vdec_get_param_type type, void *out)
{
	int ret = 0;
	struct vdec_h264_inst *inst = (struct vdec_h264_inst *)h_vdec;

	if (!inst) {
		pr_err("the h264 inst of dec is invalid.\n");
		return -1;
	}

	switch (type) {
	case GET_PARAM_DISP_FRAME_BUFFER:
		vdec_h264_get_vf(inst, out);
		break;

	case GET_PARAM_FREE_FRAME_BUFFER:
		ret = vdec_h264_get_fb(inst, out);
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

static void set_param_write_sync(struct vdec_h264_inst *inst)
{
	complete(&inst->comp);
}

static void set_param_pic_info(struct vdec_h264_inst *inst,
	struct aml_vdec_pic_infos *info)
{
	struct vdec_pic_info *pic = &inst->vsi->pic;
	struct vdec_h264_dec_info *dec = &inst->vsi->dec;
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

	dec->dpb_sz = info->dpb_size;

	/*wake up*/
	complete(&inst->comp);

	pr_info("Parse from ucode, crop(%d x %d), coded(%d x %d) dpb: %d\n",
		info->visible_width, info->visible_height,
		info->coded_width, info->coded_height,
		info->dpb_size);
}

static int vdec_h264_set_param(unsigned long h_vdec,
	enum vdec_set_param_type type, void *in)
{
	int ret = 0;
	struct vdec_h264_inst *inst = (struct vdec_h264_inst *)h_vdec;

	if (!inst) {
		pr_err("the h264 inst of dec is invalid.\n");
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

static struct vdec_common_if vdec_h264_if = {
	.init		= vdec_h264_init,
	.probe		= vdec_h264_probe,
	.decode		= vdec_h264_decode,
	.get_param	= vdec_h264_get_param,
	.set_param	= vdec_h264_set_param,
	.deinit		= vdec_h264_deinit,
};

struct vdec_common_if *get_h264_dec_comm_if(void);

struct vdec_common_if *get_h264_dec_comm_if(void)
{
	return &vdec_h264_if;
}
