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
#ifndef _AML_HWDMX_H_
#define _AML_HWDMX_H_
#include <linux/dvb/dmx.h>

enum aml_dmx_id_t {
	AM_DMX_0 = 0,
	AM_DMX_1,
	AM_DMX_2,
	AM_DMX_MAX,
};

typedef struct HWDMX_Demux_s HWDMX_Demux;
typedef struct HWDMX_Chan_s HWDMX_Chan;
typedef int (*HWDMX_Cb)(char *buf, int count, void *udata);

int hwdmx_probe(struct platform_device *pdev);
int hwdmx_remove(void);

HWDMX_Demux *hwdmx_create(int ts_id, int dmx_id, int asyncfifo_id);
void hwdmx_destory(HWDMX_Demux *pdmx);
int hwdmx_set_cb(HWDMX_Demux *pdmx, HWDMX_Cb cb, void *udata);
int hwdmx_inject(HWDMX_Demux *pdmx, const char *buf, int count);
void hwdmx_inject_destroy(HWDMX_Demux *pdmx);

HWDMX_Chan *hwdmx_alloc_chan(HWDMX_Demux *pdmx);
int hwdmx_free_chan(HWDMX_Chan *chan);
int hwdmx_set_pid(HWDMX_Chan *chan,int type, int pes_type, int pid);
int hwdmx_set_dsc(HWDMX_Demux *pdmx,int dsc_id, int link);
int hwdmx_get_dsc(HWDMX_Demux *pdmx,int *dsc_id);

int hwdmx_set_source(HWDMX_Demux *pdmx, dmx_source_t src);
int hwdmx_get_source(HWDMX_Demux *pdmx, dmx_source_t *src);

#endif

