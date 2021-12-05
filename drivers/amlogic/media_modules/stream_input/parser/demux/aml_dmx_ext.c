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
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include "aml_dmx_ext.h"
#include "aml_dvb.h"
#include "aml_dmx.h"
#include "hw_demux/hwdemux.h"
#include "hw_demux/asyncfifo.h"

struct dmx_ext {
	int major_id;
	int id;

	struct mutex mutex;
	wait_queue_head_t queue;
	int update;
	struct dmx_ext_param data;
	int user;
	int used;
};
#define DMX_EXT_NUM 			3
static struct dmx_ext dmx_ext_data[DMX_EXT_NUM];

#define WR_OPEN_FILE 	0
#define RD_OPEN_FILE	1

static int usercopy(struct file *file,
		     unsigned int cmd, unsigned long arg,
		     int (*func)(struct file *file,
		     unsigned int cmd, void *arg))
{
	char    sbuf[128];
	void    *mbuf = NULL;
	void    *parg = NULL;
	int     err  = -EINVAL;

	/*  Copy arguments into temp kernel buffer  */
	switch (_IOC_DIR(cmd)) {
	case _IOC_NONE:
		/*
		 * For this command, the pointer is actually an integer
		 * argument.
		 */
		parg = (void *) arg;
		break;
	case _IOC_READ: /* some v4l ioctls are marked wrong ... */
	case _IOC_WRITE:
	case (_IOC_WRITE | _IOC_READ):
		if (_IOC_SIZE(cmd) <= sizeof(sbuf)) {
			parg = sbuf;
		} else {
			/* too big to allocate from stack */
			mbuf = kmalloc(_IOC_SIZE(cmd), GFP_KERNEL);
			if (NULL == mbuf)
				return -ENOMEM;
			parg = mbuf;
		}

		err = -EFAULT;
		if (copy_from_user(parg, (void __user *)arg, _IOC_SIZE(cmd)))
			goto out;
		break;
	}

	/* call driver */
	if ((err = func(file, cmd, parg)) == -ENOIOCTLCMD)
		err = -ENOTTY;

	if (err < 0)
		goto out;

	/*  Copy results into user buffer  */
	switch (_IOC_DIR(cmd))
	{
	case _IOC_READ:
	case (_IOC_WRITE | _IOC_READ):
		if (copy_to_user((void __user *)arg, parg, _IOC_SIZE(cmd)))
			err = -EFAULT;
		break;
	}

out:
	kfree(mbuf);
	return err;
}

int dmx_ext_open(struct inode *inode, struct file *file) {
	struct dmx_ext *dmx_ext_dev=&dmx_ext_data[iminor(inode)];

	mutex_lock(&dmx_ext_dev->mutex);
	dmx_ext_dev->user++;
	dmx_ext_dev->id = iminor(inode);
	file->private_data = dmx_ext_dev;

	mutex_unlock(&dmx_ext_dev->mutex);

	return 0;
}

int dmx_ext_close(struct inode *inode, struct file *file) {
	struct dmx_ext *dmx_ext_dev = file->private_data;
	mutex_lock(&dmx_ext_dev->mutex);
	dmx_ext_dev->user--;
	if (dmx_ext_dev->user == 0)
		dmx_ext_dev->used = 0;
	mutex_unlock(&dmx_ext_dev->mutex);

	return 0;
}
int dmx_ext_do_ioctl(struct file *file, unsigned int cmd,void *parg) {
	struct dmx_ext *dmx_ext_dev = file->private_data;
	mutex_lock(&dmx_ext_dev->mutex);
	switch (cmd) {
		case DMX_EXT_SET_ASYNCFIFO_PARAMS:
			{
				struct dmx_ext_asyncfifo_param *params = (struct dmx_ext_asyncfifo_param *)parg;

				asyncfifo_set_security_buf(dmx_ext_dev->id, params->addr, params->size);
			}
			break;
	}

	mutex_unlock(&dmx_ext_dev->mutex);

	return 0;
}
long dmx_ext_ioctl(struct file *file, unsigned int cmd,unsigned long arg) {
	return usercopy(file, cmd, arg, dmx_ext_do_ioctl);
}

static unsigned int dmx_ext_poll(struct file *file, poll_table *wait) {
	unsigned int mask = 0;
	struct dmx_ext *dmx_ext_dev = file->private_data;

	poll_wait(file, &dmx_ext_dev->queue, wait);

	if (dmx_ext_dev->update)	{
		mask |= (POLLIN | POLLRDNORM | POLLPRI);
	} else {
		mask |= (POLLIN | POLLRDNORM | POLLPRI | POLLERR);
	}

	return mask;
}

static ssize_t dmx_ext_read(struct file *file, char __user *buf, size_t count,
			    loff_t *ppos){
	int ret = 0;
	struct dmx_ext *dmx_ext_dev = file->private_data;

	if (dmx_ext_dev->update) {
		ret = sizeof(struct dmx_ext_param);
		if (copy_to_user(buf, &(dmx_ext_dev->data), ret))
			return -EFAULT;
		dmx_ext_dev->update = 0;
	}
	return ret;
}
static ssize_t dmx_ext_write(struct file *file, const char __user *buf,
			     size_t count, loff_t *ppos) {
	struct dmx_ext *dmx_ext_dev = file->private_data;
	int ret = 0;
	int id = dmx_ext_dev->id;
	struct aml_dvb *dvb = aml_get_dvb_device();

	if ((file->f_flags & O_ACCMODE) != O_WRONLY)
		return -EINVAL;
	if (!dmx_ext_dev->used) {
		return -ENODEV;
	}
	mutex_lock(&dmx_ext_dev->mutex);
	ret = dmx_write_sw_from_user(&(dvb->dmx[id]), buf, count);
	mutex_unlock(&dmx_ext_dev->mutex);

	return ret;
}
static const struct file_operations device_fops =
{
	.owner = THIS_MODULE,
	.open  = dmx_ext_open,
	.release = dmx_ext_close,
	.read = dmx_ext_read,
	.write = dmx_ext_write,
	.poll = dmx_ext_poll,
	.unlocked_ioctl = dmx_ext_ioctl,
};
static struct class *dmx_ext_class = NULL;

int dmx_ext_init(int id) {
	int result;
	struct device *clsdev;

	memset(&dmx_ext_data[id], 0, sizeof(struct dmx_ext));

	result=register_chrdev(0,"ext",&device_fops);
	if (result<0){
		printk("error:can not register the device\n");
		return -1;
	}
	printk("init_dmx_ext success,id:%d\n",result);
	dmx_ext_data[id].major_id = result;

	if (dmx_ext_class == NULL ) {
		dmx_ext_class = class_create(THIS_MODULE, "dmx_ext");
		if (IS_ERR(dmx_ext_class)) {
			printk("class create dmx_ext_class fail\n");
			return -1;
		}
	}
	clsdev = device_create(dmx_ext_class, NULL,
			       MKDEV(dmx_ext_data[id].major_id, id),
			       &dmx_ext_data[id], "dvb0.ext%d",id);
	if (IS_ERR(clsdev)) {
		printk("device_create dvb0.ext fail\n");
		return PTR_ERR(clsdev);
	}
	mutex_init(&dmx_ext_data[id].mutex);
	dmx_ext_data[id].id = id;
	dmx_ext_data[id].used = 1;

	return 0;
}

void dmx_ext_exit(int id) {
	int i = 0;
	int count = 0;

	if (!dmx_ext_data[id].used)
		return ;
	device_destroy(dmx_ext_class, MKDEV(dmx_ext_data[id].major_id, id));
	unregister_chrdev(dmx_ext_data[id].major_id,"ext");
	dmx_ext_data[id].used = 0;

	for (i = 0; i < DMX_EXT_NUM; i++) {
		if (!dmx_ext_data[id].used)
			count++;
	}
	if (count == DMX_EXT_NUM) {
		class_destroy(dmx_ext_class);
		dmx_ext_class = NULL;
	}
}
int dmx_ext_add_pvr_pid(int id, int pid) {
	int i = 0;
	struct dmx_ext *dmx_ext_dev = &dmx_ext_data[id];

	if (!dmx_ext_dev->used)
		return 0;

	for (i=0; i<dmx_ext_dev->data.pvr_count; i++) {
		if (pid == dmx_ext_dev->data.pvr_pid[i]) {
			return 0;
		}
	}
	if (i == dmx_ext_dev->data.pvr_count && i < PVR_PID_NUM) {
		dmx_ext_dev->data.pvr_pid[i] = pid;
		dmx_ext_dev->data.pvr_count++;
		dmx_ext_dev->update = 1;
		wake_up(&dmx_ext_dev->queue);
	}
	return 0;
}

int dmx_ext_remove_pvr_pid(int id, int pid) {
	int i = 0;
	int k = 0;
	struct dmx_ext *dmx_ext_dev = &dmx_ext_data[id];

	if (!dmx_ext_dev->used)
		return 0;

	for (i = 0; i < dmx_ext_dev->data.pvr_count; i++) {
		if (pid == dmx_ext_dev->data.pvr_pid[i]) {
			break;
		}
	}
	if (i != dmx_ext_dev->data.pvr_count) {
		for (k=i; k < dmx_ext_dev->data.pvr_count; k++) {
			if (k == dmx_ext_dev->data.pvr_count -1) {
				dmx_ext_dev->data.pvr_pid[k] = 0;
			} else {
				dmx_ext_dev->data.pvr_pid[k] = dmx_ext_dev->data.pvr_pid[k+1];
			}
		}
		dmx_ext_dev->data.pvr_count--;
		wake_up(&dmx_ext_dev->queue);
	}

	return 0;
}
