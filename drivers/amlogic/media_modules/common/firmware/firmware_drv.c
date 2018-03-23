/*
 * drivers/amlogic/media/common/firmware/firmware.c
 *
 * Copyright (C) 2016 Amlogic, Inc. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <linux/amlogic/media/utils/vformat.h>
#include <linux/amlogic/cpu_version.h>
#include "../../stream_input/amports/amports_priv.h"
#include "../../frame_provider/decoder/utils/vdec.h"
#include "firmware_priv.h"
#include "../chips/chips.h"
#include <linux/string.h>
#include <linux/amlogic/media/utils/log.h>
#include <linux/firmware.h>
#include <linux/amlogic/tee.h>
#include <linux/amlogic/major.h>
#include <linux/cdev.h>
#include <linux/crc32.h>

#define CLASS_NAME	"firmware_codec"
#define DEV_NAME	"firmware_vdec"
#define DIR		"video"
#define FRIMWARE_SIZE	(128 * 1024) /*128k*/
#define BUFF_SIZE	(1024 * 1024 * 2)

#define FW_LOAD_FORCE	(0x1)
#define FW_LOAD_TRY	(0X2)

/*the first 256 bytes are signature data*/
#define SEC_OFFSET	(256)

#define PACK ('P' << 24 | 'A' << 16 | 'C' << 8 | 'K')
#define CODE ('C' << 24 | 'O' << 16 | 'D' << 8 | 'E')

static DEFINE_MUTEX(mutex);

static  struct ucode_info_s ucode_info[] = {
#include "firmware_cfg.h"
};

static const struct file_operations firmware_fops = {
	.owner = THIS_MODULE
};

struct firmware_mgr_s *g_mgr;
struct firmware_dev_s *g_dev;

static u32 debug;

int get_firmware_data(enum firmware_type_e type, char *buf)
{
	int data_len, ret = -1;
	struct firmware_mgr_s *mgr = g_mgr;
	struct firmware_info_s *info;

	if (tee_enabled()) {
		pr_info ("tee load firmware type= %d\n",(u32)type);
		ret = tee_load_video_fw((u32)type, 0);
		if (ret == 0)
			ret = 1;
		else
			ret = -1;
		return ret;
	}

	mutex_lock(&mutex);

	if (list_empty(&mgr->head)) {
		pr_info("the info list is empty.\n");
		goto out;
	}

	list_for_each_entry(info, &mgr->head, node) {
		if (type != info->type)
			continue;

		data_len = info->data->header.data_size;
		memcpy(buf, info->data->data, data_len);
		ret = data_len;

		break;
	}
out:
	mutex_unlock(&mutex);

	return ret;
}
EXPORT_SYMBOL(get_firmware_data);

int get_data_from_name(const char *name, char *buf)
{
	int data_len, ret = -1;
	struct firmware_mgr_s *mgr = g_mgr;
	struct firmware_info_s *info;
	char *firmware_name = __getname();

	if (IS_ERR_OR_NULL(firmware_name))
		return -ENOMEM;

	strcat(firmware_name, name);
	strcat(firmware_name, ".bin");

	mutex_lock(&mutex);

	if (list_empty(&mgr->head)) {
		pr_info("the info list is empty.\n");
		goto out;
	}

	list_for_each_entry(info, &mgr->head, node) {
		if (strcmp(firmware_name, info->name))
			continue;

		data_len = info->data->header.data_size;
		memcpy(buf, info->data->data, data_len);
		ret = data_len;

		break;
	}
out:
	mutex_unlock(&mutex);

	__putname(firmware_name);

	return ret;
}
EXPORT_SYMBOL(get_data_from_name);

static int firmware_probe(char *buf)
{
	int magic = 0;

	memcpy(&magic, buf, sizeof(int));
	return magic;
}

static int request_firmware_from_sys(const char *file_name,
		char *buf, int size)
{
	int ret = -1;
	const struct firmware *firmware;
	int magic, offset = 0;

	pr_info("Try load %s  ...\n", file_name);

	ret = request_firmware(&firmware, file_name, g_dev->dev);
	if (ret < 0) {
		pr_info("Error : %d can't load the %s.\n", ret, file_name);
		goto err;
	}

	if (firmware->size > size) {
		pr_info("Not enough memory size for ucode.\n");
		ret = -ENOMEM;
		goto release;
	}

	magic = firmware_probe((char *)firmware->data);
	if (magic != PACK && magic != CODE) {
		if (firmware->size < SEC_OFFSET) {
			pr_info("This is an invalid firmware file.\n");
			goto release;
		}

		magic = firmware_probe((char *)firmware->data + SEC_OFFSET);
		if (magic != PACK) {
			pr_info("The firmware file is not packet.\n");
			goto release;
		}

		offset = SEC_OFFSET;
	}

	memcpy(buf, (char *)firmware->data + offset, firmware->size - offset);

	pr_info("load firmware size : %zd, Name : %s.\n",
		firmware->size, file_name);
	ret = firmware->size;
release:
	release_firmware(firmware);
err:
	return ret;
}

int request_decoder_firmware_on_sys(enum vformat_e type,
	const char *file_name, char *buf, int size)
{
	int ret;

	ret = get_data_from_name(file_name, buf);
	if (ret < 0)
		pr_info("Get firmware fail.\n");

	if (ret > size) {
		pr_info("Not enough memory.\n");
		return -ENOMEM;
	}

	return ret;
}
int get_decoder_firmware_data(enum vformat_e type,
	const char *file_name, char *buf, int size)
{
	int ret;

	ret = request_decoder_firmware_on_sys(type, file_name, buf, size);
	if (ret < 0)
		pr_info("get_decoder_firmware_data %s for format %d failed!\n",
				file_name, type);

	return ret;
}
EXPORT_SYMBOL(get_decoder_firmware_data);

static unsigned long firmware_mgr_lock(struct firmware_mgr_s *mgr)
{
	unsigned long flags;

	spin_lock_irqsave(&mgr->lock, flags);
	return flags;
}

static void firmware_mgr_unlock(struct firmware_mgr_s *mgr, unsigned long flags)
{
	spin_unlock_irqrestore(&mgr->lock, flags);
}

static void add_info(struct firmware_info_s *info)
{
	unsigned long flags;
	struct firmware_mgr_s *mgr = g_mgr;

	flags = firmware_mgr_lock(mgr);
	list_add(&info->node, &mgr->head);
	firmware_mgr_unlock(mgr, flags);
}

static void del_info(struct firmware_info_s *info)
{
	unsigned long flags;
	struct firmware_mgr_s *mgr = g_mgr;

	flags = firmware_mgr_lock(mgr);
	list_del(&info->node);
	firmware_mgr_unlock(mgr, flags);
}

static void walk_firmware_info(void)
{
	struct firmware_mgr_s *mgr = g_mgr;
	struct firmware_info_s *info;

	mutex_lock(&mutex);

	if (list_empty(&mgr->head)) {
		pr_info("the info list is empty.\n");
		return;
	}

	list_for_each_entry(info, &mgr->head, node) {
		if (IS_ERR_OR_NULL(info->data))
			continue;

		pr_info("path : %s.\n", info->path);
		pr_info("name : %s.\n", info->name);
		pr_info("version : %s.\n",
			info->data->header.version);
		pr_info("checksum : 0x%x.\n",
			info->data->header.checksum);
		pr_info("data size : %d.\n",
			info->data->header.data_size);
		pr_info("author : %s.\n",
			info->data->header.author);
		pr_info("date : %s.\n",
			info->data->header.date);
		pr_info("commit : %s.\n\n",
			info->data->header.commit);
	}

	mutex_unlock(&mutex);
}

static ssize_t info_show(struct class *class,
			struct class_attribute *attr, char *buf)
{
	char *pbuf = buf;
	struct firmware_mgr_s *mgr = g_mgr;
	struct firmware_info_s *info;

	mutex_lock(&mutex);

	if (list_empty(&mgr->head)) {
		pbuf += sprintf(pbuf, "No firmware.\n");
		goto out;
	}

	list_for_each_entry(info, &mgr->head, node) {
		if (IS_ERR_OR_NULL(info->data))
			continue;

		pr_info("%10s : %s\n", "from", info->src_from);
		pr_info("%10s : %s\n", "name", info->name);
		pr_info("%10s : %d\n", "size",
			info->data->header.data_size);
		pr_info("%10s : %s\n", "ver",
			info->data->header.version);
		pr_info("%10s : 0x%x\n", "sum",
			info->data->header.checksum);
		pr_info("%10s : %s\n", "commit",
			info->data->header.commit);
		pr_info("%10s : %s\n", "author",
			info->data->header.author);
		pr_info("%10s : %s\n\n", "date",
			info->data->header.date);
	}
out:
	mutex_unlock(&mutex);

	return pbuf - buf;
}

static int set_firmware_info(void)
{
	int ret = 0, i, len;
	struct firmware_info_s *info;
	int info_size = ARRAY_SIZE(ucode_info);
	int cpu = get_cpu_type();
	char *path = __getname();
	const char *name;

	if (IS_ERR_OR_NULL(path))
		return -ENOMEM;

	for (i = 0; i < info_size; i++) {
		if (cpu != ucode_info[i].cpu)
			continue;

		name = ucode_info[i].name;
		if (IS_ERR_OR_NULL(name))
			break;

		len = snprintf(path, PATH_MAX, "%s/%s", DIR,
			ucode_info[i].name);
		if (len >= PATH_MAX)
			continue;

		info = kzalloc(sizeof(struct firmware_info_s), GFP_KERNEL);
		if (IS_ERR_OR_NULL(info)) {
			__putname(path);
			return -ENOMEM;
		}

		strcpy(info->path, path);
		strcpy(info->name, name);
		strcpy(info->src_from, name);
		info->type = ucode_info[i].type;
		info->data = NULL;

		add_info(info);
	}

	__putname(path);

	return ret;
}

static int checksum(struct firmware_s *firmware)
{
	unsigned int crc;

	crc = crc32_le(~0U, firmware->data, firmware->header.data_size);

	if (debug)
		pr_info("firmware crc result : 0x%x\n", crc ^ ~0U);

	return firmware->header.checksum != (crc ^ ~0U) ? 0 : 1;
}

static int check_repeat(struct firmware_s *data, enum firmware_type_e type)
{
	struct firmware_mgr_s *mgr = g_mgr;
	struct firmware_info_s *info;

	if (list_empty(&mgr->head)) {
		pr_info("the info list is empty.\n");
		return -1;
	}

	if (type == FIRMWARE_MAX)
		return 0;

	list_for_each_entry(info, &mgr->head, node) {
		if (info->type != type)
			continue;

		if (IS_ERR_OR_NULL(info->data))
			info->data = data;

		return 1;
	}

	return 0;
}

static int firmware_parse_package(struct firmware_info_s *fw_info,
	char *buf, int size)
{
	int ret = 0;
	struct package_info_s *pack_info;
	struct firmware_info_s *info;
	struct firmware_s *data;
	char *pack_data;
	int info_len, len;
	int try_cnt = 100;
	char *path = __getname();

	if (IS_ERR_OR_NULL(path))
		return -ENOMEM;

	pack_data = ((struct package_s *)buf)->data;
	pack_info = (struct package_info_s *)pack_data;
	info_len = sizeof(struct package_info_s);

	do {
		if (!pack_info->header.length)
			break;

		len = snprintf(path, PATH_MAX, "%s/%s", DIR,
			pack_info->header.name);
		if (len >= PATH_MAX)
			continue;

		info = kzalloc(sizeof(struct firmware_info_s), GFP_KERNEL);
		if (IS_ERR_OR_NULL(info)) {
			ret = -ENOMEM;
			goto out;
		}

		data = kzalloc(FRIMWARE_SIZE, GFP_KERNEL);
		if (IS_ERR_OR_NULL(data)) {
			kfree(info);
			ret = -ENOMEM;
			goto out;
		}

		strcpy(info->path, path);
		strcpy(info->name, pack_info->header.name);
		strcpy(info->src_from, fw_info->src_from);
		info->type = get_firmware_type(pack_info->header.format);

		len = pack_info->header.length;
		memcpy(data, pack_info->data, len);

		pack_data += (pack_info->header.length + info_len);
		pack_info = (struct package_info_s *)pack_data;

		ret = checksum(data);
		if (!ret) {
			pr_info("check sum fail !\n");
			kfree(data);
			kfree(info);
			goto out;
		}

		ret = check_repeat(data, info->type);
		if (ret < 0) {
			kfree(data);
			kfree(info);
			goto out;
		}

		if (ret) {
			kfree(info);
			continue;
		}

		info->data = data;
		add_info(info);
	} while (try_cnt--);
out:
	__putname(path);

	return ret;
}

static int firmware_parse_code(struct firmware_info_s *info,
	char *buf, int size)
{
	if (!IS_ERR_OR_NULL(info->data))
		kfree(info->data);

	info->data = kzalloc(FRIMWARE_SIZE, GFP_KERNEL);
	if (IS_ERR_OR_NULL(info->data))
		return -ENOMEM;

	memcpy(info->data, buf, size);

	if (!checksum(info->data)) {
		pr_info("check sum fail !\n");
		kfree(info->data);
		return -1;
	}

	return 0;
}

static int get_firmware_from_sys(const char *path,
	char *buf, int size)
{
	int len = 0;

	len = request_firmware_from_sys(path, buf, size);
	if (len < 0)
		pr_info("get data from fsys fail.\n");

	return len;
}

static int set_firmware_data(void)
{
	int ret = 0, magic = 0;
	struct firmware_mgr_s *mgr = g_mgr;
	struct firmware_info_s *info, *temp;
	char *buf = NULL;
	int size;

	if (list_empty(&mgr->head)) {
		pr_info("the info list is empty.\n");
		return 0;
	}

	buf = vmalloc(BUFF_SIZE);
	if (IS_ERR_OR_NULL(buf))
		return -ENOMEM;

	memset(buf, 0, BUFF_SIZE);

	list_for_each_entry_safe(info, temp, &mgr->head, node) {
		size = get_firmware_from_sys(info->path, buf, BUFF_SIZE);
		magic = firmware_probe(buf);

		switch (magic) {
		case PACK:
			ret = firmware_parse_package(info, buf, size);

			del_info(info);
			kfree(info);
			break;

		case CODE:
			ret = firmware_parse_code(info, buf, size);
			break;

		default:
			del_info(info);
			kfree(info);
			pr_info("invaild type.\n");
		}

		memset(buf, 0, BUFF_SIZE);
	}

	if (debug)
		walk_firmware_info();

	vfree(buf);

	return ret;
}

static int firmware_pre_load(void)
{
	int ret = -1;

	ret = set_firmware_info();
	if (ret < 0) {
		pr_info("Get path fail.\n");
		goto err;
	}

	ret = set_firmware_data();
	if (ret < 0) {
		pr_info("Set data fail.\n");
		goto err;
	}
err:
	return ret;
}

static int firmware_mgr_init(void)
{
	g_mgr = kzalloc(sizeof(struct firmware_mgr_s), GFP_KERNEL);
	if (IS_ERR_OR_NULL(g_mgr))
		return -ENOMEM;

	INIT_LIST_HEAD(&g_mgr->head);
	spin_lock_init(&g_mgr->lock);

	return 0;
}

static ssize_t reload_show(struct class *class,
			struct class_attribute *attr, char *buf)
{
	char *pbuf = buf;

	pbuf += sprintf(pbuf, "The fw reload usage.\n");
	pbuf += sprintf(pbuf, "> set 1 means that the fw is forced to update\n");
	pbuf += sprintf(pbuf, "> set 2 means that the fw is try to reload\n");

	return pbuf - buf;
}

int firmware_reload(int mode)
{
	int ret = 0;
	struct firmware_mgr_s *mgr = g_mgr;
	struct firmware_info_s *info = NULL;

	if (tee_enabled())
		return 0;

	mutex_lock(&mutex);

	if (mode & FW_LOAD_FORCE) {
		while (!list_empty(&mgr->head)) {
			info = list_entry(mgr->head.next,
				struct firmware_info_s, node);
			list_del(&info->node);
			kfree(info->data);
			kfree(info);
		}

		ret = firmware_pre_load();
		if (ret < 0)
			pr_err("The fw reload fail.\n");
	} else if (mode & FW_LOAD_TRY) {
		if (!list_empty(&mgr->head)) {
			pr_info("The fw has been loaded.\n");
			goto out;
		}

		ret = firmware_pre_load();
		if (ret < 0)
			pr_err("The fw try to reload fail.\n");
	}
out:
	mutex_unlock(&mutex);

	return ret;
}
EXPORT_SYMBOL(firmware_reload);

static ssize_t reload_store(struct class *class,
		struct class_attribute *attr,
		const char *buf, size_t size)
{
	int ret = -1;
	unsigned int val;

	ret = kstrtoint(buf, 0, &val);
	if (ret != 0)
		return -EINVAL;

	ret = firmware_reload(val);
	if (ret < 0)
		pr_err("fw reload fail.\n");

	return size;
}

static struct class_attribute firmware_class_attrs[] = {
	__ATTR_RO(info),
	__ATTR(reload, 0664, reload_show, reload_store),
	__ATTR_NULL
};

static struct class firmware_class = {
	.name = CLASS_NAME,
	.class_attrs = firmware_class_attrs,
};

static int firmware_driver_init(void)
{
	int ret = -1;

	g_dev = kzalloc(sizeof(struct firmware_dev_s), GFP_KERNEL);
	if (IS_ERR_OR_NULL(g_dev))
		return -ENOMEM;

	g_dev->dev_no = MKDEV(AMSTREAM_MAJOR, 100);

	ret = register_chrdev_region(g_dev->dev_no, 1, DEV_NAME);
	if (ret < 0) {
		pr_info("Can't get major number %d.\n", AMSTREAM_MAJOR);
		goto err;
	}

	cdev_init(&g_dev->cdev, &firmware_fops);
	g_dev->cdev.owner = THIS_MODULE;

	ret = cdev_add(&g_dev->cdev, g_dev->dev_no, 1);
	if (ret) {
		pr_info("Error %d adding cdev fail.\n", ret);
		goto err;
	}

	ret = class_register(&firmware_class);
	if (ret < 0) {
		pr_info("Failed in creating class.\n");
		goto err;
	}

	g_dev->dev = device_create(&firmware_class, NULL,
		g_dev->dev_no, NULL, DEV_NAME);
	if (IS_ERR_OR_NULL(g_dev->dev)) {
		pr_info("Create device failed.\n");
		ret = -ENODEV;
		goto err;
	}

	pr_info("Registered firmware driver success.\n");
err:
	return ret;
}

static void firmware_info_clean(void)
{
	struct firmware_mgr_s *mgr = g_mgr;
	struct firmware_info_s *info;
	unsigned long flags;

	flags = firmware_mgr_lock(mgr);
	while (!list_empty(&mgr->head)) {
		info = list_entry(mgr->head.next,
			struct firmware_info_s, node);
		list_del(&info->node);
		kfree(info->data);
		kfree(info);
	}
	firmware_mgr_unlock(mgr, flags);

	kfree(g_mgr);
}

static void firmware_driver_exit(void)
{
	cdev_del(&g_dev->cdev);
	device_destroy(&firmware_class, g_dev->dev_no);
	class_unregister(&firmware_class);
	unregister_chrdev_region(g_dev->dev_no, 1);
	kfree(g_dev);
}

static int __init firmware_module_init(void)
{
	int ret = -1;

	ret = firmware_driver_init();
	if (ret) {
		pr_info("Error %d firmware driver init fail.\n", ret);
		goto err;
	}

	ret = firmware_mgr_init();
	if (ret) {
		pr_info("Error %d firmware mgr init fail.\n", ret);
		goto err;
	}

	ret = firmware_pre_load();
	if (ret) {
		pr_info("Error %d firmware pre load fail.\n", ret);
		goto err;
	}
err:
	return ret;
}

static void __exit firmware_module_exit(void)
{
	firmware_info_clean();
	firmware_driver_exit();
	pr_info("Firmware driver cleaned up.\n");
}

module_param(debug, uint, 0664);

module_init(firmware_module_init);
module_exit(firmware_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nanxin Qin <nanxin.qin@amlogic.com>");
