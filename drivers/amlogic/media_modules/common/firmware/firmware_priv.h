/*
 * drivers/amlogic/media/common/firmware/firmware.h
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

#ifndef __VIDEO_FIRMWARE_PRIV_HEADER_
#define __VIDEO_FIRMWARE_PRIV_HEADER_
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include "firmware_type.h"

struct firmware_mgr_s {
	struct list_head head;
	spinlock_t lock;
};

struct firmware_info_s {
	struct list_head node;
	char name[32];
	char path[64];
	enum firmware_type_e type;
	struct firmware_s *data;
};

struct ucode_info_s {
	int cpu;
	enum firmware_type_e type;
	const char *name;
};

struct firmware_header_s {
	int magic;
	int checksum;
	char name[32];
	char cpu[16];
	char format[32];
	char version[32];
	char author[32];
	char date[32];
	char commit[16];
	int data_size;
	unsigned int time;
	char reserved[128];
};

struct firmware_s {
	union {
		struct firmware_header_s header;
		char buf[512];
	};
	char data[0];
};

struct package_header_s {
	int magic;
	int size;
	int checksum;
	char reserved[128];
};

struct package_s {
	union {
		struct package_header_s header;
		char buf[256];
	};
	char data[0];
};

struct info_header_s {
	char name[32];
	char format[32];
	char cpu[32];
	int length;
};

struct package_info_s {
	union {
		struct info_header_s header;
		char buf[256];
	};
	char data[0];
};

struct firmware_dev_s {
	struct cdev cdev;
	struct device *dev;
	dev_t dev_no;
};

#endif
