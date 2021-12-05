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
#ifndef _AML_DMX_H_
#define _AML_DMX_H_

#include "sw_demux/swdemux.h"
#include "hw_demux/hwdemux.h"
#include "demux.h"
#include "dvbdev.h"
#include <dmxdev.h>

struct sw_demux_ts_feed {
	struct dmx_ts_feed ts_feed;

	SWDMX_TsFilter		 *tsf;
	HWDMX_Chan			*tschan;
//	HWDMX_TsFilter		 *hwtsf;
	dmx_ts_cb ts_cb;
	int type;
	int ts_type;
	int pes_type;
	int pid;
	int state;
};

struct sw_demux_sec_filter {
	struct dmx_section_filter section_filter;

	SWDMX_SecFilter 	 *secf;
	int state;
};

struct sw_demux_sec_feed {
	struct dmx_section_feed sec_feed;

	int sec_filter_num;
	struct sw_demux_sec_filter *filter;
	HWDMX_Chan			*secchan;

	dmx_section_cb sec_cb;
	int pid;
	int check_crc;
	int type;
	int state;
};

struct aml_dmx {
	struct dmx_demux dmx;
	struct dmxdev dev;
	void *priv;
	int id;

	HWDMX_Demux		*hwdmx;
	SWDMX_Demux     *swdmx;
	SWDMX_TsParser  *tsp;

	int used_hwdmx;

	int ts_feed_num;
	struct sw_demux_ts_feed *ts_feed;

	int sec_feed_num;
	struct sw_demux_sec_feed *section_feed;

	struct list_head frontend_list;

	u16 pids[DMX_PES_OTHER];

	struct dmx_frontend  mem_fe;

	int buf_warning_level; //percent, used/total, default is 60

#define MAX_SW_DEMUX_USERS 10
	int users;
	struct mutex *pmutex;
	spinlock_t *pslock;

	int init;
};

int dmx_init(struct aml_dmx *pdmx,struct dvb_adapter *dvb_adapter);
int dmx_destroy(struct aml_dmx *pdmx);
int dmx_set_work_mode(struct aml_dmx *pdmx,int work_mode);
int dmx_get_work_mode(struct aml_dmx *pdmx,int *work_mode);
int dmx_get_buf_warning_status(struct aml_dmx *pdmx, int *status);
int dmx_set_buf_warning_level(struct aml_dmx *pdmx, int level);
int dmx_write_sw_from_user(struct aml_dmx *pdmx, const char __user *buf, size_t count);

#endif
