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
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/fcntl.h>
#include <asm/irq.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <asm/cacheflush.h>
#include <linux/dma-mapping.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/vmalloc.h>
#include <linux/of.h>

#include "s2p.h"

#define TS_INPUT_COUNT 		5
#define S2P_COUNT			5

#define pr_error(fmt, args...) printk("DVB: " fmt, ## args)
#define pr_inf(fmt, args...)   printk("DVB: " fmt, ## args)

static int ts_in_total_count = 0;
static int s2p_total_count = 0;
static struct aml_ts_input ts[TS_INPUT_COUNT];
static struct aml_s2p s2p[S2P_COUNT];

int s2p_probe(struct platform_device *pdev) {

#ifdef CONFIG_OF
	int i = 0;
	int ret = 0;
	char buf[32];
	u32 value;

	memset(buf, 0, 32);
	snprintf(buf, sizeof(buf), "ts_in_count");
	ret = of_property_read_u32(pdev->dev.of_node, buf, &value);
	if (!ret) {
		pr_inf("%s: 0x%x\n", buf, value);
		if (value > TS_INPUT_COUNT) {
			ts_in_total_count = TS_INPUT_COUNT;
			pr_inf("ts_in %d bigger than array num %d \n", value, ts_in_total_count);
		} else {
			ts_in_total_count = value;
		}
	} else {
		ts_in_total_count = 3;
	}

	memset(buf, 0, 32);
	snprintf(buf, sizeof(buf), "s2p_count");
	ret = of_property_read_u32(pdev->dev.of_node, buf, &value);
	if (!ret) {
		pr_inf("%s: 0x%x\n", buf, value);
		if (value > S2P_COUNT) {
			s2p_total_count = S2P_COUNT;
			pr_inf("s2p_count %d bigger than array num %d \n", value, s2p_total_count);
		} else {
			s2p_total_count = value;
		}
	} else {
		s2p_total_count = 2;
	}

	if (pdev->dev.of_node) {
		int s2p_id = 0;
		char buf[32];
		const char *str;
		u32 value;

		for (i = 0; i < ts_in_total_count; i++) {

			ts[i].mode = AM_TS_DISABLE;
			ts[i].s2p_id = -1;
			ts[i].pinctrl = NULL;
			memset(buf, 0, 32);
			snprintf(buf, sizeof(buf), "ts%d", i);
			ret =
				of_property_read_string(pdev->dev.of_node, buf,
							&str);
			if (!ret) {
				if (!strcmp(str, "serial")) {
					pr_inf("%s: serial\n", buf);

					if (s2p_id >= S2P_COUNT)
						pr_error("no free s2p\n");
					else {
						snprintf(buf, sizeof(buf),
							 "s_ts%d", i);
						ts[i].mode = AM_TS_SERIAL;
						ts[i].pinctrl =
							devm_pinctrl_get_select
							(&pdev->dev, buf);
						ts[i].s2p_id = s2p_id;

						s2p_id++;
					}
				} else if (!strcmp(str, "parallel")) {
					pr_inf("%s: parallel\n", buf);
					memset(buf, 0, 32);
					snprintf(buf, sizeof(buf), "p_ts%d", i);
					ts[i].mode = AM_TS_PARALLEL;
					ts[i].pinctrl =
						devm_pinctrl_get_select(&pdev->dev,
									buf);
				} else {
					ts[i].mode = AM_TS_DISABLE;
					ts[i].pinctrl = NULL;
				}
			}
			memset(buf, 0, 32);
			snprintf(buf, sizeof(buf), "ts%d_control", i);
			ret =
				of_property_read_u32(pdev->dev.of_node, buf,
						 &value);
			if (!ret) {
				pr_inf("%s: 0x%x\n", buf, value);
				ts[i].control = value;
			} else {
				pr_inf("read error:%s: 0x%x\n", buf, value);
			}

			if (ts[i].s2p_id != -1) {
				memset(buf, 0, 32);
				snprintf(buf, sizeof(buf), "ts%d_invert", i);
				ret =
					of_property_read_u32(pdev->dev.of_node, buf,
							 &value);
				if (!ret) {
					pr_inf("%s: 0x%x\n", buf, value);
					s2p[ts[i].s2p_id].invert =
						value;
				}
			}
		}
	}
#endif
	return 0;
}

int s2p_remove(void) {
	return 0;
}

int getts(struct aml_ts_input **ts_p)
{
	*ts_p = ts;
	return ts_in_total_count;
}

int gets2p(struct aml_s2p **s2p_p)
{
	*s2p_p = s2p;
	return s2p_total_count;
}



