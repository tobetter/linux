/*
 *
 * Copyright (C) 2018 Amlogic, Inc. All rights reserved.
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

#ifndef _HWDMX_INTERNAL_H_
#define _HWDMX_INTERNAL_H_

#include "hwdemux.h"

#define CHANNEL_COUNT     31

struct HWDMX_Chan_s {
	int                  type;
	int 				pes_type;
	int                  pid;
	int                  used;
	int					 cid;
	HWDMX_Demux 		 *pdmx;
};

struct HWDMX_Demux_s
{
	int ts_id;
	int dmx_id;
	int asyncfifo_id;
	int	ts_out_invert;
	int dmx_irq;
	int dmx_source;
	int dsc_id;

	int id;

	int ts_in_total_count;
	struct aml_ts_input *ts;

	int s2p_total_count;
	struct aml_s2p *s2p;

	int chan_count;
	struct HWDMX_Chan_s   channel[CHANNEL_COUNT+1];

	HWDMX_Cb cb;
	void *udata;

	int dump_ts_select;

	int inject_init;
	int init;
};

#endif

