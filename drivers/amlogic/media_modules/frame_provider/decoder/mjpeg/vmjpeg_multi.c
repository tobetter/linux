/*
 * drivers/amlogic/amports/vmjpeg.c
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
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
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/kfifo.h>
#include <linux/platform_device.h>
#include <linux/amlogic/media/frame_sync/ptsserv.h>
#include <linux/amlogic/media/utils/amstream.h>
#include <linux/amlogic/media/canvas/canvas.h>
#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/amlogic/media/vfm/vframe_provider.h>
#include <linux/amlogic/media/vfm/vframe_receiver.h>
#include <linux/amlogic/tee.h>

#include <linux/amlogic/media/utils/vdec_reg.h>
#include <linux/amlogic/media/registers/register.h>
#include "../../../stream_input/amports/amports_priv.h"

#include "../utils/vdec_input.h"
#include "../utils/vdec.h"
#include "../utils/amvdec.h"
#include "../utils/decoder_mmu_box.h"
#include "../utils/decoder_bmmu_box.h"
#include <linux/amlogic/media/codec_mm/codec_mm.h>
#include <linux/amlogic/media/codec_mm/configs.h>
#include "../utils/firmware.h"

#define MEM_NAME "codec_mmjpeg"

#define DRIVER_NAME "ammvdec_mjpeg"
#define MODULE_NAME "ammvdec_mjpeg"
#define CHECK_INTERVAL        (HZ/100)

/* protocol register usage
 *    AV_SCRATCH_4 : decode buffer spec
 *    AV_SCRATCH_5 : decode buffer index
 */

#define MREG_DECODE_PARAM   AV_SCRATCH_2	/* bit 0-3: pico_addr_mode */
/* bit 15-4: reference height */
#define MREG_TO_AMRISC      AV_SCRATCH_8
#define MREG_FROM_AMRISC    AV_SCRATCH_9
#define MREG_FRAME_OFFSET   AV_SCRATCH_A
#define DEC_STATUS_REG      AV_SCRATCH_J

#define PICINFO_BUF_IDX_MASK        0x0007
#define PICINFO_AVI1                0x0080
#define PICINFO_INTERLACE           0x0020
#define PICINFO_INTERLACE_AVI1_BOT  0x0010
#define PICINFO_INTERLACE_FIRST     0x0010

#define VF_POOL_SIZE          16
#define DECODE_BUFFER_NUM_MAX		4
#define MAX_BMMU_BUFFER_NUM		DECODE_BUFFER_NUM_MAX

#define DEFAULT_MEM_SIZE	(32*SZ_1M)
static int debug_enable;
#define DECODE_ID(hw) (hw_to_vdec(hw)->id)

static struct vframe_s *vmjpeg_vf_peek(void *);
static struct vframe_s *vmjpeg_vf_get(void *);
static void vmjpeg_vf_put(struct vframe_s *, void *);
static int vmjpeg_vf_states(struct vframe_states *states, void *);
static int vmjpeg_event_cb(int type, void *data, void *private_data);
static void vmjpeg_work(struct work_struct *work);
static int pre_decode_buf_level = 0x800;

static const char vmjpeg_dec_id[] = "vmmjpeg-dev";

#define PROVIDER_NAME   "vdec.mjpeg"
static const struct vframe_operations_s vf_provider_ops = {
	.peek = vmjpeg_vf_peek,
	.get = vmjpeg_vf_get,
	.put = vmjpeg_vf_put,
	.event_cb = vmjpeg_event_cb,
	.vf_states = vmjpeg_vf_states,
};

#define DEC_RESULT_NONE             0
#define DEC_RESULT_DONE             1
#define DEC_RESULT_AGAIN            2
#define DEC_RESULT_FORCE_EXIT       3
#define DEC_RESULT_EOS              4
#define DEC_DECODE_TIMEOUT         0x21


struct buffer_spec_s {
	unsigned int y_addr;
	unsigned int u_addr;
	unsigned int v_addr;

	int y_canvas_index;
	int u_canvas_index;
	int v_canvas_index;

	struct canvas_config_s canvas_config[3];
	unsigned long cma_alloc_addr;
	int cma_alloc_count;
	unsigned int buf_adr;
};

#define spec2canvas(x)  \
	(((x)->v_canvas_index << 16) | \
	 ((x)->u_canvas_index << 8)  | \
	 ((x)->y_canvas_index << 0))

struct vdec_mjpeg_hw_s {
	spinlock_t lock;
	struct mutex vmjpeg_mutex;

	struct platform_device *platform_dev;
	DECLARE_KFIFO(newframe_q, struct vframe_s *, VF_POOL_SIZE);
	DECLARE_KFIFO(display_q, struct vframe_s *, VF_POOL_SIZE);

	struct vframe_s vfpool[VF_POOL_SIZE];
	struct buffer_spec_s buffer_spec[DECODE_BUFFER_NUM_MAX];
	s32 vfbuf_use[DECODE_BUFFER_NUM_MAX];

	u32 frame_width;
	u32 frame_height;
	u32 frame_dur;
	u32 saved_resolution;
	u8 init_flag;
	u32 stat;
	u32 dec_result;
	unsigned long buf_start;
	u32 buf_size;
	void *mm_blk_handle;
	struct dec_sysinfo vmjpeg_amstream_dec_info;

	struct vframe_chunk_s *chunk;
	struct work_struct work;
	void (*vdec_cb)(struct vdec_s *, void *);
	void *vdec_cb_arg;
	struct firmware_s *fw;
	struct timer_list check_timer;
	unsigned decode_timeout_count;
	u8 eos;
	u32 frame_num, put_num;
};

static void set_frame_info(struct vdec_mjpeg_hw_s *hw, struct vframe_s *vf)
{
	vf->width = hw->frame_width;
	vf->height = hw->frame_height;
	vf->duration = hw->frame_dur;
	vf->ratio_control = 0;
	vf->duration_pulldown = 0;
	vf->flag = 0;

	vf->canvas0Addr = vf->canvas1Addr = -1;
	vf->plane_num = 3;

	vf->canvas0_config[0] = hw->buffer_spec[vf->index].canvas_config[0];
	vf->canvas0_config[1] = hw->buffer_spec[vf->index].canvas_config[1];
	vf->canvas0_config[2] = hw->buffer_spec[vf->index].canvas_config[2];

	vf->canvas1_config[0] = hw->buffer_spec[vf->index].canvas_config[0];
	vf->canvas1_config[1] = hw->buffer_spec[vf->index].canvas_config[1];
	vf->canvas1_config[2] = hw->buffer_spec[vf->index].canvas_config[2];
}

static irqreturn_t vmjpeg_isr(struct vdec_s *vdec, int irq)
{
	struct vdec_mjpeg_hw_s *hw = (struct vdec_mjpeg_hw_s *)(vdec->private);
	u32 reg;
	struct vframe_s *vf = NULL;
	u32 index, offset = 0, pts;
	u64 pts_us64;

	if (debug_enable & 0x1)
		pr_info("%s: %d\n", __func__, __LINE__);

	WRITE_VREG(ASSIST_MBOX1_CLR_REG, 1);

	if (!hw)
		return IRQ_HANDLED;
	if (hw->eos)
		return IRQ_HANDLED;
	reg = READ_VREG(MREG_FROM_AMRISC);
	index = READ_VREG(AV_SCRATCH_5);

	if (index >= DECODE_BUFFER_NUM_MAX) {
		pr_err("fatal error, invalid buffer index.");
		return IRQ_HANDLED;
	}

	if (kfifo_get(&hw->newframe_q, &vf) == 0) {
		pr_info(
		"fatal error, no available buffer slot.");
		return IRQ_HANDLED;
	}

	vf->index = index;
	set_frame_info(hw, vf);

	vf->type = VIDTYPE_PROGRESSIVE | VIDTYPE_VIU_FIELD;
	/* vf->pts = (pts_valid) ? pts : 0; */
	/* vf->pts_us64 = (pts_valid) ? pts_us64 : 0; */

	if (hw->chunk) {
		vf->pts = hw->chunk->pts;
		vf->pts_us64 = hw->chunk->pts64;
	} else {
		offset = READ_VREG(MREG_FRAME_OFFSET);
		if (pts_lookup_offset_us64
			(PTS_TYPE_VIDEO, offset, &pts, 3000,
			&pts_us64) == 0) {
			vf->pts = pts;
			vf->pts_us64 = pts_us64;
		} else {
			vf->pts = 0;
			vf->pts_us64 = 0;
		}
	}
	vf->orientation = 0;
	hw->vfbuf_use[index]++;

	kfifo_put(&hw->display_q, (const struct vframe_s *)vf);

	hw->frame_num++;
	vf_notify_receiver(vdec->vf_provider_name,
			VFRAME_EVENT_PROVIDER_VFRAME_READY,
			NULL);

	hw->dec_result = DEC_RESULT_DONE;

	schedule_work(&hw->work);

	return IRQ_HANDLED;
}

static struct vframe_s *vmjpeg_vf_peek(void *op_arg)
{
	struct vframe_s *vf;
	struct vdec_s *vdec = op_arg;
	struct vdec_mjpeg_hw_s *hw = (struct vdec_mjpeg_hw_s *)vdec->private;

	if (!hw)
		return NULL;

	if (kfifo_peek(&hw->display_q, &vf))
		return vf;

	return NULL;
}

static struct vframe_s *vmjpeg_vf_get(void *op_arg)
{
	struct vframe_s *vf;
	struct vdec_s *vdec = op_arg;
	struct vdec_mjpeg_hw_s *hw = (struct vdec_mjpeg_hw_s *)vdec->private;

	if (!hw)
		return NULL;

	if (kfifo_get(&hw->display_q, &vf))
		return vf;

	return NULL;
}

static void vmjpeg_vf_put(struct vframe_s *vf, void *op_arg)
{
	struct vdec_s *vdec = op_arg;
	struct vdec_mjpeg_hw_s *hw = (struct vdec_mjpeg_hw_s *)vdec->private;

	hw->vfbuf_use[vf->index]--;
	kfifo_put(&hw->newframe_q, (const struct vframe_s *)vf);
	hw->put_num++;
}

static int vmjpeg_event_cb(int type, void *data, void *private_data)
{
	return 0;
}

static int vmjpeg_vf_states(struct vframe_states *states, void *op_arg)
{
	unsigned long flags;
	struct vdec_s *vdec = op_arg;
	struct vdec_mjpeg_hw_s *hw = (struct vdec_mjpeg_hw_s *)vdec->private;

	spin_lock_irqsave(&hw->lock, flags);

	states->vf_pool_size = VF_POOL_SIZE;
	states->buf_free_num = kfifo_len(&hw->newframe_q);
	states->buf_avail_num = kfifo_len(&hw->display_q);
	states->buf_recycle_num = 0;

	spin_unlock_irqrestore(&hw->lock, flags);

	return 0;
}

static int vmjpeg_dec_status(struct vdec_s *vdec, struct vdec_info *vstatus)
{
	struct vdec_mjpeg_hw_s *hw = (struct vdec_mjpeg_hw_s *)vdec->private;
	vstatus->frame_width = hw->frame_width;
	vstatus->frame_height = hw->frame_height;
	if (0 != hw->frame_dur)
		vstatus->frame_rate = 96000 / hw->frame_dur;
	else
		vstatus->frame_rate = 96000;
	vstatus->error_count = 0;
	vstatus->status = hw->stat;

	return 0;
}

/****************************************/
static void vmjpeg_canvas_init(struct vdec_s *vdec)
{
	int i, ret;
	u32 canvas_width, canvas_height;
	u32 decbuf_size, decbuf_y_size, decbuf_uv_size;
	unsigned long buf_start, addr;
	struct vdec_mjpeg_hw_s *hw =
		(struct vdec_mjpeg_hw_s *)vdec->private;

	canvas_width = 1920;
	canvas_height = 1088;
	decbuf_y_size = 0x200000;
	decbuf_uv_size = 0x80000;
	decbuf_size = 0x300000;

	for (i = 0; i < DECODE_BUFFER_NUM_MAX; i++) {
		int canvas;

		canvas = vdec->get_canvas(i, 3);

		ret = decoder_bmmu_box_alloc_buf_phy(hw->mm_blk_handle, i,
				decbuf_size, DRIVER_NAME, &buf_start);
		if (ret < 0) {
			pr_err("CMA alloc failed! size 0x%d  idx %d\n",
				decbuf_size, i);
			return;
		}

		hw->buffer_spec[i].buf_adr = buf_start;
		addr = hw->buffer_spec[i].buf_adr;

		hw->buffer_spec[i].y_addr = addr;
		addr += decbuf_y_size;
		hw->buffer_spec[i].u_addr = addr;
		addr += decbuf_uv_size;
		hw->buffer_spec[i].v_addr = addr;

		hw->buffer_spec[i].y_canvas_index = canvas_y(canvas);
		hw->buffer_spec[i].u_canvas_index = canvas_u(canvas);
		hw->buffer_spec[i].v_canvas_index = canvas_v(canvas);

		canvas_config(hw->buffer_spec[i].y_canvas_index,
			hw->buffer_spec[i].y_addr,
			canvas_width,
			canvas_height,
			CANVAS_ADDR_NOWRAP,
			CANVAS_BLKMODE_LINEAR);
		hw->buffer_spec[i].canvas_config[0].phy_addr =
			hw->buffer_spec[i].y_addr;
		hw->buffer_spec[i].canvas_config[0].width =
			canvas_width;
		hw->buffer_spec[i].canvas_config[0].height =
			canvas_height;
		hw->buffer_spec[i].canvas_config[0].block_mode =
			CANVAS_BLKMODE_LINEAR;

		canvas_config(hw->buffer_spec[i].u_canvas_index,
			hw->buffer_spec[i].u_addr,
			canvas_width / 2,
			canvas_height / 2,
			CANVAS_ADDR_NOWRAP,
			CANVAS_BLKMODE_LINEAR);
		hw->buffer_spec[i].canvas_config[1].phy_addr =
			hw->buffer_spec[i].u_addr;
		hw->buffer_spec[i].canvas_config[1].width =
			canvas_width / 2;
		hw->buffer_spec[i].canvas_config[1].height =
			canvas_height / 2;
		hw->buffer_spec[i].canvas_config[1].block_mode =
			CANVAS_BLKMODE_LINEAR;

		canvas_config(hw->buffer_spec[i].v_canvas_index,
			hw->buffer_spec[i].v_addr,
			canvas_width / 2,
			canvas_height / 2,
			CANVAS_ADDR_NOWRAP,
			CANVAS_BLKMODE_LINEAR);
		hw->buffer_spec[i].canvas_config[2].phy_addr =
			hw->buffer_spec[i].v_addr;
		hw->buffer_spec[i].canvas_config[2].width =
			canvas_width / 2;
		hw->buffer_spec[i].canvas_config[2].height =
			canvas_height / 2;
		hw->buffer_spec[i].canvas_config[2].block_mode =
			CANVAS_BLKMODE_LINEAR;
	}
}

static void init_scaler(void)
{
	/* 4 point triangle */
	const unsigned int filt_coef[] = {
		0x20402000, 0x20402000, 0x1f3f2101, 0x1f3f2101,
		0x1e3e2202, 0x1e3e2202, 0x1d3d2303, 0x1d3d2303,
		0x1c3c2404, 0x1c3c2404, 0x1b3b2505, 0x1b3b2505,
		0x1a3a2606, 0x1a3a2606, 0x19392707, 0x19392707,
		0x18382808, 0x18382808, 0x17372909, 0x17372909,
		0x16362a0a, 0x16362a0a, 0x15352b0b, 0x15352b0b,
		0x14342c0c, 0x14342c0c, 0x13332d0d, 0x13332d0d,
		0x12322e0e, 0x12322e0e, 0x11312f0f, 0x11312f0f,
		0x10303010
	};
	int i;

	/* pscale enable, PSCALE cbus bmem enable */
	WRITE_VREG(PSCALE_CTRL, 0xc000);

	/* write filter coefs */
	WRITE_VREG(PSCALE_BMEM_ADDR, 0);
	for (i = 0; i < 33; i++) {
		WRITE_VREG(PSCALE_BMEM_DAT, 0);
		WRITE_VREG(PSCALE_BMEM_DAT, filt_coef[i]);
	}

	/* Y horizontal initial info */
	WRITE_VREG(PSCALE_BMEM_ADDR, 37 * 2);
	/* [35]: buf repeat pix0,
	 * [34:29] => buf receive num,
	 * [28:16] => buf blk x,
	 * [15:0] => buf phase
	 */
	WRITE_VREG(PSCALE_BMEM_DAT, 0x0008);
	WRITE_VREG(PSCALE_BMEM_DAT, 0x60000000);

	/* C horizontal initial info */
	WRITE_VREG(PSCALE_BMEM_ADDR, 41 * 2);
	WRITE_VREG(PSCALE_BMEM_DAT, 0x0008);
	WRITE_VREG(PSCALE_BMEM_DAT, 0x60000000);

	/* Y vertical initial info */
	WRITE_VREG(PSCALE_BMEM_ADDR, 39 * 2);
	WRITE_VREG(PSCALE_BMEM_DAT, 0x0008);
	WRITE_VREG(PSCALE_BMEM_DAT, 0x60000000);

	/* C vertical initial info */
	WRITE_VREG(PSCALE_BMEM_ADDR, 43 * 2);
	WRITE_VREG(PSCALE_BMEM_DAT, 0x0008);
	WRITE_VREG(PSCALE_BMEM_DAT, 0x60000000);

	/* Y horizontal phase step */
	WRITE_VREG(PSCALE_BMEM_ADDR, 36 * 2 + 1);
	/* [19:0] => Y horizontal phase step */
	WRITE_VREG(PSCALE_BMEM_DAT, 0x10000);
	/* C horizontal phase step */
	WRITE_VREG(PSCALE_BMEM_ADDR, 40 * 2 + 1);
	/* [19:0] => C horizontal phase step */
	WRITE_VREG(PSCALE_BMEM_DAT, 0x10000);

	/* Y vertical phase step */
	WRITE_VREG(PSCALE_BMEM_ADDR, 38 * 2 + 1);
	/* [19:0] => Y vertical phase step */
	WRITE_VREG(PSCALE_BMEM_DAT, 0x10000);
	/* C vertical phase step */
	WRITE_VREG(PSCALE_BMEM_ADDR, 42 * 2 + 1);
	/* [19:0] => C horizontal phase step */
	WRITE_VREG(PSCALE_BMEM_DAT, 0x10000);

	/* reset pscaler */
#if 1/*MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6*/
	WRITE_VREG(DOS_SW_RESET0, (1 << 10));
	WRITE_VREG(DOS_SW_RESET0, 0);
#else
	WRITE_RESET_REG(RESET2_REGISTER, RESET_PSCALE);
#endif
	READ_RESET_REG(RESET2_REGISTER);
	READ_RESET_REG(RESET2_REGISTER);
	READ_RESET_REG(RESET2_REGISTER);

	WRITE_VREG(PSCALE_RST, 0x7);
	WRITE_VREG(PSCALE_RST, 0x0);
}

static void timeout_process(struct vdec_mjpeg_hw_s *hw)
{
	amvdec_stop();
	pr_info("%s decoder timeout\n", __func__);
	hw->dec_result = DEC_RESULT_DONE;

	vdec_schedule_work(&hw->work);
}

static void check_timer_func(unsigned long arg)
{
	struct vdec_mjpeg_hw_s *hw = (struct vdec_mjpeg_hw_s *)arg;
	struct vdec_s *vdec = hw_to_vdec(hw);
	if ((debug_enable & 0x2) != 0) {
		pr_info("%s: status:nstatus=%d:%d\n",
			__func__, vdec->status, vdec->next_status);
		pr_info("%s: %d,buftl=%x:%x:%x:%x\n",
			__func__, __LINE__,
			READ_VREG(VLD_MEM_VIFIFO_BUF_CNTL),
			READ_PARSER_REG(PARSER_VIDEO_WP),
			READ_VREG(VLD_MEM_VIFIFO_LEVEL),
			READ_VREG(VLD_MEM_VIFIFO_WP));
	}
	if ((debug_enable & 0x100) != 0) {
		hw->dec_result = DEC_RESULT_DONE;
		vdec_schedule_work(&hw->work);
		pr_info("vdec %d is forced to be disconnected\n",
			debug_enable & 0xff);
		debug_enable = 0;
		return;
	}

	if (input_stream_based(vdec)
		 && READ_VREG(VLD_MEM_VIFIFO_LEVEL) <= 0x80) {
		if (hw->decode_timeout_count > 0)
			hw->decode_timeout_count--;
		if (hw->decode_timeout_count == 0)
			timeout_process(hw);
	}

	if (READ_VREG(DEC_STATUS_REG) == DEC_DECODE_TIMEOUT) {
		pr_info("ucode DEC_DECODE_TIMEOUT\n");
		if (hw->decode_timeout_count > 0)
			hw->decode_timeout_count--;
		if (hw->decode_timeout_count == 0)
			timeout_process(hw);
		WRITE_VREG(DEC_STATUS_REG, 0);
	}

	if (vdec->next_status == VDEC_STATUS_DISCONNECTED) {
		hw->dec_result = DEC_RESULT_FORCE_EXIT;
		vdec_schedule_work(&hw->work);
		pr_info("vdec requested to be disconnected\n");
		return;
	}
	mod_timer(&hw->check_timer, jiffies + CHECK_INTERVAL);
}
static void vmjpeg_hw_ctx_restore(struct vdec_s *vdec, int index)
{
	struct vdec_mjpeg_hw_s *hw =
		(struct vdec_mjpeg_hw_s *)vdec->private;

	WRITE_VREG(DOS_SW_RESET0, (1 << 7) | (1 << 6));
	WRITE_VREG(DOS_SW_RESET0, 0);

	vmjpeg_canvas_init(vdec);

	/* find next decode buffer index */
	WRITE_VREG(AV_SCRATCH_4, spec2canvas(&hw->buffer_spec[index]));
	WRITE_VREG(AV_SCRATCH_5, index);

	init_scaler();

	/* clear buffer IN/OUT registers */
	WRITE_VREG(MREG_TO_AMRISC, 0);
	WRITE_VREG(MREG_FROM_AMRISC, 0);

	WRITE_VREG(MCPU_INTR_MSK, 0xffff);
	WRITE_VREG(MREG_DECODE_PARAM, (hw->frame_height << 4) | 0x8000);

	/* clear mailbox interrupt */
	WRITE_VREG(ASSIST_MBOX1_CLR_REG, 1);
	/* enable mailbox interrupt */
	WRITE_VREG(ASSIST_MBOX1_MASK, 1);
	/* set interrupt mapping for vld */
	WRITE_VREG(ASSIST_AMR1_INT8, 8);
#if 1/*MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6*/
	CLEAR_VREG_MASK(MDEC_PIC_DC_CTRL, 1 << 17);
#endif
}

static s32 vmjpeg_init(struct vdec_s *vdec)
{
	int i;
	int size = -1, fw_size = 0x1000 * 16;
	struct firmware_s *fw = NULL;
	struct vdec_mjpeg_hw_s *hw =
		(struct vdec_mjpeg_hw_s *)vdec->private;

	fw = vmalloc(sizeof(struct firmware_s) + fw_size);
	if (IS_ERR_OR_NULL(fw))
		return -ENOMEM;

	size = get_firmware_data(VIDEO_DEC_MJPEG_MULTI, fw->data);
	if (size < 0) {
		pr_err("get firmware fail.");
		vfree(fw);
		return -1;
	}

	fw->len = size;
	hw->fw = fw;

	hw->frame_width = hw->vmjpeg_amstream_dec_info.width;
	hw->frame_height = hw->vmjpeg_amstream_dec_info.height;
	hw->frame_dur = hw->vmjpeg_amstream_dec_info.rate;
	hw->saved_resolution = 0;
	hw->eos = 0;
	hw->init_flag = 0;
	hw->frame_num = 0;
	hw->put_num = 0;
	for (i = 0; i < DECODE_BUFFER_NUM_MAX; i++)
		hw->vfbuf_use[i] = 0;

	INIT_KFIFO(hw->display_q);
	INIT_KFIFO(hw->newframe_q);

	for (i = 0; i < VF_POOL_SIZE; i++) {
		const struct vframe_s *vf = &hw->vfpool[i];

		hw->vfpool[i].index = -1;
		kfifo_put(&hw->newframe_q, vf);
	}

	if (hw->mm_blk_handle) {
		decoder_bmmu_box_free(hw->mm_blk_handle);
		hw->mm_blk_handle = NULL;
	}

	hw->mm_blk_handle = decoder_bmmu_box_alloc_box(
		DRIVER_NAME,
		0,
		MAX_BMMU_BUFFER_NUM,
		4 + PAGE_SHIFT,
		CODEC_MM_FLAGS_CMA_CLEAR |
		CODEC_MM_FLAGS_FOR_VDECODER);

	init_timer(&hw->check_timer);

	hw->check_timer.data = (unsigned long)hw;
	hw->check_timer.function = check_timer_func;
	hw->check_timer.expires = jiffies + CHECK_INTERVAL;
	/*add_timer(&hw->check_timer);*/
	hw->stat |= STAT_TIMER_ARM;

	INIT_WORK(&hw->work, vmjpeg_work);

	return 0;
}

static unsigned long run_ready(struct vdec_s *vdec, unsigned long mask)
{
	struct vdec_mjpeg_hw_s *hw =
		(struct vdec_mjpeg_hw_s *)vdec->private;
	if (hw->eos)
		return 0;
	if (vdec_stream_based(vdec) && (hw->init_flag == 0)
		&& pre_decode_buf_level != 0) {
		u32 rp, wp, level;

		rp = READ_PARSER_REG(PARSER_VIDEO_RP);
		wp = READ_PARSER_REG(PARSER_VIDEO_WP);
		if (wp < rp)
			level = vdec->input.size + wp - rp;
		else
			level = wp - rp;

		if (level < pre_decode_buf_level)
			return 0;
	}
	return CORE_MASK_VDEC_1 | CORE_MASK_HEVC;
}

static void run(struct vdec_s *vdec, unsigned long mask,
	void (*callback)(struct vdec_s *, void *), void *arg)
{
	struct vdec_mjpeg_hw_s *hw =
		(struct vdec_mjpeg_hw_s *)vdec->private;
	int i,ret = -1;

	hw->vdec_cb_arg = arg;
	hw->vdec_cb = callback;

	for (i = 0; i < DECODE_BUFFER_NUM_MAX; i++) {
		if (hw->vfbuf_use[i] == 0)
			break;
	}

	if (i == DECODE_BUFFER_NUM_MAX) {
		hw->dec_result = DEC_RESULT_AGAIN;
		schedule_work(&hw->work);
		return;
	}

	ret = vdec_prepare_input(vdec, &hw->chunk);
	if (ret < 0) {
		if (debug_enable & 0x1) {
			pr_info("%s: %d,r=%d,buftl=%x:%x:%x\n",
			__func__, __LINE__, ret,
			READ_VREG(VLD_MEM_VIFIFO_BUF_CNTL),
			READ_PARSER_REG(PARSER_VIDEO_WP),
			READ_VREG(VLD_MEM_VIFIFO_WP));
		}
		hw->dec_result = DEC_RESULT_AGAIN;
		schedule_work(&hw->work);
		return;
	}

	hw->dec_result = DEC_RESULT_NONE;

	ret = amvdec_vdec_loadmc_ex(VFORMAT_MJPEG, "mmjpeg", vdec, hw->fw->data);
	if (ret < 0) {
		pr_err("[%d] MMJPEG: the %s fw loading failed, err: %x\n",
			vdec->id, tee_enabled() ? "TEE" : "local", ret);
		return;
	}
/*	if (amvdec_vdec_loadmc_buf_ex(vdec, hw->fw->data, hw->fw->len) < 0) {
		pr_err("%s: Error amvdec_loadmc fail\n", __func__);
		return;
	}*/

	vmjpeg_hw_ctx_restore(vdec, i);
#if 0
	vdec_enable_input(vdec);
	mod_timer(&hw->check_timer, jiffies + CHECK_INTERVAL);
#endif
	amvdec_start();
	vdec_enable_input(vdec);
	mod_timer(&hw->check_timer, jiffies + CHECK_INTERVAL);
	hw->decode_timeout_count = 2;
	hw->init_flag = 1;
	if (debug_enable&0x1) {

		pr_info("%s (0x%x 0x%x 0x%x) vldcrl 0x%x bitcnt 0x%x powerctl 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			__func__,
			READ_VREG(VLD_MEM_VIFIFO_LEVEL),
			READ_VREG(VLD_MEM_VIFIFO_WP),
			READ_VREG(VLD_MEM_VIFIFO_RP),
			READ_VREG(VLD_DECODE_CONTROL),
			READ_VREG(VIFF_BIT_CNT),
			READ_VREG(POWER_CTL_VLD),
			READ_VREG(VLD_MEM_VIFIFO_START_PTR),
			READ_VREG(VLD_MEM_VIFIFO_CURR_PTR),
			READ_VREG(VLD_MEM_VIFIFO_CONTROL),
			READ_VREG(VLD_MEM_VIFIFO_BUF_CNTL),
			READ_VREG(VLD_MEM_VIFIFO_END_PTR));

	}
}

static void vmjpeg_work(struct work_struct *work)
{
	struct vdec_mjpeg_hw_s *hw = container_of(work,
	struct vdec_mjpeg_hw_s, work);
	if (debug_enable & 0x2)
		pr_info("%s: result=%d,len=%d:%d\n",
			__func__, hw->dec_result,
			kfifo_len(&hw->newframe_q),
			kfifo_len(&hw->display_q));
	if (hw->dec_result == DEC_RESULT_DONE)
		vdec_vframe_dirty(hw_to_vdec(hw), hw->chunk);
	else if (hw->dec_result == DEC_RESULT_AGAIN) {
		/*
			stream base: stream buf empty or timeout
			frame base: vdec_prepare_input fail
		*/
		if (!vdec_has_more_input(hw_to_vdec(hw))) {
			hw->dec_result = DEC_RESULT_EOS;
			vdec_schedule_work(&hw->work);
			/*pr_info("%s: return\n",
			__func__);*/
			return;
		}
	} else if (hw->dec_result == DEC_RESULT_FORCE_EXIT) {
		pr_info("%s: force exit\n",
			__func__);
	} else if (hw->dec_result == DEC_RESULT_EOS) {
		/*pr_info("%s: end of stream\n",
			__func__);*/
		if (READ_VREG(VLD_MEM_VIFIFO_LEVEL) < 0x100)
			hw->eos = 1;
		vdec_vframe_dirty(hw_to_vdec(hw), hw->chunk);
	}
	amvdec_stop();
	/* mark itself has all HW resource released and input released */
	vdec_core_finish_run(hw_to_vdec(hw), CORE_MASK_VDEC_1
			| CORE_MASK_HEVC);
	del_timer_sync(&hw->check_timer);
	hw->stat &= ~STAT_TIMER_ARM;

	if (hw->vdec_cb)
		hw->vdec_cb(hw_to_vdec(hw), hw->vdec_cb_arg);
}

static int amvdec_mjpeg_probe(struct platform_device *pdev)
{
	struct vdec_s *pdata = *(struct vdec_s **)pdev->dev.platform_data;
	struct vdec_mjpeg_hw_s *hw = NULL;

	if (pdata == NULL) {
		pr_info("amvdec_mjpeg memory resource undefined.\n");
		return -EFAULT;
	}

	hw = (struct vdec_mjpeg_hw_s *)devm_kzalloc(&pdev->dev,
		sizeof(struct vdec_mjpeg_hw_s), GFP_KERNEL);
	if (hw == NULL) {
		pr_info("\nammvdec_mjpeg device data allocation failed\n");
		return -ENOMEM;
	}

	pdata->private = hw;
	pdata->dec_status = vmjpeg_dec_status;

	pdata->run = run;
	pdata->run_ready = run_ready;
	pdata->irq_handler = vmjpeg_isr;


	if (pdata->use_vfm_path)
		snprintf(pdata->vf_provider_name, VDEC_PROVIDER_NAME_SIZE,
			VFM_DEC_PROVIDER_NAME);
	else
		snprintf(pdata->vf_provider_name, VDEC_PROVIDER_NAME_SIZE,
			PROVIDER_NAME ".%02x", pdev->id & 0xff);

	vf_provider_init(&pdata->vframe_provider, pdata->vf_provider_name,
		&vf_provider_ops, pdata);

	platform_set_drvdata(pdev, pdata);

	hw->platform_dev = pdev;

	if (pdata->sys_info)
		hw->vmjpeg_amstream_dec_info = *pdata->sys_info;

	if (vmjpeg_init(pdata) < 0) {
		pr_info("amvdec_mjpeg init failed.\n");
		return -ENODEV;
	}

	vdec_core_request(pdata, CORE_MASK_VDEC_1 | CORE_MASK_HEVC
				| CORE_MASK_COMBINE);

	return 0;
}

static int amvdec_mjpeg_remove(struct platform_device *pdev)
{
	struct vdec_mjpeg_hw_s *hw =
		(struct vdec_mjpeg_hw_s *)
		(((struct vdec_s *)(platform_get_drvdata(pdev)))->private);


	if (hw->stat & STAT_TIMER_ARM) {
		del_timer_sync(&hw->check_timer);
		hw->stat &= ~STAT_TIMER_ARM;
	}
	cancel_work_sync(&hw->work);
	if (hw->mm_blk_handle) {
		decoder_bmmu_box_free(hw->mm_blk_handle);
		hw->mm_blk_handle = NULL;
	}

	vfree(hw->fw);
	hw->fw = NULL;

	vdec_core_release(hw_to_vdec(hw), CORE_MASK_VDEC_1 | CORE_MASK_HEVC);
	vdec_set_status(hw_to_vdec(hw), VDEC_STATUS_DISCONNECTED);

	return 0;
}

/****************************************/

static struct platform_driver amvdec_mjpeg_driver = {
	.probe = amvdec_mjpeg_probe,
	.remove = amvdec_mjpeg_remove,
#ifdef CONFIG_PM
	.suspend = amvdec_suspend,
	.resume = amvdec_resume,
#endif
	.driver = {
		.name = DRIVER_NAME,
	}
};

static struct codec_profile_t amvdec_mjpeg_profile = {
	.name = "mmjpeg",
	.profile = ""
};

static int __init amvdec_mjpeg_driver_init_module(void)
{
	if (platform_driver_register(&amvdec_mjpeg_driver)) {
		pr_err("failed to register amvdec_mjpeg driver\n");
		return -ENODEV;
	}
	vcodec_profile_register(&amvdec_mjpeg_profile);
	return 0;
}

static void __exit amvdec_mjpeg_driver_remove_module(void)
{
	platform_driver_unregister(&amvdec_mjpeg_driver);
}

/****************************************/
module_param(debug_enable, uint, 0664);
MODULE_PARM_DESC(debug_enable, "\n debug enable\n");
module_param(pre_decode_buf_level, int, 0664);
MODULE_PARM_DESC(pre_decode_buf_level,
		"\n ammvdec_h264 pre_decode_buf_level\n");
module_init(amvdec_mjpeg_driver_init_module);
module_exit(amvdec_mjpeg_driver_remove_module);

MODULE_DESCRIPTION("AMLOGIC MJMPEG Video Decoder Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tim Yao <timyao@amlogic.com>");
