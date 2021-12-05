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
#include <asm/irq.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/pinctrl/consumer.h>
#include <linux/reset.h>
#include <linux/amlogic/media/utils/amstream.h>
#include <linux/amlogic/cpu_version.h>
#include <linux/clk.h>
#include <linux/dvb/ca.h>

#include "hw_demux/dvb_reg.h"
#include "aml_dsc.h"

#define pr_error(fmt, args...) printk("DVB: " fmt, ## args)
#define pr_inf(fmt, args...)   printk(KERN_DEBUG fmt, ## args)

#define DSC_CHANNEL_NUM 8

#define DSC_STATE_FREE      0
#define DSC_STATE_READY     3
#define DSC_STATE_GO        4

#define ENABLE_DEC_PL     7
#define ENABLE_DES_PL_CLK 15
#define CIPLUS_OUT_SEL    28
#define CIPLUS_IN_SEL     26

#define KEY_WR_AES_IV_B 5
#define KEY_WR_AES_IV_A 4
#define KEY_WR_AES_B    3
#define KEY_WR_AES_A    2
#define KEY_WR_DES_B    1
#define KEY_WR_DES_A    0

#define IDSA_MODE_BIT	31
#define SM4_MODE		30
#define CNTL_ENABLE     3
#define AES_CBC_DISABLE 2
#define AES_EN          1
#define DES_EN          0

#define AES_MSG_OUT_ENDIAN 24
#define AES_MSG_IN_ENDIAN  20
#define AES_KEY_ENDIAN  16
#define DES_MSG_OUT_ENDIAN 8
#define DES_MSG_IN_ENDIAN  4
#define DES_KEY_ENDIAN  0

#define ALGO_AES		0
#define ALGO_SM4		1
#define ALGO_DES		2

#define DSC_COUNT         8

#define CIPLUS_OUTPUT_AUTO 8
static int ciplus_out_sel = CIPLUS_OUTPUT_AUTO;
static int ciplus_out_auto_mode = 1;

#if 0
/*
 * param:
 * key:
 *	16bytes IV key
 * type:
 *	AM_DSC_KEY_TYPE_AES_ODD    IV odd key
 *	AM_DSC_KEY_TYPE_AES_EVEN  IV even key
 */
static void _ci_plus_set_iv(struct DescChannel *ch, enum ca_cw_type type,
			u8 *key)
{
	unsigned int k0, k1, k2, k3;

	k3 = (key[0] << 24) | (key[1] << 16) | (key[2] << 8) | key[3];
	k2 = (key[4] << 24) | (key[5] << 16) | (key[6] << 8) | key[7];
	k1 = (key[8] << 24) | (key[9] << 16) | (key[10] << 8) | key[11];
	k0 = (key[12] << 24) | (key[13] << 16) | (key[14] << 8) | key[15];

	if (type == CA_CW_AES_EVEN_IV ||
		type == CA_CW_SM4_EVEN_IV) {
		WRITE_MPEG_REG(CIPLUS_KEY0, k0);
		WRITE_MPEG_REG(CIPLUS_KEY1, k1);
		WRITE_MPEG_REG(CIPLUS_KEY2, k2);
		WRITE_MPEG_REG(CIPLUS_KEY3, k3);
		WRITE_MPEG_REG(CIPLUS_KEY_WR,
			(ch->id << 9) | (1<<KEY_WR_AES_IV_A));
	} else if (type == CA_CW_AES_ODD_IV ||
			   type == CA_CW_SM4_ODD_IV) {
		WRITE_MPEG_REG(CIPLUS_KEY0, k0);
		WRITE_MPEG_REG(CIPLUS_KEY1, k1);
		WRITE_MPEG_REG(CIPLUS_KEY2, k2);
		WRITE_MPEG_REG(CIPLUS_KEY3, k3);
		WRITE_MPEG_REG(CIPLUS_KEY_WR,
			(ch->id << 9) | (1<<KEY_WR_AES_IV_B));
	}
}
#endif
/*
 * Param:
 * key_endian
 *	S905D  7 for kl    0 for set key directly
 * mode
 *  0 for ebc
 *  1 for cbc
 */
static void _ci_plus_config(int key_endian, int mode, int algo)
{
	unsigned int data;
	unsigned int idsa_mode = 0;
	unsigned int sm4_mode = 0;
	unsigned int cbc_disable = 0;
	unsigned int des_enable = 0;
	unsigned int aes_enable = 1;

	pr_error("%s mode:%d,alog:%d\n",__FUNCTION__,mode,algo);

	if (get_cpu_type() < MESON_CPU_MAJOR_ID_SM1) {
		WRITE_MPEG_REG(CIPLUS_ENDIAN,
				(15 << AES_MSG_OUT_ENDIAN)
				| (15 << AES_MSG_IN_ENDIAN)
				| (key_endian << AES_KEY_ENDIAN)
				|
				(15 << DES_MSG_OUT_ENDIAN)
				| (15 << DES_MSG_IN_ENDIAN)
				| (key_endian << DES_KEY_ENDIAN)
				);
	} else {
		WRITE_MPEG_REG(CIPLUS_ENDIAN, 0);
	}

	data = READ_MPEG_REG(CIPLUS_ENDIAN);

	if (algo == ALGO_SM4) {
		sm4_mode = 1;
	} else if (algo ==  ALGO_AES){
		sm4_mode = 0;
	} else {
		sm4_mode = 0;
		des_enable = 1;
	}

	if (mode == IDSA_MODE) {
		idsa_mode = 1;
		cbc_disable = 0;
	} else if (mode == CBC_MODE) {
		cbc_disable = 0;
	} else {
		cbc_disable = 1;
	}
	pr_error("idsa_mode:%d sm4_mode:%d cbc_disable:%d aes_enable:%d des_enable:%d\n", \
		idsa_mode,sm4_mode,cbc_disable,aes_enable,des_enable);

	data =  (idsa_mode << IDSA_MODE_BIT) |
			(sm4_mode << SM4_MODE ) |
			(cbc_disable << AES_CBC_DISABLE) |
			/*1 << AES_CBC_DISABLE     : ECB
			 *0 << AES_CBC_DISABLE     : CBC
			 */
			(1 << CNTL_ENABLE) |
			(aes_enable << AES_EN) |
			(des_enable << DES_EN);

	WRITE_MPEG_REG(CIPLUS_CONFIG, data);
	data = READ_MPEG_REG(CIPLUS_CONFIG);
	pr_error("CIPLUS_CONFIG is 0x%x\n",data);
}


/*
 * Set output to demux set.
 */
static void _ci_plus_set_output(struct DescChannel *ch)
{
	struct aml_dsc *dsc = ch->dsc;
	u32 data;
	u32 in = 0, out = 0;
	int set = 0;

	if (dsc->id != 0) {
		pr_error("Ciplus set output can only work at dsc0 device\n");
		return;
	}

	switch (dsc->source) {
	case  AM_TS_SRC_DMX0:
		in = 0;
		break;
	case  AM_TS_SRC_DMX1:
		in = 1;
		break;
	case  AM_TS_SRC_DMX2:
		in = 2;
		break;
	default:
		break;
	}

	if (ciplus_out_auto_mode == 1) {
		switch (dsc->dst) {
		case  AM_TS_SRC_DMX0:
			out = 1;
			break;
		case  AM_TS_SRC_DMX1:
			out = 2;
			break;
		case  AM_TS_SRC_DMX2:
			out = 4;
			break;
		default:
			break;
		}
		set = 1;
		ciplus_out_sel = out;
	} else if (ciplus_out_sel >= 0 && ciplus_out_sel <= 7) {
		set = 1;
		out = ciplus_out_sel;
	} else {
		pr_error("dsc ciplus out config is invalid\n");
	}

	if (set) {
		/* Set ciplus input source ,
		 * output set 0 means no output. ---> need confirm.
		 * if output set 0 still affects dsc output, we need to disable
		 * ciplus module.
		 */
		data = READ_MPEG_REG(STB_TOP_CONFIG);
		data &= ~(3<<CIPLUS_IN_SEL);
		data |= in << CIPLUS_IN_SEL;
		data &= ~(7<<CIPLUS_OUT_SEL);
		data |= out << CIPLUS_OUT_SEL;
		WRITE_MPEG_REG(STB_TOP_CONFIG, data);
		pr_inf("dsc ciplus in[%x] out[%x] %s\n", in, out,
			(ciplus_out_auto_mode) ? "" : "force");
	}

}
#if 0
/*
 * Ciplus output has high priority,
 * disable it's output will let dsc output go.
 */
static void _ci_plus_disable_output(void)
{
	u32 data = 0;

	data = READ_MPEG_REG(STB_TOP_CONFIG);
	WRITE_MPEG_REG(STB_TOP_CONFIG, data &
			~(7 << CIPLUS_OUT_SEL));
}

static void _ci_plus_enable(void)
{
	u32 data = 0;

	data = READ_MPEG_REG(STB_TOP_CONFIG);
	WRITE_MPEG_REG(CIPLUS_CONFIG,
			(1 << CNTL_ENABLE)
			| (1 << AES_EN)
			| (1 << DES_EN));
}
#endif
static void _ci_plus_disable(void)
{
	u32 data = 0;

	WRITE_MPEG_REG(CIPLUS_CONFIG, 0);

	data = READ_MPEG_REG(STB_TOP_CONFIG);
	WRITE_MPEG_REG(STB_TOP_CONFIG, data &
			~((1 << CIPLUS_IN_SEL) | (7 << CIPLUS_OUT_SEL)));
/*	WRITE_MPEG_REG(CIPLUS_CONFIG,
 *			(0 << CNTL_ENABLE) | (0 << AES_EN));
 */
}

static int _hwdsc_ch_set_pid(struct DescChannel *ch, int pid)
{
	struct aml_dsc *dsc = ch->dsc;
	int is_dsc2 = (dsc->id == 1) ? 1 : 0;
	u32 data;

	WRITE_MPEG_REG(TS_PL_PID_INDEX,
			((ch->id & 0x0f) >> 1)+(is_dsc2 ? 4 : 0));
	data = READ_MPEG_REG(TS_PL_PID_DATA);
	if (ch->id & 1) {
		data &= 0xFFFF0000;
		data |= pid & 0x1fff;
		if (ch->state == DSC_STATE_FREE)
			data |= 1 << PID_MATCH_DISABLE_LOW;
	} else {
		data &= 0xFFFF;
		data |= (pid & 0x1fff) << 16;
		if (ch->state == DSC_STATE_FREE)
			data |= 1 << PID_MATCH_DISABLE_HIGH;
	}
	WRITE_MPEG_REG(TS_PL_PID_INDEX,
			((ch->id & 0x0f) >> 1)+(is_dsc2 ? 4 : 0));
	WRITE_MPEG_REG(TS_PL_PID_DATA, data);
	WRITE_MPEG_REG(TS_PL_PID_INDEX, 0);

	if (ch->state != DSC_STATE_FREE)
		pr_inf("set DSC %d ch %d PID %d\n", dsc->id, ch->id, pid);
	else
		pr_inf("disable DSC %d ch %d\n", dsc->id, ch->id);
	return 0;
}
static int _hwdsc_set_aes_des_sm4_key(struct DescChannel *ch, int flags,
			enum ca_cw_type type, u8 *key)
{
	unsigned int k0, k1, k2, k3;
	int iv = 0, aes = 0, des = 0;
	int ab_iv = 0, ab_aes = 0, ab_des = 0;
	int from_kl = flags & CA_CW_FROM_KL;
	int algo = 0;

	if (!from_kl) {
		if (get_cpu_type() < MESON_CPU_MAJOR_ID_SM1) {
		k3 = (key[0] << 24) | (key[1] << 16) | (key[2] << 8) | key[3];
		k2 = (key[4] << 24) | (key[5] << 16) | (key[6] << 8) | key[7];
		k1 = (key[8] << 24) | (key[9] << 16) | (key[10] << 8) | key[11];
		k0 = (key[12] << 24) | (key[13] << 16)
			| (key[14] << 8) | key[15];
		} else {
		k0 = (key[0]) | (key[1] << 8) | (key[2] << 16) | (key[3] << 24);
		k1 = (key[4]) | (key[5] << 8) | (key[6] << 16) | (key[7] << 24);
		k2 = (key[8]) | (key[9] << 8) | (key[10] << 16)| (key[11] << 24);
		k3 = (key[12])| (key[13] << 8)| (key[14] << 16)| (key[15] << 24);
		}
	} else
		k0 = k1 = k2 = k3 = 0;

	switch (type) {
	case CA_CW_AES_EVEN:
	case CA_CW_SM4_EVEN:
		ab_aes = (from_kl) ? 0x2 : 0x1;
		if (ch->mode == -1)
			ch->mode = ECB_MODE;
		aes = 1;
		if (type == CA_CW_AES_EVEN)
			algo = ALGO_AES;
		else
			algo = ALGO_SM4;
		break;
	case CA_CW_AES_ODD:
	case CA_CW_SM4_ODD:
		ab_aes = (from_kl) ? 0x1 : 0x2;
		if (ch->mode == -1)
			ch->mode = ECB_MODE;
		aes = 1;
		if (type == CA_CW_AES_ODD)
			algo = ALGO_AES;
		else
			algo = ALGO_SM4;
		break;
	case CA_CW_AES_EVEN_IV:
	case CA_CW_SM4_EVEN_IV:
		ab_iv = 0x1;
		if (ch->mode == -1)
			ch->mode = CBC_MODE;
		iv = 1;
		if (type == CA_CW_AES_EVEN_IV)
			algo = ALGO_AES;
		else
			algo = ALGO_SM4;
		break;
	case CA_CW_AES_ODD_IV:
	case CA_CW_SM4_ODD_IV:
		ab_iv = 0x2;
		if (ch->mode == -1)
			ch->mode = CBC_MODE;
		iv = 1;
		if (type == CA_CW_AES_ODD_IV)
			algo = ALGO_AES;
		else
			algo = ALGO_SM4;
		break;
	case CA_CW_DES_EVEN:
		ab_des = 0x1;
		ch->mode = ECB_MODE;
		des = 1;
		algo = ALGO_DES;
		break;
	case CA_CW_DES_ODD:
		ab_des = 0x2;
		ch->mode = ECB_MODE;
		algo = ALGO_DES;
		des = 1;
		break;
	default:
		break;
	}

	/* Set endian and cbc/ecb mode */
	if (from_kl)
		_ci_plus_config(7, ch->mode, algo);
	else
		_ci_plus_config(0, ch->mode, algo);

	/* Write keys to work */
	if (iv || aes) {
		WRITE_MPEG_REG(CIPLUS_KEY0, k0);
		WRITE_MPEG_REG(CIPLUS_KEY1, k1);
		WRITE_MPEG_REG(CIPLUS_KEY2, k2);
		WRITE_MPEG_REG(CIPLUS_KEY3, k3);
	} else {/*des*/
		WRITE_MPEG_REG(CIPLUS_KEY0, k2);
		WRITE_MPEG_REG(CIPLUS_KEY1, k3);
		WRITE_MPEG_REG(CIPLUS_KEY2, 0);
		WRITE_MPEG_REG(CIPLUS_KEY3, 0);
	}
	WRITE_MPEG_REG(CIPLUS_KEY_WR,
		(ch->id << 9) |
				/* bit[11:9] the key of index,
					need match PID index*/
		((from_kl && des) ? (1 << 8) : 0) |
				/* bit[8] des key use cw[127:64]*/
		(0 << 7) |		/* bit[7] aes iv use cw*/
		((from_kl && (aes || des)) ? (1 << 6) : 0) |
				/* bit[6] aes/des key use cw*/
				/* bit[5] write AES IV B value*/
		(ab_iv << 4) |	/* bit[4] write AES IV A value*/
				/* bit[3] write AES B key*/
		(ab_aes << 2) | /* bit[2] write AES A key*/
				/* bit[1] write DES B key*/
		(ab_des));		/* bit[0] write DES A key*/

	/*
	pr_inf("k:%08x:%08x:%08x:%08x kl:%d aes:%d des:%d ab_iv:%d ab_aes:%d ab_des:%d id:%d mod:%d\n",
		k0, k1, k2, k3,
		from_kl, aes, des, ab_iv, ab_aes, ab_des, ch->id, ch->aes_mode);
	*/
	return 0;
}

static int _hwdsc_set_csa_key(struct DescChannel *ch, int flags,
			enum ca_cw_type type, u8 *key)
{
	struct aml_dsc *dsc = ch->dsc;
	int is_dsc2 = (dsc->id == 1) ? 1 : 0;
	u16 k0, k1, k2, k3;
	u32 key0, key1;
	int reg; /*not sure if reg readable*/
/*	u32 data;
 *	u32 pid = 0x1fff;
 */
	int from_kl = flags & CA_CW_FROM_KL;
/*  int pid = ch->pid; */

	if (from_kl) {
		k0 = k1 = k2 = k3 = 0;

		/*dummy write to check if kl not working*/
		key0 = key1 = 0;
		WRITE_MPEG_REG(COMM_DESC_KEY0, key0);
		WRITE_MPEG_REG(COMM_DESC_KEY1, key1);

	/*tdes? :*/
		if (get_cpu_type() == MESON_CPU_MAJOR_ID_GXBB) {
			WRITE_MPEG_REG(COMM_DESC_KEY_RW,
/*				(type ? (1 << 6) : (1 << 5)) | */
				((1 << 5)) |
				((ch->id + type * DSC_COUNT)+
					(is_dsc2 ? 16 : 0)));
		}
		if (get_cpu_type() == MESON_CPU_MAJOR_ID_GXL ||
			get_cpu_type() == MESON_CPU_MAJOR_ID_GXM) {
			pr_info("do kl..\n");
			WRITE_MPEG_REG(COMM_DESC_KEY_RW,
				(type ? (1 << 6) : (1 << 5)) | (1<<7) |
				((ch->id + type * DSC_COUNT)+
				 (is_dsc2 ? 16 : 0)));
		}
		reg = (type ? (1 << 6) : (1 << 5)) |
				((ch->id + type * DSC_COUNT)+
				 (is_dsc2 ? 16 : 0));
	} else {
		k0 = (key[0] << 8) | key[1];
		k1 = (key[2] << 8) | key[3];
		k2 = (key[4] << 8) | key[5];
		k3 = (key[6] << 8) | key[7];

		key0 = (k0 << 16) | k1;
		key1 = (k2 << 16) | k3;
		WRITE_MPEG_REG(COMM_DESC_KEY0, key0);
		WRITE_MPEG_REG(COMM_DESC_KEY1, key1);
		reg = (ch->id + type * DSC_COUNT)+(is_dsc2 ? 16 : 0);
		WRITE_MPEG_REG(COMM_DESC_KEY_RW,reg);
	}

	return 0;
}
static int _get_dsc_key_work_mode(enum ca_cw_type cw_type)
{
	int work_mode = DVBCSA_MODE;

	switch (cw_type) {
	case CA_CW_DVB_CSA_EVEN:
	case CA_CW_DVB_CSA_ODD:
		work_mode = DVBCSA_MODE;
		break;
	case CA_CW_AES_EVEN:
	case CA_CW_AES_ODD:
	case CA_CW_AES_ODD_IV:
	case CA_CW_AES_EVEN_IV:
	case CA_CW_DES_EVEN:
	case CA_CW_DES_ODD:
	case CA_CW_SM4_EVEN:
	case CA_CW_SM4_ODD:
	case CA_CW_SM4_ODD_IV:
	case CA_CW_SM4_EVEN_IV:
		work_mode = CIPLUS_MODE;
	default:
		break;
	}
	return work_mode;
}

/* Check if there are channels run in previous mode(aes/dvbcsa)
 * in dsc0/ciplus
 */
static void _dsc_ciplus_switch_check(struct DescChannel *ch,
			enum ca_cw_type cw_type)
{
	struct aml_dsc *dsc = ch->dsc;
	int work_mode = 0;
	struct DescChannel *pch = NULL;
	int i;

	work_mode = _get_dsc_key_work_mode(cw_type);
	if (dsc->work_mode == work_mode)
		return;

	dsc->work_mode = work_mode;

	for (i = 0; i < dsc->channel_num; i++) {
		pch = &dsc->channels[i];
		if (pch->work_mode != work_mode && pch->work_mode != -1) {
			pr_error("Dsc work mode changed,");
			pr_error("but there are still some channels");
			pr_error("run in different mode\n");
		}
	}
}

static void _dsc_channel_alloc(struct aml_dsc *dsc, int id, unsigned int pid)
{
	struct DescChannel *ch = &dsc->channels[id];

	ch->state  = DSC_STATE_READY;
	ch->work_mode = -1;
	ch->id    = id;
	ch->pid   = pid;
	ch->set   = 0;
	ch->mode = -1;

	if (dsc->mode == SW_DSC_MODE) {
		ch->chan = swdmx_descrambler_alloc_channel(dsc->swdsc);
		if (!ch->chan) {
			pr_error("swdmx_descrambler_alloc_channel fail\n");
			return ;
		}
		if (swdmx_desc_channel_set_pid(ch->chan, ch->pid) != SWDMX_OK)
			pr_error("swdmx_desc_channel_set_pid fail\n");
	} else {
		_hwdsc_ch_set_pid(ch, pid);
	}
}
static void _dsc_channel_free(struct DescChannel *ch)
{
	struct aml_dsc *dsc = (struct aml_dsc *)ch->dsc;

	if (ch->state == DSC_STATE_FREE)
		return;

	ch->state = DSC_STATE_FREE;

	if (dsc->mode == SW_DSC_MODE) {
		if (ch->chan) {
			swdmx_desc_channel_disable(ch->chan);
			/* this will free algo */
			swdmx_desc_channel_free(ch->chan);
			ch->chan = NULL;
			ch->algo = NULL;
		}
	} else {
		_hwdsc_ch_set_pid(ch, 0x1fff);
		_ci_plus_disable();
	}

	ch->pid   = 0x1fff;
	ch->set = 0;
	ch->work_mode = -1;
	ch->mode  = -1;
}

static void _dsc_reset(struct aml_dsc *dsc)
{
	int i;

	for (i = 0; i < dsc->channel_num; i++)
		_dsc_channel_free(&dsc->channels[i]);
}
static int _dsc_set_key(struct DescChannel *ch, int flags, enum ca_cw_type type,
			u8 *key)
{
	int ret = 0;
	struct aml_dsc *dsc = (struct aml_dsc *)ch->dsc;

	if (ch->dsc == NULL)
		return -1;

	switch (type)
	{
		case CA_CW_DVB_CSA_EVEN:
		case CA_CW_DVB_CSA_ODD:
		{
			if (dsc->mode == SW_DSC_MODE) {
				if (!ch->algo) {
					ch->algo = swdmx_dvbcsa2_algo_new();
					if (!ch->algo) {
						pr_error("swdmx_dvbcsa2_algo_new fail\n");
						return -1;
					}
					swdmx_desc_channel_set_algo(ch->chan, ch->algo);
				}
				if (type == CA_CW_DVB_CSA_EVEN)
					swdmx_desc_channel_set_param(ch->chan,SWDMX_DVBCSA2_PARAM_EVEN_KEY,key);
				else
					swdmx_desc_channel_set_param(ch->chan,SWDMX_DVBCSA2_PARAM_ODD_KEY,key);

				if (ch->state == DSC_STATE_READY)
				{
					swdmx_desc_channel_enable(ch->chan);
					ch->state = DSC_STATE_GO;
				}
			} else {
				_ci_plus_disable();
				ret = _hwdsc_set_csa_key(ch, flags, type, key);
				if (ret != 0)
					return ret;
				/* Different with old mode, do change */
				if (ch->work_mode == CIPLUS_MODE || ch->work_mode == -1) {
					if (ch->work_mode == -1)
						pr_error("Dsc set output and enable\n");
					else
						pr_error("Dsc set output change from ciplus\n");
					ch->mode = ECB_MODE;
					/*aml_ci_plus_disable();*/
					ch->work_mode = DVBCSA_MODE;
				}
			}
			break;
		}
		case CA_CW_AES_EVEN:
		case CA_CW_AES_ODD:
		case CA_CW_AES_EVEN_IV:
		case CA_CW_AES_ODD_IV:
		case CA_CW_DES_EVEN:
		case CA_CW_DES_ODD:
		case CA_CW_SM4_EVEN:
		case CA_CW_SM4_ODD:
		case CA_CW_SM4_EVEN_IV:
		case CA_CW_SM4_ODD_IV:
		{
			if (dsc->mode == SW_DSC_MODE) {
				if (!ch->algo) {
					ch->algo = swdmx_aes_cbc_algo_new();
					if (!ch->algo) {
						pr_error("swdmx_dvbcsa2_algo_new fail\n");
						return -1;
					}
					swdmx_desc_channel_set_algo(ch->chan, ch->algo);
					swdmx_desc_channel_set_param(ch->chan, SWDMX_AES_CBC_PARAM_ALIGN, SWDMX_DESC_ALIGN_HEAD);
				}
				if (type == CA_CW_AES_EVEN) {
					swdmx_desc_channel_set_param(ch->chan, SWDMX_AES_CBC_PARAM_EVEN_KEY, key);
				}else if (type == CA_CW_AES_ODD) {
					swdmx_desc_channel_set_param(ch->chan, SWDMX_AES_CBC_PARAM_ODD_KEY, key);
				}else if (type == CA_CW_AES_EVEN_IV) {
					swdmx_desc_channel_set_param(ch->chan, SWDMX_AES_CBC_PARAM_EVEN_IV, key);
				}else{
					swdmx_desc_channel_set_param(ch->chan, SWDMX_AES_CBC_PARAM_ODD_IV, key);
				}
				if (ch->state == DSC_STATE_READY)
				{
					swdmx_desc_channel_enable(ch->chan);
					ch->state = DSC_STATE_GO;
				}
			}else {
				_ci_plus_set_output(ch);
				ret = _hwdsc_set_aes_des_sm4_key(ch, flags, type, key);
				if (ret != 0)
					return ret;
				/* Different with old mode, do change */
				if (ch->work_mode == DVBCSA_MODE || ch->work_mode == -1) {
					if (ch->work_mode == -1)
						pr_error("Ciplus set output and enable\n");
					else
						pr_error("Ciplus set output change from dsc\n");
					ch->work_mode = CIPLUS_MODE;
				}

			}
			break;
		}
		default:
			break;
	}
	return 0;
}
static int _dsc_set_cw(struct aml_dsc *dsc, struct ca_descr_ex *d)
{
	struct DescChannel *ch;

	if (d->index >= DSC_CHANNEL_NUM)
		return -EINVAL;

	ch = &dsc->channels[d->index];

	switch (d->type) {
	case CA_CW_DVB_CSA_EVEN:
	case CA_CW_AES_EVEN:
	case CA_CW_DES_EVEN:
	case CA_CW_SM4_EVEN:
		memcpy(ch->even, d->cw, DSC_KEY_SIZE_MAX);
		break;
	case CA_CW_DVB_CSA_ODD:
	case CA_CW_AES_ODD:
	case CA_CW_DES_ODD:
	case CA_CW_SM4_ODD:
		memcpy(ch->odd, d->cw, DSC_KEY_SIZE_MAX);
		break;
	case CA_CW_AES_EVEN_IV:
	case CA_CW_SM4_EVEN_IV:
		memcpy(ch->even_iv, d->cw, DSC_KEY_SIZE_MAX);
		break;
	case CA_CW_AES_ODD_IV:
	case CA_CW_SM4_ODD_IV:
		memcpy(ch->odd_iv, d->cw, DSC_KEY_SIZE_MAX);
		break;
	default:
		break;
	}

	ch->set |= (1 << d->type) | (d->flags << 24);

	if (d->mode == CA_DSC_IDSA) {
		ch->mode = IDSA_MODE;
	}

	_dsc_set_key(ch, d->flags, d->type, d->cw);
	_dsc_ciplus_switch_check(ch, d->type);

	return 0;
}
static int _dvb_dsc_open(struct inode *inode, struct file *file)
{
	int err;

	err = dvb_generic_open(inode, file);
	if (err < 0)
		return err;

	return 0;
}
static int _dsc_set_pid(struct aml_dsc *dsc, ca_pid_t *pi) {
	int i;
	struct DescChannel *ch;
	int ret = 0;

	if (pi->index == -1) {
		for (i = 0; i < dsc->channel_num; i++) {
			ch = &dsc->channels[i];

			if (ch->state != DSC_STATE_FREE && (ch->pid == pi->pid)) {
				_dsc_channel_free(ch);
				break;
			}
		}
	} else if ((pi->index >= 0) && (pi->index < DSC_CHANNEL_NUM)) {
		ch = &dsc->channels[pi->index];

		if (pi->pid < 0x1fff) {
			if (ch->state != DSC_STATE_FREE) {
				_dsc_channel_free(ch);
			}
			_dsc_channel_alloc(dsc,
			pi->index, pi->pid);

		} else {
			if (ch->state != DSC_STATE_FREE)
				_dsc_channel_free(ch);
		}
	} else {
		ret = -EINVAL;
	}
	return ret;
}

static int _dvb_dsc_do_ioctl(struct file *file, unsigned int cmd,
			  void *parg)
{
	struct dvb_device *dvbdev = file->private_data;
	struct aml_dsc *dsc = dvbdev->priv;
	int ret = 0;

	if (mutex_lock_interruptible(&dsc->mutex))
		return -ERESTARTSYS;

	switch (cmd) {
	case CA_RESET:
		_dsc_reset(dsc);
		break;
	case CA_GET_CAP: {
		ca_caps_t *cap = parg;

		cap->slot_num   = 1;
		cap->slot_type  = CA_DESCR;
		cap->descr_num  = DSC_CHANNEL_NUM;
		cap->descr_type = 0;
		break;
	}
	case CA_GET_SLOT_INFO: {
		ca_slot_info_t *slot = parg;

		slot->num   = 1;
		slot->type  = CA_DESCR;
		slot->flags = 0;
		break;
	}
	case CA_GET_DESCR_INFO: {
		ca_descr_info_t *descr = parg;

		descr->num  = DSC_CHANNEL_NUM;
		descr->type = 0;
		break;
	}
	case CA_SET_DESCR: {
		ca_descr_t    *d = parg;
		struct ca_descr_ex  dex;

		dex.index = d->index;
		dex.type  = d->parity ? CA_CW_DVB_CSA_ODD : CA_CW_DVB_CSA_EVEN;
		dex.flags = 0;
		memcpy(dex.cw, d->cw, sizeof(d->cw));

		ret = _dsc_set_cw(dsc, &dex);
		break;
	}
	case CA_SET_PID: {
		ca_pid_t *pi = parg;

		ret = _dsc_set_pid(dsc,pi);

		break;
	}
	case CA_SET_DESCR_EX: {
		struct ca_descr_ex *d = parg;

		ret = _dsc_set_cw(dsc, d);
		break;
	}
	default:
		ret = -EINVAL;
		break;
	}

	mutex_unlock(&dsc->mutex);

	return ret;
}

static int _dvb_dsc_usercopy(struct file *file,
		     unsigned int cmd, unsigned long arg,
		     int (*func)(struct file *file,
		     unsigned int cmd, void *arg))
{
	char    sbuf[128];
	void    *mbuf = NULL;
	void    *parg = NULL;
	int     err  = -EINVAL;

	/*  Copy arguments into temp kernel buffer  */
	switch (_IOC_DIR(cmd)) {
	case _IOC_NONE:
		/*
		 * For this command, the pointer is actually an integer
		 * argument.
		 */
		parg = (void *) arg;
		break;
	case _IOC_READ: /* some v4l ioctls are marked wrong ... */
	case _IOC_WRITE:
	case (_IOC_WRITE | _IOC_READ):
		if (_IOC_SIZE(cmd) <= sizeof(sbuf)) {
			parg = sbuf;
		} else {
			/* too big to allocate from stack */
			mbuf = kmalloc(_IOC_SIZE(cmd), GFP_KERNEL);
			if (mbuf == NULL)
				return -ENOMEM;
			parg = mbuf;
		}

		err = -EFAULT;
		if (copy_from_user(parg, (void __user *)arg, _IOC_SIZE(cmd)))
			goto out;
		break;
	}

	/* call driver */
	err = func(file, cmd, parg);
	if (err == -ENOIOCTLCMD)
		err = -ENOTTY;

	if (err < 0)
		goto out;

	/*  Copy results into user buffer  */
	switch (_IOC_DIR(cmd)) {
	case _IOC_READ:
	case (_IOC_WRITE | _IOC_READ):
		if (copy_to_user((void __user *)arg, parg, _IOC_SIZE(cmd)))
			err = -EFAULT;
		break;
	}

out:
	kfree(mbuf);
	return err;
}

static long _dvb_dsc_ioctl(struct file *file, unsigned int cmd,
			  unsigned long arg)
{
	return _dvb_dsc_usercopy(file, cmd, arg, _dvb_dsc_do_ioctl);
}

static int _dvb_dsc_release(struct inode *inode, struct file *file)
{
	struct dvb_device *dvbdev = file->private_data;
	struct aml_dsc *dsc = dvbdev->priv;

	if (mutex_lock_interruptible(&dsc->mutex))
		return -ERESTARTSYS;

	_dsc_reset(dsc);

	mutex_unlock(&dsc->mutex);

	dvb_generic_release(inode, file);

	return 0;
}

#ifdef CONFIG_COMPAT
static long _dvb_dsc_compat_ioctl(struct file *filp,
			unsigned int cmd, unsigned long args)
{
	unsigned long ret;

	args = (unsigned long)compat_ptr(args);
	ret = _dvb_dsc_ioctl(filp, cmd, args);
	return ret;
}
#endif


static const struct file_operations dvb_dsc_fops = {
	.owner = THIS_MODULE,
	.read = NULL,
	.write = NULL,
	.unlocked_ioctl = _dvb_dsc_ioctl,
	.open = _dvb_dsc_open,
	.release = _dvb_dsc_release,
	.poll = NULL,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= _dvb_dsc_compat_ioctl,
#endif
};

static struct dvb_device dvbdev_dsc = {
	.priv = NULL,
	.users = 1,
	.readers = 1,
	.writers = 1,
	.fops = &dvb_dsc_fops,
};

int dsc_init(struct aml_dsc *dsc, struct dvb_adapter *dvb_adapter)
{
	int i;

	dsc->channel_num = DSC_CHANNEL_NUM;
	dsc->channels = kmalloc(sizeof(struct DescChannel)*dsc->channel_num,GFP_KERNEL);
	if (!dsc->channels) {
		return -1;
	}
	for (i=0; i<dsc->channel_num; i++) {
		dsc->channels[i].state = DSC_STATE_FREE;
		dsc->channels[i].algo = NULL;
		dsc->channels[i].dsc = (void *)dsc;
		dsc->channels[i].set = 0;
		dsc->channels[i].pid = 0x1fff;
	}

	dsc->mode = HW_DSC_MODE;

	/*Register descrambler device */
	return dvb_register_device(dvb_adapter, &dsc->dev,
				  &dvbdev_dsc, dsc, DVB_DEVICE_CA, 0);
}
void dsc_release(struct aml_dsc *dsc)
{
	if (dsc->dev) {
		kfree(dsc->channels);
		dsc->channels = NULL;

		dvb_unregister_device(dsc->dev);
		dsc->dev = NULL;
	}
}

int dsc_set_mode(struct aml_dsc *dsc, int mode) {

	if (dsc->mode != mode) {
		dsc->mode = mode;
	}
	return 0;
}

int dsc_get_mode(struct aml_dsc *dsc, int *mode) {

	*mode = dsc->mode;
	return 0;
}

