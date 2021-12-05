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
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/of.h>

#include "aml_dvb.h"
#include "aml_dmx_ext.h"
#include "hw_demux/hwdemux.h"

#define pr_error(fmt, args...) printk("DVB: " fmt, ## args)
#define pr_inf(fmt, args...)   printk("DVB: " fmt, ## args)

#define CARD_NAME "amlogic-dvb"

DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);

extern ssize_t stb_show_tuner_setting(struct class *class,
				   struct class_attribute *attr, char *buf);
extern ssize_t stb_store_tuner_setting(struct class *class,
				    struct class_attribute *attr,
				    const char *buf, size_t count);

static struct aml_dvb aml_dvb_device;
typedef struct _ChainPath
{
	int ts_id;
	int dmx_id;
	int asyncfifo_id;
}ChainPath;

#define MAX_DMXCHAINPATH_NUM 3
static ChainPath DmxChainPath[MAX_DMXCHAINPATH_NUM];

#define DSC_MODE_FUNC_DECL(i)\
static ssize_t dsc##i##_store_mode(struct class *class,\
				struct class_attribute *attr, const char *buf,size_t size) {\
	if (!strncmp("sw", buf, 2)) {\
		dsc_set_mode(&aml_dvb_device.dsc[i], SW_DSC_MODE);\
	} else if (!strncmp("hw", buf, 2)) {\
		dsc_set_mode(&aml_dvb_device.dsc[i], HW_DSC_MODE);\
	}\
	return 0;\
}\
static ssize_t dsc##i##_show_mode(struct class *class,\
			       struct class_attribute *attr, char *buf) {\
	int mode = 0;\
	char *str ;\
	ssize_t ret = 0;\
	dsc_get_mode(&aml_dvb_device.dsc[i],&mode);\
	if (mode == SW_DSC_MODE) {\
		str = "sw";\
	} else if (mode == HW_DSC_MODE) {\
		str = "hw";\
	}else {\
		str = "none";\
	}\
	ret = sprintf(buf,"%s\n",str);\
	return ret;\
}

#define DSC_PATH_FUNC_DECL(i)  \
static ssize_t dsc##i##_store_path(struct class *class,\
				struct class_attribute *attr, const char *buf,\
				size_t size) {\
	struct aml_dvb *dvb = &aml_dvb_device;\
	int path = 0;\
	int link = 0;\
	int ret = 0;\
	unsigned long flags;\
	if (size < 13)\
		goto ERROR;\
	if (!strncmp("path=",buf,5)) {\
		path = buf[5]-0x30;\
	} else if (!strncmp("link=",buf+7,6)){\
		link = buf[12]-0x30;\
	} else {\
		goto ERROR;\
	}\
	if (path > CHAIN_PATH_COUNT)\
		goto ERROR;\
	spin_lock_irqsave(&dvb->slock, flags);\
	ret = hwdmx_set_dsc(aml_dvb_device.hwdmx[path],i,link);\
	spin_unlock_irqrestore(&dvb->slock, flags);\
	if (ret == 0)\
		return 0;\
ERROR:\
	pr_error("error, such as path=0,link=1");\
	return 0;\
}\
static ssize_t dsc##i##_show_path(struct class *class,\
			       struct class_attribute *attr, char *buf) {\
	int n = 0;\
	int dsc_id = -1;\
	int ret = -1;\
	struct aml_dvb *dvb = &aml_dvb_device;\
	unsigned long flags;\
	spin_lock_irqsave(&dvb->slock, flags);\
	for (n=0; n<CHAIN_PATH_COUNT; n++) {\
		if (aml_dvb_device.hwdmx[n]) {\
			hwdmx_get_dsc(aml_dvb_device.hwdmx[n],&dsc_id);\
			if (dsc_id == i)\
				break;\
		}\
	}\
	spin_unlock_irqrestore(&dvb->slock, flags);\
	ret = sprintf(buf,"%s%d\n","path=",dsc_id);\
	return ret;\
}

#define CHAIN_PATH_FUNC_DECL(i)\
ssize_t chain_path##i##_store_source(struct class *class,\
			struct class_attribute *attr, const char *buf,\
			size_t size)\
{\
	dmx_source_t src = -1;\
	struct aml_dvb *dvb = aml_get_dvb_device();\
	if (!dvb->hwdmx[i]) {\
		return 0;\
	}\
	if (!strncmp("ts0", buf, 3))\
		src = DMX_SOURCE_FRONT0;\
	else if (!strncmp("ts1", buf, 3))\
		src = DMX_SOURCE_FRONT1;\
	else if (!strncmp("ts2", buf, 3))\
		src = DMX_SOURCE_FRONT2;\
	else if (!strncmp("ts3", buf, 3))\
		src = DMX_SOURCE_FRONT3;\
	else if (!strncmp("hiu", buf, 3))\
		src = DMX_SOURCE_DVR0;\
	else if (!strncmp("dmx0", buf, 4))\
		src = DMX_SOURCE_FRONT0 + 100;\
	else if (!strncmp("dmx1", buf, 4))\
		src = DMX_SOURCE_FRONT1 + 100;\
	else if (!strncmp("dmx2", buf, 4))\
		src = DMX_SOURCE_FRONT2 + 100;\
	if (src != -1) {\
		hwdmx_set_source(dvb->hwdmx[i], src);\
		if (src == DMX_SOURCE_DVR0) {\
			dmx_set_work_mode(&dvb->dmx[0], 0);\
		} else {\
			dmx_set_work_mode(&dvb->dmx[0], 1);\
		}\
	}\
	return size;\
}\
ssize_t chain_path##i##_show_source(struct class *class,\
			       struct class_attribute *attr, char *buf)\
{\
	struct aml_dvb *dvb = aml_get_dvb_device();\
	ssize_t ret = 0;\
	char *src;\
	dmx_source_t dmx_src=0;\
	if (!dvb->hwdmx[i]) {\
		return 0;\
	}\
	if (hwdmx_get_source(dvb->hwdmx[i], &dmx_src) != 0)\
		return 0;\
	switch (dmx_src) {\
	case DMX_SOURCE_FRONT0:\
		src = "ts0";\
		break;\
	case DMX_SOURCE_FRONT1:\
		src = "ts1";\
		break;\
	case DMX_SOURCE_FRONT2:\
		src = "ts2";\
		break;\
	case DMX_SOURCE_FRONT3:\
		src = "ts3";\
		break;\
	case DMX_SOURCE_DVR0:\
		src = "hiu";\
		break;\
	case DMX_SOURCE_FRONT0_OFFSET:\
		src = "dmx0";\
		break;\
	case DMX_SOURCE_FRONT1_OFFSET:\
		src = "dmx1";\
		break;\
	case DMX_SOURCE_FRONT2_OFFSET:\
		src = "dmx2";\
		break;\
	default:\
		src = "disable";\
		break;\
	}\
	ret = sprintf(buf, "%s\n", src);\
	return ret;\
}

#define WORK_MODE_FUNC_DECL(i)\
ssize_t chain_path##i##_store_work_mode(struct class *class,\
			struct class_attribute *attr, const char *buf,\
			size_t size)\
{\
	int mode = -1;\
	struct aml_dvb *dvb = aml_get_dvb_device();\
	if (!dvb->dmx[i].init) {\
		return 0;\
	}\
	if (!strncmp("sw", buf, 2))\
		mode = 0;\
	else if (!strncmp("hw", buf, 2))\
		mode = 1;\
	if (mode != -1)\
		dmx_set_work_mode(&dvb->dmx[i], mode);\
	return size;\
}\
ssize_t chain_path##i##_show_work_mode(struct class *class,\
				   struct class_attribute *attr, char *buf)\
{\
	struct aml_dvb *dvb = aml_get_dvb_device();\
	ssize_t ret = 0;\
	char *src;\
	int work_mode=-1;\
	if (!dvb->dmx[i].init) {\
		return 0;\
	}\
	if (dmx_get_work_mode(&dvb->dmx[i], &work_mode) != 0)\
		return 0;\
	switch (work_mode) {\
	case 0:\
		src = "sw";\
		break;\
	case 1:\
		src = "hw";\
		break;\
	default:\
		src = "disable";\
		break;\
	}\
	ret = sprintf(buf, "%s\n", src);\
	return ret;\
}

#define PATH_BUFF_STATUS_FUNC_DECL(i)\
ssize_t path##i##_set_buf_warning_level(struct class *class,\
			struct class_attribute *attr, const char *buf,\
			size_t size)\
{\
	unsigned long warning_level = 0;\
	struct aml_dvb *dvb = aml_get_dvb_device();\
	int dmx_id = DmxChainPath[i].dmx_id;\
	if (dmx_id == -1)\
		return 0;\
	if (!dvb->dmx[dmx_id].init) {\
		return 0;\
	}\
	if (kstrtoul(buf,10,&warning_level) == 0) {\
		if (warning_level >0 && warning_level <= 100) {\
			dmx_set_buf_warning_level(&dvb->dmx[dmx_id], (int)warning_level);\
		}\
	}\
	return size;\
}\
ssize_t path##i##_show_buf_warning_status(struct class *class,\
				   struct class_attribute *attr, char *buf)\
{\
	struct aml_dvb *dvb = aml_get_dvb_device();\
	ssize_t ret = 0;\
	char *src;\
	int status=-1;\
	int dmx_id = DmxChainPath[i].dmx_id;\
	if (dmx_id == -1)\
		return 0;\
	if (!dvb->dmx[dmx_id].init) {\
		return 0;\
	}\
	if (dmx_get_buf_warning_status(&dvb->dmx[dmx_id], &status) != 0)\
		return 0;\
	switch (status) {\
	case 0:\
		src = "0";\
		break;\
	case 1:\
		src = "1";\
		break;\
	default:\
		src = "disable";\
		break;\
	}\
	ret = sprintf(buf, "%s\n", src);\
	return ret;\
}

DSC_PATH_FUNC_DECL(0)
DSC_PATH_FUNC_DECL(1)

DSC_MODE_FUNC_DECL(0)
DSC_MODE_FUNC_DECL(1)

CHAIN_PATH_FUNC_DECL(0)
CHAIN_PATH_FUNC_DECL(1)

WORK_MODE_FUNC_DECL(0)
WORK_MODE_FUNC_DECL(1)

PATH_BUFF_STATUS_FUNC_DECL(0)
PATH_BUFF_STATUS_FUNC_DECL(1)

static struct class_attribute aml_dvb_class_attrs[] = {
	__ATTR(path0_source, 0664, chain_path0_show_source,
	       chain_path0_store_source),
	__ATTR(path1_source, 0664, chain_path1_show_source,
	       chain_path1_store_source),
	__ATTR(dsc0_mode, 0664, dsc0_show_mode,dsc0_store_mode),
	__ATTR(dsc1_mode, 0664, dsc1_show_mode,dsc1_store_mode),
	__ATTR(dsc0_path, 0664, dsc0_show_path,dsc0_store_path),
	__ATTR(dsc1_path, 0664, dsc1_show_path,dsc1_store_path),
	__ATTR(path0_work_mode, 0664, chain_path0_show_work_mode,\
			chain_path0_store_work_mode),
	__ATTR(path1_work_mode, 0664, chain_path1_show_work_mode,\
			chain_path1_store_work_mode),
	__ATTR(path0_buf_status, 0664, path0_show_buf_warning_status,\
			path0_set_buf_warning_level),
	__ATTR(path1_buf_status, 0664, path1_show_buf_warning_status,\
			path1_set_buf_warning_level),\
	__ATTR(tuner_setting, 0664, stb_show_tuner_setting, stb_store_tuner_setting),
	__ATTR_NULL
};

static struct class aml_dvb_class = {
	.name = "stb",
	.class_attrs = aml_dvb_class_attrs,
};

static void dmx_chain_path_init(void) {
	int i = 0;
	for (i = 0; i < MAX_DMXCHAINPATH_NUM; i++) {
		DmxChainPath[i].ts_id = -1;
		DmxChainPath[i].dmx_id = -1;
		DmxChainPath[i].asyncfifo_id = -1;
	}
}
static int dmx_chain_path_parse(struct platform_device *pdev) {
	int i = 0;
	u32 ts_id = 0;
	u32 dmx_id = 0;
	u32 asyncfifo_id = 0;
	int count = 0;
	char buf[32];
	int ret = 0;
	u32 path_num = 0;

	memset(buf, 0, 32);
	snprintf(buf, sizeof(buf), "path_num");
	ret = of_property_read_u32(pdev->dev.of_node, buf, &path_num);
	if (!ret) {
		pr_inf("%s: 0x%x\n", buf, path_num);
	}
	if (path_num > MAX_DMXCHAINPATH_NUM) {
		return 0;
	}
	for (i = 0; i < path_num; i++) {
		memset(buf, 0, 32);
		snprintf(buf, sizeof(buf), "path%d_ts",i);
		ret = of_property_read_u32(pdev->dev.of_node, buf, &ts_id);
		if (!ret) {
			pr_inf("%s: 0x%x\n", buf, ts_id);
		} else {
			continue;
		}
		memset(buf, 0, 32);
		snprintf(buf, sizeof(buf), "path%d_dmx",i);
		ret = of_property_read_u32(pdev->dev.of_node, buf, &dmx_id);
		if (!ret) {
			pr_inf("%s: 0x%x\n", buf, dmx_id);
		} else {
			continue;
		}

		memset(buf, 0, 32);
		snprintf(buf, sizeof(buf), "path%d_asyncfifo",i);
		ret = of_property_read_u32(pdev->dev.of_node, buf, &asyncfifo_id);
		if (!ret) {
			pr_inf("%s: 0x%x\n", buf, asyncfifo_id);
		} else {
			continue;
		}
		DmxChainPath[count].ts_id = ts_id;
		DmxChainPath[count].dmx_id = dmx_id;
		DmxChainPath[count].asyncfifo_id = asyncfifo_id;
		count++;
	}

	if (count == 0) {
		/*pengcc test for local inject*/
//		DmxChainPath[count].ts_id = DMX_SOURCE_DVR0;
		DmxChainPath[count].ts_id = DMX_SOURCE_FRONT2;
		DmxChainPath[count].dmx_id = 0;
		DmxChainPath[count].asyncfifo_id = 0;
		count++;
	}

	return count;
}
int dmx_get_dev_num(struct platform_device *pdev){
	char buf[32];
	u32 dmxdev = 0;
	int ret = 0;

	memset(buf, 0, 32);
	snprintf(buf, sizeof(buf), "dmxdev_num");
	ret = of_property_read_u32(pdev->dev.of_node, buf, &dmxdev);
	if (!ret)
		pr_inf("%s: 0x%x\n", buf, dmxdev);

	return dmxdev;
}
struct aml_dvb *aml_get_dvb_device(void)
{
	return &aml_dvb_device;
}
EXPORT_SYMBOL(aml_get_dvb_device);

struct dvb_adapter *aml_get_dvb_adapter(void)
{
	return &aml_dvb_device.dvb_adapter;
}
EXPORT_SYMBOL(aml_get_dvb_adapter);

static int aml_dvb_remove(struct platform_device *pdev)
{
	struct aml_dvb *advb;
	int i;
	printk("aml_dvb_remove.....");

	advb = &aml_dvb_device;

	for (i = 0; i < CHAIN_PATH_COUNT; i++) {
		if (advb->hwdmx[i])
			hwdmx_destory(advb->hwdmx[i]);
		if (advb->tsp[i])
			swdmx_ts_parser_free(advb->tsp[i]);
		if (advb->swdsc[i])
			swdmx_descrambler_free(advb->swdsc[i]);
		if (advb->swdmx[i])
			swdmx_demux_free(advb->swdmx[i]);
	}

	for (i = 0; i < DMX_DEV_COUNT; i++) {
		if (advb->dmx[i].init) {
			dmx_destroy(&advb->dmx[i]);
			advb->dmx[i].id = -1;
		}
		dmx_ext_exit(i);
	}

	for (i = 0; i<DSC_DEV_COUNT; i++) {
		dsc_release(&advb->dsc[i]);
		advb->dsc[i].id = -1;
	}

	hwdmx_remove();

	mutex_destroy(&advb->mutex);
	dvb_unregister_adapter(&advb->dvb_adapter);
	class_unregister(&aml_dvb_class);

	return 0;
}

static int hwdmx_cb(char *buf, int count,void *udata) {
	SWDMX_TsParser *tsp = (SWDMX_TsParser *)udata;
	int len = 0;
	struct aml_dvb *advb;

	advb = &aml_dvb_device;

	if (mutex_lock_interruptible(&advb->mutex))
		return 0;
	len = swdmx_ts_parser_run(tsp, buf, count);
	mutex_unlock(&advb->mutex);
//	if (len != count)
//		pr_inf("count is %d,len:%d remain %d\n",count,len,count-len);

	return len;
}

static int aml_dvb_probe(struct platform_device *pdev)
{
	struct aml_dvb *advb;
	int i, ret = 0;
	int dmxChainPathNum = 0;
	int dmxDevNum = 0;

	pr_inf("probe amlogic dvb driver\n");
	advb = &aml_dvb_device;
	memset(advb, 0, sizeof(aml_dvb_device));

	advb->dev = &pdev->dev;
	advb->pdev = pdev;

	ret = dvb_register_adapter(&advb->dvb_adapter, CARD_NAME, THIS_MODULE,
				 advb->dev, adapter_nr);
	if (ret < 0) {
		return ret;
	}

	mutex_init(&advb->mutex);
	spin_lock_init(&advb->slock);

	hwdmx_probe(pdev);

	dmx_chain_path_init();
	dmxChainPathNum = dmx_chain_path_parse(pdev);
	if (dmxChainPathNum == 0) {
		pr_error("invalid chain path\n");
		return -1;
	}

	dmxDevNum = dmx_get_dev_num(pdev);
	if (dmxDevNum == 0)
		dmxDevNum = dmxChainPathNum;

	//create chain path
	for (i = 0; i < dmxChainPathNum; i++) {
		advb->hwdmx[i] = hwdmx_create(DmxChainPath[i].ts_id, DmxChainPath[i].dmx_id,DmxChainPath[i].asyncfifo_id);
		if (advb->hwdmx[i] == NULL) {
			goto INIT_ERR;
		}
		advb->swdsc[i] = swdmx_descrambler_new();
		if (!advb->swdsc[i]) {
			goto INIT_ERR;
		}
		advb->swdmx[i] = swdmx_demux_new();
		if (!advb->swdmx[i]) {
			goto INIT_ERR;
		}
		advb->tsp[i] = swdmx_ts_parser_new();
		if (!advb->tsp[i]) {
			goto INIT_ERR;
		}
		hwdmx_set_cb(advb->hwdmx[i], hwdmx_cb, advb->tsp[i]);
	}

	//create dmx dev
	for (i = 0; i < dmxDevNum; i++) {
		advb->dmx[i].id = i;
		advb->dmx[i].pmutex = &advb->mutex;
		advb->dmx[i].pslock = &advb->slock;
		if (dmxChainPathNum == dmxDevNum) {
			advb->dmx[i].hwdmx = advb->hwdmx[i];
			advb->dmx[i].swdmx = advb->swdmx[i];
			advb->dmx[i].tsp   = advb->tsp[i];
		} else {
			advb->dmx[i].hwdmx = advb->hwdmx[0];
			advb->dmx[i].swdmx = advb->swdmx[0];
			advb->dmx[i].tsp   = advb->tsp[0];
		}
		ret = dmx_init(&advb->dmx[i],&advb->dvb_adapter);
		if (ret) {
			goto INIT_ERR;
		}
		dmx_ext_init(i);
	}

	for (i = 0; i<DSC_DEV_COUNT; i++) {
		advb->dsc[i].mutex = advb->mutex;
		advb->dsc[i].slock = advb->slock;
		advb->dsc[i].id = i;
		if (dmxChainPathNum == DSC_DEV_COUNT) {
			advb->dsc[i].swdsc = advb->swdsc[i];
		} else {
			advb->dsc[i].swdsc = advb->swdsc[0];
		}
		ret = dsc_init(&advb->dsc[i], &advb->dvb_adapter);
		if (ret) {
			goto INIT_ERR;
		}
	}

	for (i = 0; i < dmxChainPathNum; i++) {
		swdmx_ts_parser_add_ts_packet_cb(advb->tsp[i],swdmx_descrambler_ts_packet_cb,
				advb->swdsc[i]);
		swdmx_descrambler_add_ts_packet_cb(advb->swdsc[i],
				swdmx_demux_ts_packet_cb,advb->swdmx[i]);
	}

	if (class_register(&aml_dvb_class) < 0)
		pr_error("register class error\n");

	return 0;

INIT_ERR:
	aml_dvb_remove(pdev);

	return -1;
}
#ifdef CONFIG_OF
static const struct of_device_id aml_dvb_dt_match[] = {
	{
	 .compatible = "amlogic, dvb-swdmx",
	 },
	{},
};
#endif /*CONFIG_OF */


struct platform_driver aml_dvb_driver = {
	.probe = aml_dvb_probe,
	.remove = aml_dvb_remove,
	.suspend = NULL,
	.resume = NULL,
	.driver = {
		   .name = "amlogic-dvb",
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
	   .of_match_table = aml_dvb_dt_match,
#endif
		}
};

static int __init aml_dvb_init(void)
{
	pr_inf("aml dvb init\n");
	return platform_driver_register(&aml_dvb_driver);
}
static void __exit aml_dvb_exit(void)
{
	pr_inf("aml dvb exit\n");
	platform_driver_unregister(&aml_dvb_driver);
}

module_init(aml_dvb_init);
module_exit(aml_dvb_exit);

MODULE_DESCRIPTION("driver for the AMLogic DVB card");
MODULE_AUTHOR("AMLOGIC");
MODULE_LICENSE("GPL");

