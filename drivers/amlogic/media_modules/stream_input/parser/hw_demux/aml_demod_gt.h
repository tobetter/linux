/*
 * test
 *
 * Copyright (C) 2018 <crope@iki.fi>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 */


#ifndef __AML_DEMOD_GT_H__
#define __AML_DEMOD_GT_H__

#include  "dvb_frontend.h"

struct amlfe_exp_config {
	/*config by aml_fe ?*/
	/* */
	int set_mode;
};

static inline struct dvb_frontend* aml_dtvdm_attach (const struct amlfe_exp_config *config) {
	return NULL;
}

static inline struct dvb_frontend* si2151_attach (struct dvb_frontend *fe,struct i2c_adapter *i2c,u8 i2c_addr/*,
				     struct si2151_config *cfg*/)
{
	return NULL;
}
static inline struct dvb_frontend* mxl661_attach (struct dvb_frontend *fe,struct i2c_adapter *i2c,u8 i2c_addr/*,
				     struct si2151_config *cfg*/)
{
	return NULL;
}
#endif	/*__AML_DEMOD_GT_H__*/
