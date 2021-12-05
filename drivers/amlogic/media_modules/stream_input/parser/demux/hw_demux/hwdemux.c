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
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/poll.h>
#include <linux/crc32.h>
#include <linux/clk.h>

#include <asm/uaccess.h>
#include <asm/div64.h>

//#include "c_stb_define.h"
//#include "c_stb_regs_define.h"
#include "dvb_reg.h"

#include "../aml_dvb.h"
#include "hwdemux.h"
#include "hwdemux_internal.h"

#include "s2p.h"
#include "frontend.h"
#include "asyncfifo.h"

#define DEMUX_COUNT				  3
#define USE_AHB_MODE

#define CIPLUS_OUT_SEL    28
#define CIPLUS_IN_SEL     26

#define DMX_TYPE_TS  0
#define DMX_TYPE_SEC 1
#define DMX_TYPE_PES 2

#define pr_dbg_flag(_f, _args...)\
	do {\
		if (debug_dmx &(_f))\
			printk(_args);\
	} while (0)
#define pr_dbg_irq_flag(_f, _args...)\
	do {\
		if (debug_irq&(_f))\
			printk(_args);\
	} while (0)
#define pr_dbg(args...)	pr_dbg_flag(0x1, args)

#define pr_error(fmt, args...) printk("DVB: " fmt, ## args)
#define pr_inf(fmt, args...)   printk("DVB: " fmt, ## args)

#define IS_SRC_DMX(_src) ((_src) >= AM_TS_SRC_DMX0 && (_src) <= AM_TS_SRC_DMX2)

static int npids = CHANNEL_COUNT;
#define MOD_PARAM_DECLARE_CHANPIDS(_dmx) \
MODULE_PARM_DESC(debug_dmx##_dmx##_chanpids, "\n\t\t pids of dmx channels"); \
static short debug_dmx##_dmx##_chanpids[CHANNEL_COUNT] = \
					{[0 ... (CHANNEL_COUNT - 1)] = -1}; \
module_param_array(debug_dmx##_dmx##_chanpids, short, &npids, 0444)

MOD_PARAM_DECLARE_CHANPIDS(0);
MOD_PARAM_DECLARE_CHANPIDS(1);
MOD_PARAM_DECLARE_CHANPIDS(2);

#define set_debug_dmx_chanpids(_dmx, _idx, _pid)\
	do { \
		if ((_dmx) == 0) \
			debug_dmx0_chanpids[(_idx)] = (_pid); \
		else if ((_dmx) == 1) \
			debug_dmx1_chanpids[(_idx)] = (_pid); \
		else if ((_dmx) == 2) \
			debug_dmx2_chanpids[(_idx)] = (_pid); \
	} while (0)

static u32 old_stb_top_config;
static u32 old_fec_input_control;
static int have_old_stb_top_config = 1;
static int have_old_fec_input_control = 1;
static int dmx_count = 0;

MODULE_PARM_DESC(disable_dsc, "\n\t\t Disable discrambler");
static int disable_dsc;
module_param(disable_dsc, int, 0644);

static void
_hwdmx_write_reg(int r, u32 v)
{
	u32 oldv, mask;

	if (disable_dsc) {
		if (r == STB_TOP_CONFIG) {
			if (have_old_stb_top_config) {
				oldv = old_stb_top_config;
				have_old_stb_top_config = 0;
			} else {
				oldv = READ_MPEG_REG(STB_TOP_CONFIG);
			}

			mask = (1<<7)|(1<<15)|(3<<26)|(7<<28);
			v	 &= ~mask;
			v	 |= (oldv & mask);
		} else if (r == FEC_INPUT_CONTROL) {
			if (have_old_fec_input_control) {
				oldv = old_fec_input_control;
				have_old_fec_input_control = 0;
			} else {
				oldv = READ_MPEG_REG(FEC_INPUT_CONTROL);
			}

			mask = (1<<15);
			v	&= ~mask;
			v	|= (oldv & mask);
		} else if ((r == RESET1_REGISTER) || (r == RESET3_REGISTER)) {
			if (!have_old_stb_top_config) {
				have_old_stb_top_config = 1;
				old_stb_top_config =
					READ_MPEG_REG(STB_TOP_CONFIG);
			}
			if (!have_old_fec_input_control) {
				have_old_fec_input_control = 1;
				old_fec_input_control =
					READ_MPEG_REG(FEC_INPUT_CONTROL);
			}
		} else if ((r == TS_PL_PID_INDEX) || (r == TS_PL_PID_DATA)
					|| (r == COMM_DESC_KEY0)
					|| (r == COMM_DESC_KEY1)
					|| (r == COMM_DESC_KEY_RW)
					|| (r == CIPLUS_KEY0)
					|| (r == CIPLUS_KEY1)
					|| (r == CIPLUS_KEY2)
					|| (r == CIPLUS_KEY3)
					|| (r == CIPLUS_KEY_WR)
					|| (r == CIPLUS_CONFIG)
					|| (r == CIPLUS_ENDIAN)) {
			return;
		}
	}

	WRITE_MPEG_REG(r, v);
}

#undef WRITE_MPEG_REG
#define WRITE_MPEG_REG(r, v) _hwdmx_write_reg(r, v)

#define DMX_READ_REG(i, r)\
			((i)?((i == 1)?READ_MPEG_REG(r##_2) :\
			READ_MPEG_REG(r##_3)) : READ_MPEG_REG(r))

#define DMX_WRITE_REG(i, r, d)\
			do {\
				if (i == 1) {\
					WRITE_MPEG_REG(r##_2, d);\
				} else if (i == 2) {\
					WRITE_MPEG_REG(r##_3, d);\
				} \
				else {\
					WRITE_MPEG_REG(r, d);\
				} \
			} while (0)

#define DEMUX_INT_MASK\
							((0<<(AUDIO_SPLICING_POINT))	|\
							(0<<(VIDEO_SPLICING_POINT)) 	|\
							(1<<(OTHER_PES_READY))			|\
							(1<<(PCR_READY))				|\
							(1<<(SUB_PES_READY))			|\
							(1<<(SECTION_BUFFER_READY)) 	|\
							(0<<(OM_CMD_READ_PENDING))		|\
							(1<<(TS_ERROR_PIN)) 			|\
							(1<<(NEW_PDTS_READY))			|\
							(0<<(DUPLICATED_PACKET))		|\
							(0<<(DIS_CONTINUITY_PACKET)))

static struct clk *aml_dvb_demux_clk;
static struct clk *aml_dvb_afifo_clk;
static struct clk *aml_dvb_ahbarb0_clk;
static struct clk *aml_dvb_uparsertop_clk;

static int demux_skipbyte = 0;
static int tsfile_clkdiv = 4;

static HWDMX_Demux Demux[DEMUX_COUNT];
static spinlock_t	slock;

static int cbus_base = 0x1800;
static int asyncfifo0_reg_base = 0x2800;
static int asyncfifo1_reg_base = 0x9800;
static int asyncfifo2_reg_base = 0x2400;
static int reset_base = 0x0400;
static int parser_sub_ptr_base = 0x3800;

MODULE_PARM_DESC(debug_dmx, "\n\t\t Enable demux debug information");
static int debug_dmx;
module_param(debug_dmx, int, 0644);


MODULE_PARM_DESC(use_of_sop, "\n\t\t Enable use of sop input");
static int use_of_sop;
module_param(use_of_sop, int, 0644);

long aml_stb_get_base(int id)
{
	switch (id) {
	case ID_STB_CBUS_BASE:
		return cbus_base;
	case ID_ASYNC_FIFO_REG_BASE:
		return asyncfifo0_reg_base;
	case ID_ASYNC_FIFO1_REG_BASE:
		return asyncfifo1_reg_base;
	case ID_ASYNC_FIFO2_REG_BASE:
		return asyncfifo2_reg_base;
	case ID_RESET_BASE:
		return reset_base;
	case ID_PARSER_SUB_START_PTR_BASE:
		return parser_sub_ptr_base;
	default:
		return 0;
	}
	return 0;
}

static int _hwdmx_init_clk(struct platform_device *pdev){

	aml_dvb_demux_clk =
		devm_clk_get(&pdev->dev, "demux");
	if (IS_ERR_OR_NULL(aml_dvb_demux_clk)) {
		dev_err(&pdev->dev, "get demux clk fail\n");
		return -1;
	}
	clk_prepare_enable(aml_dvb_demux_clk);

	aml_dvb_afifo_clk =
		devm_clk_get(&pdev->dev, "asyncfifo");
	if (!IS_ERR_OR_NULL(aml_dvb_afifo_clk)) {
		clk_prepare_enable(aml_dvb_afifo_clk);
	}

	aml_dvb_ahbarb0_clk =
		devm_clk_get(&pdev->dev, "ahbarb0");
	if (IS_ERR_OR_NULL(aml_dvb_ahbarb0_clk)) {
		dev_err(&pdev->dev, "get ahbarb0 clk fail\n");
		return -1;
	}
	clk_prepare_enable(aml_dvb_ahbarb0_clk);

	aml_dvb_uparsertop_clk =
		devm_clk_get(&pdev->dev, "uparsertop");
	if (IS_ERR_OR_NULL(aml_dvb_uparsertop_clk)) {
		dev_err(&pdev->dev, "get uparsertop clk fail\n");
		return -1;
	}
	clk_prepare_enable(aml_dvb_uparsertop_clk);
	return 0;
}
static int _hwdmx_release_clk(void) {

	clk_disable_unprepare(aml_dvb_uparsertop_clk);
	clk_disable_unprepare(aml_dvb_ahbarb0_clk);

	if (!IS_ERR_OR_NULL(aml_dvb_afifo_clk))
		clk_disable_unprepare(aml_dvb_afifo_clk);

	clk_disable_unprepare(aml_dvb_demux_clk);
	return 0;
}
static int _hwdmx_set_misc(HWDMX_Demux *pdmx, int hi_bsf, int en_dsc)
{
	if (!pdmx->init)
		return 0;

	if (hi_bsf >= 0) {
		DMX_WRITE_REG(pdmx->id, TS_HIU_CTL,
					hi_bsf ?
					(DMX_READ_REG(pdmx->id, TS_HIU_CTL) |
					(1 << USE_HI_BSF_INTERFACE))
					:
					(DMX_READ_REG(pdmx->id, TS_HIU_CTL) &
					(~(1 << USE_HI_BSF_INTERFACE))));
	}

	if (en_dsc >= 0) {
		DMX_WRITE_REG(pdmx->id, FEC_INPUT_CONTROL,
				en_dsc ?
				(DMX_READ_REG(pdmx->id, FEC_INPUT_CONTROL) |
				(1 << FEC_CORE_SEL))
				:
				(DMX_READ_REG(pdmx->id, FEC_INPUT_CONTROL) &
				(~(1 << FEC_CORE_SEL))));
	}

	return 0;
}
static int _hwdmx_enable(HWDMX_Demux *pdmx)
{
	int fec_sel = 0, hi_bsf=0, fec_ctrl = 0, record = 0;
	int fec_core_sel = 0;
	int set_stb = 0, fec_s = 0;
	int s2p_id;
	u32 invert0 = 0, invert1 = 0, invert2 = 0, fec_s0 = 0, fec_s1 = 0, fec_s2 = 0;
	u32 use_sop = 0;

	record = 1;
	if (use_of_sop == 1) {
		use_sop = 1;
		pr_inf("dmx use of sop input\r\n");
	}
	switch (pdmx->dmx_source) {
	case AM_TS_SRC_TS0:
		fec_sel = 0;
		fec_ctrl = pdmx->ts[0].control;
		record = record ? 1 : 0;
		break;
	case AM_TS_SRC_TS1:
		fec_sel = 1;
		fec_ctrl = pdmx->ts[1].control;
		record = record ? 1 : 0;
		break;
	case AM_TS_SRC_TS2:
		fec_sel = 2;
		fec_ctrl = pdmx->ts[2].control;
		record = record ? 1 : 0;
		break;
	case AM_TS_SRC_S_TS0:
	case AM_TS_SRC_S_TS1:
	case AM_TS_SRC_S_TS2:
		s2p_id = 0;
		fec_ctrl = 0;
		if (pdmx->dmx_source == AM_TS_SRC_S_TS0) {
			s2p_id = pdmx->ts[0].s2p_id;
			fec_ctrl = pdmx->ts[0].control;
		} else if (pdmx->dmx_source == AM_TS_SRC_S_TS1) {
			s2p_id = pdmx->ts[1].s2p_id;
			fec_ctrl = pdmx->ts[1].control;
		} else if (pdmx->dmx_source == AM_TS_SRC_S_TS2) {
			s2p_id = pdmx->ts[2].s2p_id;
			fec_ctrl = pdmx->ts[2].control;
		}
		//fec_sel = (s2p_id == 1) ? 5 : 6;
		fec_sel = 6 - s2p_id;
		record = record ? 1 : 0;
		set_stb = 1;
		fec_s = pdmx->dmx_source - AM_TS_SRC_S_TS0;
		break;
	case AM_TS_SRC_HIU:
		fec_sel = 7;
		fec_ctrl = 0;
		/*
			support record in HIU mode
		record = 0;
		*/
		break;
	default:
		fec_sel = 0;
		fec_ctrl = 0;
		record = 0;
		break;
	}

	if (pdmx->channel[0].used || pdmx->channel[1].used)
		hi_bsf = 1;
	else
		hi_bsf = 0;

	pr_dbg("[dmx-%d]src: %d, rec: %d, hi_bsf: %d, dsc: %d\n",
		   pdmx->id, pdmx->dmx_source, record, hi_bsf, fec_core_sel);

	if (pdmx->chan_count) {
		if (set_stb) {
			u32 v = READ_MPEG_REG(STB_TOP_CONFIG);
			int i;

			for (i = 0; i < pdmx->ts_in_total_count; i++) {
				if (pdmx->ts[i].s2p_id == 0)
					fec_s0 = i;
				else if (pdmx->ts[i].s2p_id == 1)
					fec_s1 = i;
				else if (pdmx->ts[i].s2p_id == 2)
					fec_s2 = i;
			}

			invert0 = pdmx->s2p[0].invert;
			invert1 = pdmx->s2p[1].invert;

			v &= ~((0x3 << S2P0_FEC_SERIAL_SEL) |
				   (0x1f << INVERT_S2P0_FEC_CLK) |
				   (0x3 << S2P1_FEC_SERIAL_SEL) |
				   (0x1f << INVERT_S2P1_FEC_CLK));

			v |= (fec_s0 << S2P0_FEC_SERIAL_SEL) |
				(invert0 << INVERT_S2P0_FEC_CLK) |
				(fec_s1 << S2P1_FEC_SERIAL_SEL) |
				(invert1 << INVERT_S2P1_FEC_CLK);
			WRITE_MPEG_REG(STB_TOP_CONFIG, v);

			if (pdmx->s2p_total_count >= 3) {
				invert2 = pdmx->s2p[2].invert;

			//add s2p2 config
			v = READ_MPEG_REG(STB_S2P2_CONFIG);
			v &= ~((0x3 << S2P2_FEC_SERIAL_SEL) |
				   (0x1f << INVERT_S2P2_FEC_CLK));
				v |= (fec_s2 << S2P2_FEC_SERIAL_SEL) |
				   (invert2 << INVERT_S2P2_FEC_CLK);
				WRITE_MPEG_REG(STB_S2P2_CONFIG, v);
			}
		}

		/*Initialize the registers */
		DMX_WRITE_REG(pdmx->id, STB_INT_MASK, DEMUX_INT_MASK);
		DMX_WRITE_REG(pdmx->id, DEMUX_MEM_REQ_EN,
#ifdef USE_AHB_MODE
				  (1 << SECTION_AHB_DMA_EN) |
				  (0 << SUB_AHB_DMA_EN) |
				  (1 << OTHER_PES_AHB_DMA_EN) |
#endif
				  (1 << SECTION_PACKET) |
				  (1 << VIDEO_PACKET) |
				  (1 << AUDIO_PACKET) |
				  (1 << SUB_PACKET) |
				  (1 << SCR_ONLY_PACKET) |
				(1 << OTHER_PES_PACKET));
		DMX_WRITE_REG(pdmx->id, PES_STRONG_SYNC, 0x1234);
		DMX_WRITE_REG(pdmx->id, DEMUX_ENDIAN,
				  (1<<SEPERATE_ENDIAN) |
				  (0<<OTHER_PES_ENDIAN) |
				  (7<<SCR_ENDIAN) |
				  (7<<SUB_ENDIAN) |
				  (7<<AUDIO_ENDIAN) |
				  (7<<VIDEO_ENDIAN) |
				  (7 << OTHER_ENDIAN) |
				  (7 << BYPASS_ENDIAN) | (0 << SECTION_ENDIAN));
		DMX_WRITE_REG(pdmx->id, TS_HIU_CTL,
				  (0 << LAST_BURST_THRESHOLD) |
				  (hi_bsf << USE_HI_BSF_INTERFACE));

		DMX_WRITE_REG(pdmx->id, FEC_INPUT_CONTROL,
				  (fec_core_sel << FEC_CORE_SEL) |
				  (fec_sel << FEC_SEL) | (fec_ctrl << 0));
		DMX_WRITE_REG(pdmx->id, STB_OM_CTL,
				  (0x40 << MAX_OM_DMA_COUNT) |
				  (0x7f << LAST_OM_ADDR));
		DMX_WRITE_REG(pdmx->id, DEMUX_CONTROL,
				  (0 << BYPASS_USE_RECODER_PATH) |
				  (0 << INSERT_AUDIO_PES_STRONG_SYNC) |
				  (0 << INSERT_VIDEO_PES_STRONG_SYNC) |
				  (0 << OTHER_INT_AT_PES_BEGINING) |
				  (0 << DISCARD_AV_PACKAGE) |
				  ((!!pdmx->dump_ts_select) << TS_RECORDER_SELECT) |
				  (record << TS_RECORDER_ENABLE) |
				  (1 << KEEP_DUPLICATE_PACKAGE) |
				  (1 << SECTION_END_WITH_TABLE_ID) |
				  (1 << ENABLE_FREE_CLK_FEC_DATA_VALID) |
				  (1 << ENABLE_FREE_CLK_STB_REG) |
				  (1 << STB_DEMUX_ENABLE) |
				  (use_sop << NOT_USE_OF_SOP_INPUT));
	} else {
		DMX_WRITE_REG(pdmx->id, STB_INT_MASK, 0);
		DMX_WRITE_REG(pdmx->id, FEC_INPUT_CONTROL, 0);
		DMX_WRITE_REG(pdmx->id, DEMUX_CONTROL, 0);
	}

	return 0;
}
/*Get the channel's target*/
static u32 _hwdmx_get_chan_target(HWDMX_Demux *pdmx, int cid)
{
	u32 type;

	if (!pdmx->channel[cid].used)
		return 0xFFFF;

	if (pdmx->channel[cid].type == DMX_TYPE_SEC) {
		type = SECTION_PACKET;
	} else {
		switch (pdmx->channel[cid].pes_type) {
		case DMX_PES_AUDIO:
			type = AUDIO_PACKET;
			break;
		case DMX_PES_VIDEO:
			type = VIDEO_PACKET;
			break;
		case DMX_PES_SUBTITLE:
		case DMX_PES_TELETEXT:
			type = SUB_PACKET;
			break;
		case DMX_PES_PCR:
			type = SCR_ONLY_PACKET;
			break;
		default:
			type = OTHER_PES_PACKET;
			break;
		}
	}

	pr_dbg("chan target: %x %x\n", type, pdmx->channel[cid].pid);
	return (type << PID_TYPE) | pdmx->channel[cid].pid;
}
/*Get the advance value of the channel*/
static inline u32 _hwdmx_get_chan_advance(HWDMX_Demux *pdmx, int cid)
{
	return 0;
}


/*Set the channel registers*/
static int _hwdmx_set_chan_regs(HWDMX_Demux *pdmx, int cid)
{
	u32 data, addr, advance, max;

	pr_dbg("set channel (id:%d PID:0x%x) registers\n", cid,
	       pdmx->channel[cid].pid);

	while (DMX_READ_REG(pdmx->id, FM_WR_ADDR) & 0x8000)
		udelay(1);

	if (cid & 1) {
		data =
		    (_hwdmx_get_chan_target(pdmx, cid - 1) << 16) |
		    _hwdmx_get_chan_target(pdmx, cid);
		advance =
		    (_hwdmx_get_chan_advance(pdmx, cid) << 8) |
		    _hwdmx_get_chan_advance(pdmx, cid - 1);
	} else {
		data =
		    (_hwdmx_get_chan_target(pdmx, cid) << 16) |
		    _hwdmx_get_chan_target(pdmx, cid + 1);
		advance =
		    (_hwdmx_get_chan_advance(pdmx, cid + 1) << 8) |
		    _hwdmx_get_chan_advance(pdmx, cid);
	}
	addr = cid >> 1;
	DMX_WRITE_REG(pdmx->id, FM_WR_DATA, data);
	DMX_WRITE_REG(pdmx->id, FM_WR_ADDR, (advance << 16) | 0x8000 | addr);

	pr_dbg("write fm %x:%x\n", (advance << 16) | 0x8000 | addr, data);

	for (max = CHANNEL_COUNT - 1; max > 0; max--) {
		if (pdmx->channel[max].used)
			break;
	}

	data = DMX_READ_REG(pdmx->id, MAX_FM_COMP_ADDR) & 0xF0;
	DMX_WRITE_REG(pdmx->id, MAX_FM_COMP_ADDR, data | (max >> 1));

	pr_dbg("write fm comp %x\n", data | (max >> 1));

	if (DMX_READ_REG(pdmx->id, OM_CMD_STATUS) & 0x8e00) {
		pr_error("error send cmd %x\n",
			 DMX_READ_REG(pdmx->id, OM_CMD_STATUS));
	}

	return 0;
}
/*Free a channel*/
void _hwdmx_free_chan(HWDMX_Demux *pdmx, int cid)
{
	pr_dbg("free channel(id:%d PID:0x%x)\n", cid, pdmx->channel[cid].pid);

	_hwdmx_set_chan_regs(pdmx, cid);
	set_debug_dmx_chanpids(pdmx->id, cid, -1);
	_hwdmx_enable(pdmx);
}
int _hwdmx_set_chan_pid(HWDMX_Demux *pdmx, int cid, int pid)
{
	_hwdmx_set_chan_regs(pdmx, cid);
	set_debug_dmx_chanpids(pdmx->id, cid, pid);
	_hwdmx_enable(pdmx);
	return 0;
}
static void _hwdmx_init_chan(HWDMX_Demux *pdmx) {

	memset(pdmx->channel,0,sizeof(pdmx->channel));
}
static int _hwdmx_enable_ts(HWDMX_Demux *pdmx)
{
	int out_src=0, des_in=0, en_des=0, fec_clk=0, hiu=0, dec_clk_en=0;
//	int src=0, tso_src=0, i=0;
	int src=0, i=0;
	u32 fec_s0=0, fec_s1=0,fec_s2=0;
	u32 invert0=0, invert1=0, invert2=0;
//	u32 data = 0;

	switch (pdmx->dmx_source) {
	case AM_TS_SRC_TS0:
		fec_clk = tsfile_clkdiv;
		hiu = 0;
		break;
	case AM_TS_SRC_TS1:
		fec_clk = tsfile_clkdiv;
		hiu = 0;
		break;
	case AM_TS_SRC_TS2:
		fec_clk = tsfile_clkdiv;
		hiu = 0;
		break;
	case AM_TS_SRC_TS3:
		fec_clk = tsfile_clkdiv;
		hiu = 0;
		break;
	case AM_TS_SRC_S_TS0:
		fec_clk = tsfile_clkdiv;
		hiu = 0;
		break;
	case AM_TS_SRC_S_TS1:
		fec_clk = tsfile_clkdiv;
		hiu = 0;
		break;
	case AM_TS_SRC_S_TS2:
		fec_clk = tsfile_clkdiv;
		hiu = 0;
		break;
	case AM_TS_SRC_HIU:
		fec_clk = tsfile_clkdiv;
		hiu = 1;
		break;
	default:
		fec_clk = 0;
		hiu = 0;
		break;
	}
#if 0
	switch (dvb->dsc[0].source) {
	case AM_TS_SRC_DMX0:
		des_in = 0;
		en_des = 1;
		dec_clk_en = 1;
		break;
	case AM_TS_SRC_DMX1:
		des_in = 1;
		en_des = 1;
		dec_clk_en = 1;
		break;
	case AM_TS_SRC_DMX2:
		des_in = 2;
		en_des = 1;
		dec_clk_en = 1;
		break;
	default:
		des_in = 0;
		en_des = 0;
		dec_clk_en = 0;
		break;
	}

	switch (dvb->tso_source) {
	case AM_TS_SRC_DMX0:
		tso_src = dvb->dmx[0].source;
		break;
	case AM_TS_SRC_DMX1:
		tso_src = dvb->dmx[1].source;
		break;
	case AM_TS_SRC_DMX2:
		tso_src = dvb->dmx[2].source;
		break;
	default:
		tso_src = dvb->tso_source;
		break;
	}

	switch (tso_src) {
	case AM_TS_SRC_TS0:
		out_src = 0;
		break;
	case AM_TS_SRC_TS1:
		out_src = 1;
		break;
	case AM_TS_SRC_TS2:
		out_src = 2;
		break;
	case AM_TS_SRC_TS3:
		out_src = 3;
		break;
	case AM_TS_SRC_S_TS0:
		out_src = 6;
		break;
	case AM_TS_SRC_S_TS1:
		out_src = 5;
		break;
	case AM_TS_SRC_S_TS2:
		out_src = 4;
		break;
	case AM_TS_SRC_HIU:
		out_src = 7;
		break;
	default:
		out_src = 0;
		break;
	}
#endif
	pr_dbg("[stb]src: %d, dsc1in: %d, tso: %d\n", src, des_in, out_src);

	fec_s0 = 0;
	fec_s1 = 0;
	fec_s2 = 0;
	invert0 = 0;
	invert1 = 0;
	invert2 = 0;

	for (i = 0; i < pdmx->ts_in_total_count; i++) {
		if (pdmx->ts[i].s2p_id == 0)
			fec_s0 = i;
		else if (pdmx->ts[i].s2p_id == 1)
			fec_s1 = i;
		else if (pdmx->ts[i].s2p_id == 2)
			fec_s2 = i;
	}

	invert0 = pdmx->s2p[0].invert;
	invert1 = pdmx->s2p[1].invert;

	WRITE_MPEG_REG(STB_TOP_CONFIG,
			   (invert1 << INVERT_S2P1_FEC_CLK) |
			   (fec_s1 << S2P1_FEC_SERIAL_SEL) |
			   (out_src << TS_OUTPUT_SOURCE) |
			   (des_in << DES_INPUT_SEL) |
			   (en_des << ENABLE_DES_PL) |
			   (dec_clk_en << ENABLE_DES_PL_CLK) |
			   (invert0 << INVERT_S2P0_FEC_CLK) |
			   (fec_s0 << S2P0_FEC_SERIAL_SEL));

	if (pdmx->s2p_total_count >= 3) {
		invert2 = pdmx->s2p[2].invert;

		WRITE_MPEG_REG(STB_S2P2_CONFIG,
			   (invert2 << INVERT_S2P2_FEC_CLK) |
			   (fec_s2 << S2P2_FEC_SERIAL_SEL));
	}

#if 0
	/* invert ts out clk,add ci model need add this*/
	if (dvb->ts_out_invert) {
		/*printk("ts out invert ---\r\n");*/
		data = READ_MPEG_REG(TS_TOP_CONFIG);
		data |= 1 << TS_OUT_CLK_INVERT;
		WRITE_MPEG_REG(TS_TOP_CONFIG, data);
	}
	/* invert ts out clk  end */
#endif
	WRITE_MPEG_REG(TS_FILE_CONFIG,
			   (demux_skipbyte << 16) |
			   (6 << DES_OUT_DLY) |
			   (3 << TRANSPORT_SCRAMBLING_CONTROL_ODD) |
			   (3 << TRANSPORT_SCRAMBLING_CONTROL_ODD_2) |
			   (hiu << TS_HIU_ENABLE) | (fec_clk << FEC_FILE_CLK_DIV));
	return 0;
}
static int _hwdmx_init(HWDMX_Demux *pdmx)
{
//	struct aml_dvb *dvb = (struct aml_dvb *)dmx->demux.priv;
	int id, times;
	u32 version, data;

	if (pdmx->init)
		return 0;

	pr_inf("demux init\n");

	WRITE_MPEG_REG(RESET1_REGISTER, RESET_DEMUXSTB);

	id = pdmx->id;
	times = 0;
	while (times++ < 1000000) {
		if (!(DMX_READ_REG(id, OM_CMD_STATUS) & 0x01))
			break;
	}

	WRITE_MPEG_REG(STB_TOP_CONFIG, 0);
	WRITE_MPEG_REG(STB_S2P2_CONFIG, 0);

	DMX_WRITE_REG(id, DEMUX_CONTROL, 0x0000);
	version = DMX_READ_REG(id, STB_VERSION);
	DMX_WRITE_REG(id, STB_TEST_REG, version);
	pr_dbg("STB %d hardware version : %d\n", id, version);
	DMX_WRITE_REG(id, STB_TEST_REG, 0x5550);
	data = DMX_READ_REG(id, STB_TEST_REG);
	if (data != 0x5550)
		pr_error("STB %d register access failed\n", id);
	DMX_WRITE_REG(id, STB_TEST_REG, 0xaaa0);
	data = DMX_READ_REG(id, STB_TEST_REG);
	if (data != 0xaaa0)
		pr_error("STB %d register access failed\n", id);
	DMX_WRITE_REG(id, MAX_FM_COMP_ADDR, 0x0000);
	DMX_WRITE_REG(id, STB_INT_MASK, 0);
	DMX_WRITE_REG(id, STB_INT_STATUS, 0xffff);
	DMX_WRITE_REG(id, FEC_INPUT_CONTROL, 0);

	pdmx->dump_ts_select = 0;

	//get ts
	pdmx->ts_in_total_count = getts(&pdmx->ts);
	pdmx->s2p_total_count = gets2p(&pdmx->s2p);

	pdmx->init = 1;

	return 0;
}
static void _hwdmx_get_base_addr(struct platform_device *pdev){
	char buf[32];
	u32 value;
	int ret = 0;

	memset(buf, 0, 32);
	snprintf(buf, sizeof(buf), "cbus_base");
	ret = of_property_read_u32(pdev->dev.of_node, buf, &value);
	if (!ret) {
		pr_inf("%s: 0x%x\n", buf, value);
		cbus_base = value;
	}

	memset(buf, 0, 32);
	snprintf(buf, sizeof(buf), "asyncfifo0_reg_base");
	ret = of_property_read_u32(pdev->dev.of_node, buf, &value);
	if (!ret) {
		pr_inf("%s: 0x%x\n", buf, value);
		asyncfifo0_reg_base = value;
	}
	memset(buf, 0, 32);
	snprintf(buf, sizeof(buf), "asyncfifo1_reg_base");
	ret = of_property_read_u32(pdev->dev.of_node, buf, &value);
	if (!ret) {
		pr_inf("%s: 0x%x\n", buf, value);
		asyncfifo1_reg_base = value;
	}
	memset(buf, 0, 32);
	snprintf(buf, sizeof(buf), "asyncfifo2_reg_base");
	ret = of_property_read_u32(pdev->dev.of_node, buf, &value);
	if (!ret) {
		pr_inf("%s: 0x%x\n", buf, value);
		asyncfifo2_reg_base = value;
	}
	memset(buf, 0, 32);
	snprintf(buf, sizeof(buf), "reset_base");
	ret = of_property_read_u32(pdev->dev.of_node, buf, &value);
	if (!ret) {
		pr_inf("%s: 0x%x\n", buf, value);
		reset_base = value;
	}
	memset(buf, 0, 32);
	snprintf(buf, sizeof(buf), "parser_sub_ptr_base");
	ret = of_property_read_u32(pdev->dev.of_node, buf, &value);
	if (!ret) {
		pr_inf("%s: 0x%x\n", buf, value);
		parser_sub_ptr_base = value;
	}
}

int hwdmx_set_source(HWDMX_Demux *pdmx, dmx_source_t src)
{
	unsigned long flags;
	int hw_src;
	int ret;

	ret = 0;

	spin_lock_irqsave(&slock, flags);

	if (pdmx->init == 0) {
		spin_unlock_irqrestore(&slock, flags);
		return -1;
	}

	hw_src = pdmx->dmx_source;

	switch (src) {
	case DMX_SOURCE_FRONT0:
		hw_src =
			(pdmx->ts[0].mode ==
			 AM_TS_SERIAL) ? (pdmx->ts[0].s2p_id + AM_TS_SRC_S_TS0) : AM_TS_SRC_TS0;
		break;
	case DMX_SOURCE_FRONT1:
		hw_src =
			(pdmx->ts[1].mode ==
			 AM_TS_SERIAL) ? (pdmx->ts[1].s2p_id + AM_TS_SRC_S_TS0) : AM_TS_SRC_TS1;
		break;
	case DMX_SOURCE_FRONT2:
		hw_src =
			(pdmx->ts[2].mode ==
			 AM_TS_SERIAL) ? (pdmx->ts[2].s2p_id + AM_TS_SRC_S_TS0) : AM_TS_SRC_TS2;
		break;
	case DMX_SOURCE_FRONT3:
		hw_src =
			(pdmx->ts[3].mode ==
			 AM_TS_SERIAL) ? (pdmx->ts[3].s2p_id + AM_TS_SRC_S_TS0) : AM_TS_SRC_TS3;
		break;
	case DMX_SOURCE_DVR0:
		hw_src = AM_TS_SRC_HIU;
		break;
	case DMX_SOURCE_FRONT0_OFFSET:
		hw_src = AM_TS_SRC_DMX0;
		break;
	case DMX_SOURCE_FRONT1_OFFSET:
		hw_src = AM_TS_SRC_DMX1;
		break;
	case DMX_SOURCE_FRONT2_OFFSET:
		hw_src = AM_TS_SRC_DMX2;
		break;
	default:
		pr_error("illegal demux source %d\n", src);
		ret = -EINVAL;
		break;
	}

	if (pdmx->dmx_source != hw_src) {
		int old_source = pdmx->dmx_source;

		pdmx->dmx_source = hw_src;

		if (IS_SRC_DMX(old_source)) {
			_hwdmx_set_misc(pdmx, 0, -1);
		} else {
			/*which dmx for av-play is unknown,
			 *can't avoid reset-all
			 */
//			dmx_reset_hw_ex(dvb, 0);
		}

		if (IS_SRC_DMX(pdmx->dmx_source)) {
			_hwdmx_set_misc(pdmx, 1, -1);
			/*dmx_reset_dmx_id_hw_ex_unlock
			 *	 (dvb, (dvb->stb_source-AM_TS_SRC_DMX0), 0);
			 */
		} else {
			/*which dmx for av-play is unknown,
			 *can't avoid reset-all
			 */
//			dmx_reset_hw_ex(dvb, 0);
		}
	}

	_hwdmx_enable_ts(pdmx);

	spin_unlock_irqrestore(&slock, flags);

	return ret;
}
int hwdmx_get_source(HWDMX_Demux *pdmx, dmx_source_t *src) {
	unsigned long flags;

	spin_lock_irqsave(&slock, flags);
	if (pdmx->init == 0) {
		spin_unlock_irqrestore(&slock, flags);
		return -1;
	}
	switch (pdmx->dmx_source) {
	case AM_TS_SRC_S_TS0:
	case AM_TS_SRC_TS0:
		*src = DMX_SOURCE_FRONT0;
		break;
	case AM_TS_SRC_TS1:
	case AM_TS_SRC_S_TS1:
		*src = DMX_SOURCE_FRONT1;
		break;
	case AM_TS_SRC_TS2:
	case AM_TS_SRC_S_TS2:
		*src = DMX_SOURCE_FRONT2;
		break;
	case AM_TS_SRC_HIU:
		*src = DMX_SOURCE_DVR0;
		break;
	case AM_TS_SRC_DMX0:
		*src = DMX_SOURCE_FRONT0_OFFSET;
		break;
	case AM_TS_SRC_DMX1:
		*src = DMX_SOURCE_FRONT1_OFFSET;
		break;
	case AM_TS_SRC_DMX2:
		*src = DMX_SOURCE_FRONT2_OFFSET;
		break;
	default:
		spin_unlock_irqrestore(&slock, flags);
		return -1;
	}

	spin_unlock_irqrestore(&slock, flags);
	return 0;
}
int hwdmx_probe(struct platform_device *pdev){
	char buf[32];
//	const char *str;
	u32 value;
	int i = 0;
	int ret = 0;
	struct resource *res;
	struct aml_dvb *advb = aml_get_dvb_device();

	_hwdmx_get_base_addr(pdev);

	if (_hwdmx_init_clk(pdev) != 0) {
		pr_error("hwdmx_probe init clk fail\n");
		return -1;
	}
	memset(buf, 0, 32);
	snprintf(buf, sizeof(buf), "ts_out_invert");
	ret = of_property_read_u32(pdev->dev.of_node, buf, &value);
	if (!ret) {
		pr_inf("%s: 0x%x\n", buf, value);
		advb->ts_out_invert = value;
	}
	for (i = 0; i < DEMUX_COUNT; i++) {
		Demux[i].dmx_irq = -1;
		snprintf(buf, sizeof(buf), "demux%d_irq", i);
		res = platform_get_resource_byname(pdev, IORESOURCE_IRQ, buf);
		if (res)
			Demux[i].dmx_irq = res->start;

		pr_error("%s irq num:%d \r\n", buf, Demux[i].dmx_irq);
	}
	frontend_probe(pdev);
	s2p_probe(pdev);
	asyncfifo_probe(pdev);
	slock = advb->slock;

	return 0;
}

int hwdmx_remove(void) {
	int i = 0;
	_hwdmx_release_clk();
	frontend_remove();
	s2p_remove();

	for (i = 0; i < dmx_count; i++) {
		asyncfifo_deinit(Demux[i].asyncfifo_id);
	}
	return 0;
}
HWDMX_Demux *hwdmx_create(int ts_id, int dmx_id, int asyncfifo_id){
	if (dmx_count >= DEMUX_COUNT)
		return NULL;

	Demux[dmx_count].ts_id = ts_id;
	Demux[dmx_count].asyncfifo_id = asyncfifo_id;
	Demux[dmx_count].dmx_id = dmx_id;
	Demux[dmx_count].cb = NULL;
	Demux[dmx_count].id = dmx_id;
	Demux[dmx_count].dsc_id = -1;

	_hwdmx_init_chan(&Demux[dmx_count]);

	_hwdmx_init(&Demux[dmx_count]);
	hwdmx_set_source(&Demux[dmx_count],ts_id);

	asyncfifo_init(asyncfifo_id,&Demux[dmx_count]);
	asyncfifo_set_source(asyncfifo_id, dmx_id);
	dmx_count++;

	return &Demux[dmx_count-1];
}

int hwdmx_set_dsc(HWDMX_Demux *pdmx,int dsc_id, int link) {

	int des_in = 0, en_des = 0, dec_clk_en = 0, des_out = 0;
	int dmx_source;
	int fec_core_sel = 1;
	u32 data;
	int ciplus_src = 0;
	int ciplus_out = 0;

	if (!pdmx || (dsc_id !=0 && dsc_id != 1))
		return -1;

	dmx_source = pdmx->dmx_id;

	switch (dmx_source) {
		case AM_DMX_0:
			des_in = 0;
			en_des = 1;
			dec_clk_en = 1;
			des_out = 1;

			ciplus_src = 0;
			ciplus_out = 1;
			break;
		case AM_DMX_1:
			des_in = 1;
			en_des = 1;
			dec_clk_en = 1;
			des_out = 2;

			ciplus_src = 1;
			ciplus_out = 2;

			break;
		case AM_DMX_2:
			des_in = 2;
			en_des = 1;
			dec_clk_en = 1;
			des_out = 4;

			ciplus_src = 2;
			ciplus_out = 4;

			break;
		default:
			des_in = 0;
			en_des = 0;
			dec_clk_en = 0;
			des_out = 0;

			ciplus_src = 0;
			ciplus_out = 0;

			break;
	}
	if (link == 0) {
		des_in = 0;
		en_des = 0;
		dec_clk_en = 0;
		fec_core_sel = 0;
	}
	if (dsc_id == 0) {
		data = READ_MPEG_REG(STB_TOP_CONFIG);
		data &= ~((3 << DES_INPUT_SEL) |
					(1 << ENABLE_DES_PL) |
					(1 << ENABLE_DES_PL_CLK));

		WRITE_MPEG_REG(STB_TOP_CONFIG, data |
			       (des_in << DES_INPUT_SEL) |
			       (en_des << ENABLE_DES_PL) |
			       (dec_clk_en << ENABLE_DES_PL_CLK));

		//for ciplus configure, i can't confirm.
		data = READ_MPEG_REG(STB_TOP_CONFIG);
		/* Set ciplus input source ,
		 * output set 0 means no output. ---> need confirm.
		 * if output set 0 still affects dsc output, we need to disable
		 * ciplus module.
		 */
		data &= ~(3<<CIPLUS_IN_SEL);
		WRITE_MPEG_REG(STB_TOP_CONFIG, data |
				(ciplus_src << CIPLUS_IN_SEL));

		data &= ~(7<<CIPLUS_OUT_SEL);
		WRITE_MPEG_REG(STB_TOP_CONFIG, data |
				(ciplus_out << CIPLUS_OUT_SEL));

	} else {
		WRITE_MPEG_REG(COMM_DESC_2_CTL,
				(6 << 8) |/*des_out_dly_2*/
				((!!en_des) << 6) |/* des_pl_clk_2*/
				((!!en_des) << 5) |/* des_pl_2*/
				(des_out << 2) |/*use_des_2*/
				(des_in)/*des_i_sel_2*/
				);
	}

	data = 0;

	data = DMX_READ_REG(pdmx->id, FEC_INPUT_CONTROL);

	data &= ~(0x1 << FEC_CORE_SEL);

	DMX_WRITE_REG(pdmx->id, FEC_INPUT_CONTROL, data |
		(fec_core_sel << FEC_CORE_SEL));

	if (link == 0)
		pdmx->dsc_id = -1;
	else
		pdmx->dsc_id = dsc_id;

	return 0;
}
int hwdmx_get_dsc(HWDMX_Demux *pdmx,int *dsc_id) {
	if (!pdmx || !dsc_id)
		return -1;
	*dsc_id = pdmx->dsc_id;
	return 0;
}
void hwdmx_destory(HWDMX_Demux *pdmx){
	return ;
}

int hwdmx_set_cb(HWDMX_Demux *pdmx, HWDMX_Cb cb,void *udata) {
	if (pdmx == NULL)
		return -1;

	pdmx->cb = cb;
	pdmx->udata = udata;
	asyncfifo_set_cb(pdmx->asyncfifo_id, cb, udata);
	return 0;
}
HWDMX_Chan *hwdmx_alloc_chan(HWDMX_Demux *pdmx) {
	int i = 0;

	for (i = 0; i < CHANNEL_COUNT; i++) {
		if (!pdmx->channel[i].used) {
			break;
		}
	}

	if (i == CHANNEL_COUNT) {
		return NULL;
	} else {
		pdmx->channel[i].pdmx = pdmx;
		pdmx->channel[i].used = 1;
		pdmx->channel[i].cid = i;
		pdmx->chan_count++;
	}
	return &pdmx->channel[i];
}
int hwdmx_free_chan(HWDMX_Chan *chan) {

	if (chan && chan->used) {
		chan->pdmx->chan_count--;
		chan->pid = 0x1fff;
		_hwdmx_free_chan(chan->pdmx, chan->cid);
		chan->used = 0;
	}
	return 0;
}
int hwdmx_set_pid(HWDMX_Chan *chan,int type, int pes_type, int pid) {
	if (chan && chan->used) {
		chan->pid = pid;
		chan->type = type;
		chan->pes_type = pes_type;
		_hwdmx_set_chan_pid(chan->pdmx, chan->cid, pid);
	}
	return 0;
}

