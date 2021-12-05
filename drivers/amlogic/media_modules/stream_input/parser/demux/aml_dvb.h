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
#ifndef _AML_DVB_H_
#define _AML_DVB_H_

#include "aml_dmx.h"
#include "aml_dsc.h"
#include "hw_demux/hwdemux.h"
#include "hw_demux/demod_gt.h"

#define CHAIN_PATH_COUNT  5
#define DMX_DEV_COUNT     5
#define DSC_DEV_COUNT     2
#define FE_DEV_COUNT 	  2


struct aml_dvb {
	struct dvb_device    dvb_dev;
	struct dvb_adapter   dvb_adapter;

	struct device       *dev;
	struct platform_device *pdev;

	HWDMX_Demux *hwdmx[CHAIN_PATH_COUNT];
	SWDMX_TsParser *tsp[CHAIN_PATH_COUNT];
	SWDMX_Descrambler *swdsc[CHAIN_PATH_COUNT];
	SWDMX_Demux     *swdmx[CHAIN_PATH_COUNT];

	struct aml_dmx 	dmx[DMX_DEV_COUNT];
	struct aml_dsc	dsc[DSC_DEV_COUNT];

	struct mutex mutex;
	spinlock_t slock;

	int ts_out_invert;

	unsigned int tuner_num;
	unsigned int tuner_cur;
	struct aml_tuner *tuners;
	bool tuner_attached;
};

struct aml_dvb *aml_get_dvb_device(void);

#endif
