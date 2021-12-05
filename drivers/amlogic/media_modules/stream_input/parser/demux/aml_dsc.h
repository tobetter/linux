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
#ifndef _AML_DSC_H_
#define _AML_DSC_H_

#include "sw_demux/swdemux.h"
#include "dvbdev.h"
#include <dmxdev.h>
#include <linux/device.h>
#include "hw_demux/s2p.h"

#define SW_DSC_MODE 0
#define HW_DSC_MODE 1

#define DVBCSA_MODE 0
#define CIPLUS_MODE 1
#define CBC_MODE 0
#define ECB_MODE 1
#define IDSA_MODE 2

#define DSC_SET_EVEN     1
#define DSC_SET_ODD      2
#define DSC_SET_AES_EVEN 4
#define DSC_SET_AES_ODD  8
#define DSC_FROM_KL      16
#define DSC_SET_SM4_EVEN 32
#define DSC_SET_SM4_ODD  64

#define DSC_KEY_SIZE_MAX 16

struct DescChannel {
	SWDMX_DescChannel *chan;
	SWDMX_DescAlgo   *algo;

	void *dsc;

	int  pid;
	u8   even[DSC_KEY_SIZE_MAX];
	u8   odd[DSC_KEY_SIZE_MAX];
	u8   even_iv[DSC_KEY_SIZE_MAX];
	u8   odd_iv[DSC_KEY_SIZE_MAX];
	int  set;
	int  id;

	int  work_mode;
	int  mode;

	int  state;
};

struct aml_dsc {
	struct dvb_device   *dev;

	int mode;
	int work_mode;
	enum aml_ts_source_t   source;
	enum aml_ts_source_t   dst;

	SWDMX_Descrambler* swdsc;
	int  id;

	int channel_num;
	struct DescChannel *channels;

	struct mutex mutex;
	spinlock_t slock;
};

int dsc_init(struct aml_dsc *dsc, struct dvb_adapter *dvb_adapter);
void dsc_release(struct aml_dsc *dsc);
int dsc_set_mode(struct aml_dsc *dsc, int mode);
int dsc_get_mode(struct aml_dsc *dsc, int *mode);

#endif

