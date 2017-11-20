/*
 * drivers/amlogic/amports/vdec_profile.c
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
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/debugfs.h>

#include <linux/amlogic/media/utils/vdec_reg.h>
#include "vdec_profile.h"
#include "vdec.h"

#define ISA_TIMERE 0x2662
#define ISA_TIMERE_HI 0x2663

#define PROFILE_REC_SIZE 40

static DEFINE_MUTEX(vdec_profile_mutex);
static int rec_wp;
static bool rec_wrapped;

struct dentry *root, *event;

struct vdec_profile_rec_s {
	struct vdec_s *vdec;
	u64 timestamp;
	int event;
	int para1;
	int para2;
};

static struct vdec_profile_rec_s recs[PROFILE_REC_SIZE];
static const char *event_name[VDEC_PROFILE_MAX_EVENT] = {
	"run",
	"cb",
	"save_input",
	"check run ready",
	"run ready",
	"disconnect",
	"dec_work",
	"info"
};

static u64 get_us_time(void)
{
	u32 lo, hi1, hi2;

	do {
		hi1 = READ_MPEG_REG(ISA_TIMERE_HI);
		lo = READ_MPEG_REG(ISA_TIMERE);
		hi2 = READ_MPEG_REG(ISA_TIMERE_HI);
	} while (hi1 != hi2);

	return (((u64)hi1) << 32) | lo;
}

void vdec_profile_more(struct vdec_s *vdec, int event, int para1, int para2)
{
	mutex_lock(&vdec_profile_mutex);

	recs[rec_wp].vdec = vdec;
	recs[rec_wp].timestamp = get_us_time();
	recs[rec_wp].event = event;
	recs[rec_wp].para1 = para1;
	recs[rec_wp].para2 = para2;

	rec_wp++;
	if (rec_wp == PROFILE_REC_SIZE) {
		rec_wrapped = true;
		rec_wp = 0;
	}

	mutex_unlock(&vdec_profile_mutex);
}
EXPORT_SYMBOL(vdec_profile_more);

void vdec_profile(struct vdec_s *vdec, int event)
{
	vdec_profile_more(vdec, event, 0 , 0);
}
EXPORT_SYMBOL(vdec_profile);

void vdec_profile_flush(struct vdec_s *vdec)
{
	int i;

	mutex_lock(&vdec_profile_mutex);

	for (i = 0; i < PROFILE_REC_SIZE; i++) {
		if (recs[i].vdec == vdec)
			recs[i].vdec = NULL;
	}

	mutex_unlock(&vdec_profile_mutex);
}

static const char *event_str(int event)
{
	if (event < VDEC_PROFILE_MAX_EVENT)
		return event_name[event];

	return "INVALID";
}

static int vdec_profile_dbg_show(struct seq_file *m, void *v)
{
	int i, end;
	u64 base_timestamp;

	mutex_lock(&vdec_profile_mutex);

	if (rec_wrapped) {
		i = rec_wp;
		end = rec_wp;
	} else {
		i = 0;
		end = rec_wp;
	}

	base_timestamp = recs[i].timestamp;
	while (1) {
		if ((!rec_wrapped) && (i == end))
			break;

		if (recs[i].vdec) {
			seq_printf(m, "[%s:%d] \t%016llu us : %s (%d,%d)\n",
				vdec_device_name_str(recs[i].vdec),
				recs[i].vdec->id,
				recs[i].timestamp - base_timestamp,
				event_str(recs[i].event),
				recs[i].para1,
				recs[i].para2
				);
		} else {
			seq_printf(m, "[%s:%d] \t%016llu us : %s (%d,%d)\n",
				"N/A",
				0,
				recs[i].timestamp - base_timestamp,
				event_str(recs[i].event),
				recs[i].para1,
				recs[i].para2
				);
		}
		if (++i == PROFILE_REC_SIZE)
			i = 0;

		if (rec_wrapped && (i == end))
			break;
	}

	mutex_unlock(&vdec_profile_mutex);

	return 0;
}

static int vdec_profile_dbg_open(struct inode *inode, struct file *file)
{
	return single_open(file, vdec_profile_dbg_show, NULL);
}

static const struct file_operations event_dbg_fops = {
	.open    = vdec_profile_dbg_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

#if 0 /*DEBUG_TMP*/
static int __init vdec_profile_init_debugfs(void)
{
	struct dentry *root, *event;

	root = debugfs_create_dir("vdec_profile", NULL);
	if (IS_ERR(root) || !root)
		goto err;

	event = debugfs_create_file("event", 0400, root, NULL,
			&event_dbg_fops);
	if (!event)
		goto err_1;

	mutex_init(&vdec_profile_mutex);

	return 0;

err_1:
	debugfs_remove(root);
err:
	pr_err("Can not create debugfs for vdec_profile\n");
	return 0;
}

#endif

int vdec_profile_init_debugfs(void)
{
	root = debugfs_create_dir("vdec_profile", NULL);
	if (IS_ERR(root) || !root)
		goto err;

	event = debugfs_create_file("event", 0400, root, NULL,
			&event_dbg_fops);
	if (!event)
		goto err_1;

	mutex_init(&vdec_profile_mutex);

	return 0;

err_1:
	debugfs_remove(root);
err:
	pr_err("Can not create debugfs for vdec_profile\n");
	return 0;
}
EXPORT_SYMBOL(vdec_profile_init_debugfs);

void vdec_profile_exit_debugfs(void)
{
	debugfs_remove(event);
	debugfs_remove(root);
}
EXPORT_SYMBOL(vdec_profile_exit_debugfs);

/*module_init(vdec_profile_init_debugfs);*/

