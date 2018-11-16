/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */



#ifndef _AML_VCODEC_DEC_PM_H_
#define _AML_VCODEC_DEC_PM_H_

#include "aml_vcodec_drv.h"

int aml_vcodec_init_dec_pm(struct aml_vcodec_dev *dev);
void aml_vcodec_release_dec_pm(struct aml_vcodec_dev *dev);

void aml_vcodec_dec_pw_on(struct aml_vcodec_pm *pm);
void aml_vcodec_dec_pw_off(struct aml_vcodec_pm *pm);
void aml_vcodec_dec_clock_on(struct aml_vcodec_pm *pm);
void aml_vcodec_dec_clock_off(struct aml_vcodec_pm *pm);

#endif /* _AML_VCODEC_DEC_PM_H_ */
