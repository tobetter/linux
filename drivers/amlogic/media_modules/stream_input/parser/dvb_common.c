/*
 * ../hardware/amlogic/media_modules/drivers/stream_input/parser/dvb_common.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
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

#include <linux/printk.h>
#include "dvb_common.h"

#define pr_error(fmt, args...)  printk("DVB: " fmt, ## args)

int dvb_attach_tuner(struct dvb_frontend *fe, struct aml_tuner *tuner,
		enum tuner_type *type)
{
	struct tuner_config *cfg = NULL;
	struct i2c_adapter *i2c_adap = NULL;

	if (fe == NULL || tuner == NULL || type == NULL) {
		pr_error("%s: NULL pointer.\n", __func__);
		return -1;
	}

	cfg = &tuner->cfg;
	i2c_adap = tuner->i2c_adp;

	switch (cfg->id) {
	case AM_TUNER_R840:
		if (!dvb_attach(r840_attach, fe, i2c_adap, cfg)) {
			pr_error("dvb attach r840_attach tuner error\n");
			return -1;
		} else {
			pr_error("r840_attach  attach sucess\n");
			*type = AM_TUNER_R840;
		}
		break;
	case AM_TUNER_R842:
		if (!dvb_attach(r842_attach, fe, i2c_adap, cfg)) {
			pr_error("dvb attach r842_attach tuner error\n");
			return -1;
		} else {
			pr_error("r842_attach  attach sucess\n");
			*type = AM_TUNER_R842;
		}
		break;
	case AM_TUNER_SI2151:
		if (!dvb_attach(si2151_attach, fe, i2c_adap, cfg)) {
			pr_error("dvb attach tuner error\n");
			return -1;
		} else {
			pr_error("si2151 attach sucess\n");
			*type = AM_TUNER_SI2151;
		}
		break;
	case AM_TUNER_SI2159:
		if (!dvb_attach(si2159_attach, fe, i2c_adap, cfg)) {
			pr_error("dvb attach si2159_attach tuner error\n");
			return -1;
		} else {
			pr_error("si2159_attach  attach sucess\n");
			*type = AM_TUNER_SI2159;
		}
		break;
	case AM_TUNER_MXL661:
		if (!dvb_attach(mxl661_attach, fe, i2c_adap, cfg)) {
			pr_error("dvb attach mxl661_attach tuner error\n");
			return -1;
		} else {
			pr_error("mxl661_attach  attach sucess\n");
			*type = AM_TUNER_MXL661;
		}
		break;
	case AM_TUNER_ATBM2040:
		if (!dvb_attach(atbm2040_attach, fe, i2c_adap, cfg)) {
			pr_error("dvb attach atbm2040_attach tuner error\n");
			return -1;
		} else {
			pr_error("atbm2040_attach  attach sucess\n");
			*type = AM_TUNER_ATBM2040;
		}
		break;
	default:
		pr_error("can't support tuner type: %d\n", cfg->id);
		break;
	}

	return 0;
}
EXPORT_SYMBOL(dvb_attach_tuner);

int dvb_detach_tuner(enum tuner_type *type)
{
	if (type == NULL) {
		pr_error("%s: type == NULL\n", __func__);
		return -1;
	}

	switch (*type) {
	case AM_TUNER_R840:
		dvb_detach(r840_attach);
		*type = AM_TUNER_NONE;
		break;
	case AM_TUNER_R842:
		dvb_detach(r842_attach);
		*type = AM_TUNER_NONE;
		break;
	case AM_TUNER_SI2151:
		dvb_detach(si2151_attach);
		*type = AM_TUNER_NONE;
		break;
	case AM_TUNER_SI2159:
		dvb_detach(si2159_attach);
		*type = AM_TUNER_NONE;
		break;
	case AM_TUNER_MXL661:
		dvb_detach(mxl661_attach);
		*type = AM_TUNER_NONE;
		break;
	case AM_TUNER_ATBM2040:
		dvb_detach(atbm2040_attach);
		*type = AM_TUNER_NONE;
		break;
	default:
		pr_error("can't support tuner type: %d\n", *type);
		break;
	}

	return 0;
}
EXPORT_SYMBOL(dvb_detach_tuner);

struct dvb_frontend *dvb_attach_demod(struct demod_config *cfg,
		enum dtv_demod_type *type, const char *name)
{
	struct dvb_frontend *fe = NULL;

	if (cfg == NULL || type == NULL || name == NULL) {
		pr_error("%s: NULL pointer.\n", __func__);
		return NULL;
	}

	if (!strncmp(name, "internal", strlen("internal"))) {
		fe = dvb_attach(aml_dtvdm_attach, cfg);
		if (fe)
			*type = AM_DTV_DEMOD_AML;
	} else if (!strncmp(name, "Si2168", strlen("Si2168"))) {
		fe = dvb_attach(si2168_attach, cfg);
		if (fe)
			*type = AM_DTV_DEMOD_SI2168;
	} else if (!strncmp(name, "Si2168-1", strlen("Si2168-1"))) {
		fe = dvb_attach(si2168_attach_1, cfg);
		if (fe)
			*type = AM_DTV_DEMOD_SI2168_1;
	} else if (!strncmp(name, "Avl6762", strlen("Avl6762"))) {
		fe = dvb_attach(avl6762_attach, cfg);
		if (fe)
			*type = AM_DTV_DEMOD_AVL6762;
	} else if (!strncmp(name, "cxd2856", strlen("cxd2856"))) {
		fe = dvb_attach(cxd2856_attach, cfg);
		if (fe)
			*type = AM_DTV_DEMOD_CXD2856;
	} else if (!strncmp(name, "atbm7821", strlen("atbm7821"))) {
		fe = dvb_attach(atbm7821_attach, cfg);
		if (fe)
			*type = AM_DTV_DEMOD_ATBM7821;
	} else if (!strncmp(name, "mxl248", strlen("mxl248"))) {
		fe = dvb_attach(mxl248_attach, cfg);
		if (fe)
			*type = AM_DTV_DEMOD_MXL248;
	} else {
		pr_error("can't support demod type: %s\n", name);
	}

	return fe;
}
EXPORT_SYMBOL(dvb_attach_demod);

int dvb_detach_demod(enum dtv_demod_type *type)
{
	if (type == NULL) {
		pr_error("%s: type == NULL\n", __func__);
		return -1;
	}

	switch (*type) {
	case AM_DTV_DEMOD_AML:
		dvb_detach(aml_dtvdm_attach);
		*type = AM_DTV_DEMOD_NONE;
		break;
	case AM_DTV_DEMOD_ATBM8881:
		dvb_detach(atbm8881_attach);
		*type = AM_DTV_DEMOD_NONE;
		break;
	case AM_DTV_DEMOD_SI2168:
		dvb_detach(si2168_attach);
		*type = AM_DTV_DEMOD_NONE;
		break;
	case AM_DTV_DEMOD_SI2168_1:
		dvb_detach(si2168_attach_1);
		*type = AM_DTV_DEMOD_NONE;
		break;
	case AM_DTV_DEMOD_AVL6762:
		dvb_detach(avl6762_attach);
		*type = AM_DTV_DEMOD_NONE;
		break;
	case AM_DTV_DEMOD_CXD2856:
		dvb_detach(cxd2856_attach);
		*type = AM_DTV_DEMOD_NONE;
		break;
	case AM_DTV_DEMOD_ATBM7821:
		dvb_detach(atbm7821_attach);
		*type = AM_DTV_DEMOD_NONE;
		break;
	case AM_DTV_DEMOD_MXL248:
		dvb_detach(mxl248_attach);
		*type = AM_DTV_DEMOD_NONE;
		break;
	default:
		pr_error("can't support demod type: %d\n", *type);
		break;
	}

	return 0;
}
EXPORT_SYMBOL(dvb_detach_demod);
