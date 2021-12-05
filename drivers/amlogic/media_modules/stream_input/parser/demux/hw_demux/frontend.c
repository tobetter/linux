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
//#include <asm/irq.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/pinctrl/consumer.h>
#include <linux/reset.h>
#include <linux/of_gpio.h>
#include <linux/amlogic/media/utils/amstream.h>
//#include <linux/clk.h>
#include "c_stb_define.h"
#include "c_stb_regs_define.h"
#include "../aml_dvb.h"
#include "dvb_reg.h"

#include "demod_gt.h"
#include "../../../../common/media_clock/switch/amports_gate.h"

#define pr_error(fmt, args...) printk("DVB: " fmt, ## args)
#define pr_inf(fmt, args...)   printk("DVB: " fmt, ## args)

static struct dvb_frontend *frontend[FE_DEV_COUNT] = {NULL, NULL};
static enum dtv_demod_type s_demod_type[FE_DEV_COUNT] = {AM_DTV_DEMOD_NONE, AM_DTV_DEMOD_NONE};
static enum tuner_type s_tuner_type[FE_DEV_COUNT] = {AM_TUNER_NONE, AM_TUNER_NONE};


ssize_t stb_show_tuner_setting(struct class *class,
				   struct class_attribute *attr, char *buf)
{
	struct aml_dvb *dvb = aml_get_dvb_device();

	if (dvb->tuner_cur >= 0)
		pr_inf("dvb current attatch tuner %d, id: %d\n",
				dvb->tuner_cur, dvb->tuners[dvb->tuner_cur].cfg.id);
	else
		pr_inf("dvb has no attatch tuner.\n");

	return 0;
}

ssize_t stb_store_tuner_setting(struct class *class,
				    struct class_attribute *attr,
				    const char *buf, size_t count)
{
	int n = 0, i = 0, val = 0;
	unsigned long tmp = 0;
	char *buf_orig = NULL, *ps = NULL, *token = NULL;
	char *parm[4] = { NULL };
	struct aml_dvb *dvb = aml_get_dvb_device();
	int tuner_id = 0;
	struct aml_tuner *tuner = NULL;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	ps = buf_orig;

	while (1) {
		token = strsep(&ps, "\n ");
		if (token == NULL)
			break;
		if (*token == '\0')
			continue;
		parm[n++] = token;
	}

	if (parm[0] && kstrtoul(parm[0], 10, &tmp) == 0) {
		val = tmp;

		for (i = 0; i < dvb->tuner_num; ++i) {
			if (dvb->tuners[i].cfg.id == val) {
				tuner_id = dvb->tuners[i].cfg.id;
				break;
			}
		}

		if (tuner_id == 0 || dvb->tuner_cur == i) {
			pr_error("%s: set nonsupport or the same tuner %d.\n",
					__func__, val);
			goto EXIT;
		}

		dvb->tuner_cur = i;

		for (i = 0; i < FE_DEV_COUNT; i++) {
			tuner = &dvb->tuners[dvb->tuner_cur];

			if (frontend[i] == NULL)
				continue;

			if (dvb_attach_tuner(frontend[i], tuner, &s_tuner_type[i]) < 0) {
				pr_error("attach tuner %d failed\n", dvb->tuner_cur);
				goto EXIT;
			}
		}

		pr_error("%s: attach tuner %d done.\n", __func__, dvb->tuners[dvb->tuner_cur].cfg.id);
	}

EXIT:

	return count;
}


int frontend_probe(struct platform_device *pdev)
{
	struct demod_config config;
	char buf[32];
	const char *str = NULL;
	struct device_node *node_tuner = NULL;
	struct device_node *node_i2c = NULL;
	u32 i2c_addr = 0xFFFFFFFF;
	u32 value = 0;
	int i = 0;
	int ret =0;
	int j = 0;
	struct aml_dvb *advb = aml_get_dvb_device();

	for (i=0; i<FE_DEV_COUNT; i++) {
		memset(&config, 0, sizeof(struct demod_config));

		memset(buf, 0, 32);
		snprintf(buf, sizeof(buf), "fe%d_mode", i);
		ret = of_property_read_string(pdev->dev.of_node, buf, &str);
		if (ret) {
			continue;
		}
		if (!strcmp(str,"internal")) {
			config.mode = 0;

			frontend[i] = dvb_attach_demod(&config, &s_demod_type[i], "internal");
			if (frontend[i] == NULL) {
				pr_error("dvb attach demod error\n");
				goto error_fe;
			} else {
				pr_inf("dtvdemod attatch sucess\n");
				s_demod_type[i] = AM_DTV_DEMOD_AML;
			}

			memset(buf, 0, 32);
			snprintf(buf, sizeof(buf), "fe%d_tuner",i);
			node_tuner = of_parse_phandle(pdev->dev.of_node, buf, 0);
			if (!node_tuner){
				pr_err("can't find tuner.\n");
				goto error_fe;
			}
			ret = of_property_read_u32(node_tuner, "tuner_num", &value);
			if (ret) {
				pr_err("can't find tuner_num.\n");
				goto error_fe;
			} else
				advb->tuner_num = value;

			advb->tuners = kzalloc(sizeof(struct aml_tuner) * advb->tuner_num, GFP_KERNEL);
			if (!advb->tuners) {
				pr_err("can't kzalloc for tuners.\n");
				goto error_fe;
			}

			ret = of_property_read_u32(node_tuner, "tuner_cur", &value);
			if (ret) {
				pr_err("can't find tuner_cur, use default 0.\n");
				advb->tuner_cur = -1;
			} else
				advb->tuner_cur = value;

			for (j = 0; j < advb->tuner_num; ++j) {
				snprintf(buf, sizeof(buf), "tuner_name_%d", j);
				ret = of_property_read_string(node_tuner, buf, &str);
				if (ret) {
					//pr_error("tuner%d type error\n",i);
					ret = 0;
					continue;
				} else {
					if (!strncmp(str, "mxl661_tuner", 12))
						advb->tuners[j].cfg.id = AM_TUNER_MXL661;
					else if (!strncmp(str, "si2151_tuner", 12))
						advb->tuners[j].cfg.id = AM_TUNER_SI2151;
					else if (!strncmp(str, "si2159_tuner", 12))
						advb->tuners[j].cfg.id = AM_TUNER_SI2159;
					else if (!strncmp(str, "r840_tuner", 10))
						advb->tuners[j].cfg.id = AM_TUNER_R840;
					else if (!strncmp(str, "r842_tuner", 10))
						advb->tuners[j].cfg.id = AM_TUNER_R842;
					else if (!strncmp(str, "atbm2040_tuner", 14))
						advb->tuners[i].cfg.id = AM_TUNER_ATBM2040;
					else {
						pr_err("nonsupport tuner: %s.\n", str);
						advb->tuners[j].cfg.id = AM_TUNER_NONE;
					}
				}

				snprintf(buf, sizeof(buf), "tuner_i2c_adap_%d", j);
				node_i2c = of_parse_phandle(node_tuner, buf, 0);
				if (!node_i2c) {
					pr_error("tuner_i2c_adap_id error\n");
				} else {
					advb->tuners[j].i2c_adp = of_find_i2c_adapter_by_node(node_i2c);
					of_node_put(node_i2c);
					if (advb->tuners[j].i2c_adp == NULL) {
						pr_error("i2c_get_adapter error\n");
						of_node_put(node_tuner);
						goto error_fe;
					}
				}

				snprintf(buf, sizeof(buf), "tuner_i2c_addr_%d", j);
				ret = of_property_read_u32(node_tuner, buf, &i2c_addr);
				if (ret) {
					pr_error("i2c_addr error\n");
				}
				else
					advb->tuners[j].cfg.i2c_addr = i2c_addr;

				snprintf(buf, sizeof(buf), "tuner_xtal_%d", j);
				ret = of_property_read_u32(node_tuner, buf, &value);
				if (ret)
					pr_err("tuner_xtal error.\n");
				else
					advb->tuners[j].cfg.xtal = value;

				snprintf(buf, sizeof(buf), "tuner_xtal_mode_%d", j);
				ret = of_property_read_u32(node_tuner, buf, &value);
				if (ret)
					pr_err("tuner_xtal_mode error.\n");
				else
					advb->tuners[j].cfg.xtal_mode = value;

				snprintf(buf, sizeof(buf), "tuner_xtal_cap_%d", j);
				ret = of_property_read_u32(node_tuner, buf, &value);
				if (ret)
					pr_err("tuner_xtal_cap error.\n");
				else
					advb->tuners[j].cfg.xtal_cap = value;
			}

			of_node_put(node_tuner);

			/* define general-purpose callback pointer */
			frontend[i]->callback = NULL;

			if (advb->tuner_cur >= 0) {
				if (dvb_attach_tuner(frontend[i], &advb->tuners[advb->tuner_cur], &s_tuner_type[i]) < 0) {
					pr_error("attach tuner failed\n");
					goto error_fe;
				}
			}

			ret = dvb_register_frontend(&advb->dvb_adapter, frontend[i]);
			if (ret) {
				pr_error("register dvb frontend failed\n");
				goto error_fe;
			}
		} else if(!strcmp(str,"external")) {
			const char *name = NULL;

			config.mode = 1;
			config.dev_id = i;
			memset(buf, 0, 32);
			snprintf(buf, sizeof(buf), "fe%d_demod",i);
			ret = of_property_read_string(pdev->dev.of_node, buf, &name);
			if (ret) {
				ret = 0;
				continue;
			}

			memset(buf, 0, 32);
			snprintf(buf, sizeof(buf), "fe%d_i2c_adap_id",i);
			node_i2c = of_parse_phandle(pdev->dev.of_node,buf,0);
			if (!node_i2c) {
				pr_error("demod%d_i2c_adap_id error\n", i);
			} else {
				config.i2c_adap = of_find_i2c_adapter_by_node(node_i2c);
				of_node_put(node_i2c);
				if (config.i2c_adap == NULL) {
					pr_error("i2c_get_adapter error\n");
					goto error_fe;
				}
			}

			memset(buf, 0, 32);
			snprintf(buf, sizeof(buf), "fe%d_demod_i2c_addr",i);
			ret = of_property_read_u32(pdev->dev.of_node, buf,&config.i2c_addr);
			if (ret) {
				pr_error("i2c_addr error\n");
				goto error_fe;
			}

			memset(buf, 0, 32);
			snprintf(buf, sizeof(buf), "fe%d_ts",i);
			ret = of_property_read_u32(pdev->dev.of_node, buf,&config.ts);
			if (ret) {
				pr_error("ts error\n");
				goto error_fe;
			}

			memset(buf, 0, 32);
			snprintf(buf, sizeof(buf), "fe%d_reset_gpio",i);
			ret = of_property_read_string(pdev->dev.of_node, buf, &str);
			if (!ret) {
				config.reset_gpio =
				     of_get_named_gpio_flags(pdev->dev.of_node,
				     buf, 0, NULL);
				pr_inf("%s: %d\n", buf, config.reset_gpio);
			} else {
				config.reset_gpio = -1;
				pr_error("cannot find resource \"%s\"\n", buf);
				goto error_fe;
			}

			memset(buf, 0, 32);
			snprintf(buf, sizeof(buf), "fe%d_reset_value",i);
			ret = of_property_read_u32(pdev->dev.of_node, buf,&config.reset_value);
			if (ret) {
				pr_error("reset_value error\n");
				config.reset_value = 0;
				goto error_fe;
			}

			memset(buf, 0, 32);
			snprintf(buf, sizeof(buf), "fe%d_ant_power_gpio", i);
			ret = of_property_read_string(pdev->dev.of_node, buf, &str);
			if (!ret) {
				config.ant_power_gpio =
					 of_get_named_gpio_flags(pdev->dev.of_node,
					 buf, 0, NULL);
				pr_inf("%s: %d\n", buf, config.ant_power_gpio);
			} else {
				config.ant_power_gpio = -1;
				pr_error("cannot find resource \"%s\"\n", buf);
			}

			memset(buf, 0, 32);
			snprintf(buf, sizeof(buf), "fe%d_ant_poweron_value", i);
			ret = of_property_read_u32(pdev->dev.of_node, buf, &config.ant_power_value);
			if (ret) {
				pr_error("ant_power_value error\n");
				config.ant_power_value = 0;
			}

			memset(buf, 0, 32);
			snprintf(buf, sizeof(buf), "fe%d_tuner0_i2c_addr",i);
			ret = of_property_read_u32(pdev->dev.of_node, buf,&config.tuner0_i2c_addr);
			if (ret) {
				pr_error("no tuner0 i2c_addr define\n");
			}

			memset(buf, 0, 32);
			snprintf(buf, sizeof(buf), "fe%d_tuner1_i2c_addr",i);
			ret = of_property_read_u32(pdev->dev.of_node, buf,&config.tuner1_i2c_addr);
			if (ret) {
				pr_error("no tuner1 addr define\n");
			}

			memset(buf, 0, 32);
			snprintf(buf, sizeof(buf), "fe%d_tuner0_code",i);
			ret = of_property_read_u32(pdev->dev.of_node, buf,&config.tuner0_code);
			if (ret) {
				pr_error("no tuner0_code define\n");
			}

			memset(buf, 0, 32);
			snprintf(buf, sizeof(buf), "fe%d_tuner1_code",i);
			ret = of_property_read_u32(pdev->dev.of_node, buf,&config.tuner1_code);
			if (ret) {
				pr_error("no tuner1_code define\n");
			}

			frontend[i] = dvb_attach_demod(&config, &s_demod_type[i], name);
			if (frontend[i] == NULL) {
				pr_error("dvb attach demod error\n");
				goto error_fe;
			} else
				pr_inf("dtvdemod attatch sucess\n");

			if (frontend[i]) {
				ret = dvb_register_frontend(&advb->dvb_adapter, frontend[i]);
				if (ret) {
					pr_error("register dvb frontend failed\n");
					goto error_fe;
				}
			}
		}
	}
	if (advb->tuners)
		kfree(advb->tuners);
	return 0;
error_fe:
	for (i=0; i<FE_DEV_COUNT; i++) {
		dvb_detach_demod(&s_demod_type[i]);
		frontend[i] = NULL;
		s_demod_type[i] = AM_DTV_DEMOD_NONE;

		dvb_detach_tuner(&s_tuner_type[i]);
		s_tuner_type[i] = AM_TUNER_NONE;
	}

	if (advb->tuners)
		kfree(advb->tuners);

	return 0;
}

int frontend_remove(void)
{
	int i;

	for (i=0; i<FE_DEV_COUNT; i++) {
		dvb_detach_demod(&s_demod_type[i]);

		dvb_detach_tuner(&s_tuner_type[i]);

		if (frontend[i] &&
			((s_tuner_type[i] == AM_TUNER_SI2151)
			|| (s_tuner_type[i] == AM_TUNER_MXL661)
			|| (s_tuner_type[i] == AM_TUNER_SI2159)
			|| (s_tuner_type[i] == AM_TUNER_R842)
			|| (s_tuner_type[i] == AM_TUNER_R840)
			|| (s_tuner_type[i] == AM_TUNER_ATBM2040))) {
			dvb_unregister_frontend(frontend[i]);
			dvb_frontend_detach(frontend[i]);
		}

		frontend[i] = NULL;
		s_demod_type[i] = AM_DTV_DEMOD_NONE;
		s_tuner_type[i] = AM_TUNER_NONE;

	}
	return 0;
}

