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
#ifndef _S2P_H_
#define _S2P_H_

enum aml_ts_source_t {
	AM_TS_SRC_TS0,
	AM_TS_SRC_TS1,
	AM_TS_SRC_TS2,
	AM_TS_SRC_TS3,

	AM_TS_SRC_S_TS0,
	AM_TS_SRC_S_TS1,
	AM_TS_SRC_S_TS2,

	AM_TS_SRC_HIU,
	AM_TS_SRC_DMX0,
	AM_TS_SRC_DMX1,
	AM_TS_SRC_DMX2
};

enum{
	AM_TS_DISABLE,
	AM_TS_PARALLEL,
	AM_TS_SERIAL
};

struct aml_ts_input {
	int                  mode;
	struct pinctrl      *pinctrl;
	int                  control;
	int                  s2p_id;
};

struct aml_s2p {
	int    invert;
};

int s2p_probe(struct platform_device *pdev);
int s2p_remove(void);

int getts(struct aml_ts_input **ts_p);
int gets2p(struct aml_s2p **s2p_p);


#endif
