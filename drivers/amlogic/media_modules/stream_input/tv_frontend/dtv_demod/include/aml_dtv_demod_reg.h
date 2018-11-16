/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef _DTV_REG_H_
#define _DTV_REG_H_

#include <linux/amlogic/iomap.h>

#define DTV_WRITE_CBUS_REG(_r, _v)   aml_write_cbus(_r, _v)
#define DTV_READ_CBUS_REG(_r)        aml_read_cbus(_r)

#endif	/*  */
