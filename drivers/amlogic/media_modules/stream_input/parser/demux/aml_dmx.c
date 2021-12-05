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
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/poll.h>
#include <linux/crc32.h>
#include <asm/uaccess.h>
#include <asm/div64.h>

//#include "aml_dvb.h"
#include "aml_dmx.h"
#include "aml_dmx_ext.h"
//#include "hwdemux.h"

#define pr_error(fmt, args...) printk("DVB: " fmt, ## args)
#define pr_inf(fmt, args...)   printk(KERN_DEBUG fmt, ## args)

#define MAX_SEC_FEED_NUM 20
#define MAX_TS_FEED_NUM 20
#define MAX_FILTER_PER_SEC_FEED 8


#define DMX_STATE_FREE      0
#define DMX_STATE_ALLOCATED 1
#define DMX_STATE_SET       2
#define DMX_STATE_READY     3
#define DMX_STATE_GO        4

#define SWDMX_MAX_PID 0x1fff

#define DMX_TYPE_TS  0
#define DMX_TYPE_SEC 1
#define DMX_TYPE_PES 2

static int _dmx_write(struct aml_dmx *pdmx, const u8 *buf, size_t count);


static inline void _invert_mode(struct dmx_section_filter *filter)
{
	int i;

	for (i = 0; i < DMX_FILTER_SIZE; i++)
		filter->filter_mode[i] ^= 0xff;
}

static int _dmx_open(struct dmx_demux *demux)
{
	struct aml_dmx *pdmx = (struct aml_dmx *)demux->priv;

	if (pdmx->users >= MAX_SW_DEMUX_USERS)
		return -EUSERS;

	pdmx->users++;
	return 0;
}

static int _dmx_close(struct dmx_demux *demux)
{
	struct aml_dmx *pdmx = (struct aml_dmx *)demux->priv;

	if (pdmx->users == 0)
		return -ENODEV;

	pdmx->users--;

	if (pdmx->users == 0) {
		if (pdmx->used_hwdmx)
			hwdmx_inject_destroy(pdmx->hwdmx);
	}
	//FIXME: release any unneeded resources if users==0
	return 0;
}

static int _dmx_write_from_user(struct dmx_demux *demux, const char __user *buf, size_t count)
{
	struct aml_dmx *pdmx = (struct aml_dmx *)demux->priv;
	void *p;
	int ret = 0;

	if (pdmx->used_hwdmx) {
		ret = hwdmx_inject(pdmx->hwdmx, buf, count);
		return ret;
	}
//	pr_inf("_dmx_write_from_user\n");
	p = memdup_user(buf, count);
	if (IS_ERR(p)) {
		pr_error("get fail mem pointer\n");
		return PTR_ERR(p);
	}
	ret = _dmx_write(pdmx, p, count);

	kfree(p);

	if (signal_pending(current))
		return -EINTR;
	return ret;
}

static struct sw_demux_sec_feed *_dmx_section_feed_alloc(struct aml_dmx *demux)
{
	int i;

	for (i = 0; i < demux->sec_feed_num; i++)
		if (demux->section_feed[i].state == DMX_STATE_FREE)
			break;

	if (i == demux->sec_feed_num)
		return NULL;

	demux->section_feed[i].state = DMX_STATE_ALLOCATED;

	return &demux->section_feed[i];
}

static struct sw_demux_ts_feed *_dmx_ts_feed_alloc(struct aml_dmx *demux)
{
	int i;

	for (i = 0; i < demux->ts_feed_num; i++)
		if (demux->ts_feed[i].state == DMX_STATE_FREE)
			break;

	if (i == demux->ts_feed_num)
		return NULL;

	demux->ts_feed[i].state = DMX_STATE_ALLOCATED;

	return &demux->ts_feed[i];
}

static struct sw_demux_sec_filter *_dmx_dmx_sec_filter_alloc(struct sw_demux_sec_feed *sec_feed)
{
	int i;

	for (i = 0; i < MAX_FILTER_PER_SEC_FEED; i++)
		if (sec_feed->filter[i].state == DMX_STATE_FREE)
			break;

	if (i == MAX_FILTER_PER_SEC_FEED)
		return NULL;

	sec_feed->filter[i].state = DMX_STATE_ALLOCATED;

	return &sec_feed->filter[i];
}
#if 0
static void prdump(const char* m, const void* data, uint32_t len) {
	if (m)
		pr_error("%s:\n", m);
	if (data) {
		size_t i = 0;
		const unsigned char *c __attribute__((unused)) = data;
		while (len >= 16) {
			pr_error("%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n",
					c[i], c[i+1], c[i+2], c[i+3], c[i+4], c[i+5], c[i+6], c[i+7],
					c[i+8], c[i+9], c[i+10], c[i+11], c[i+12], c[i+13], c[i+14], c[i+15]);
			len -= 16;
			i += 16;
		}
		while (len >= 8) {
			pr_error("%02x %02x %02x %02x %02x %02x %02x %02x\n",
					c[i], c[i+1], c[i+2], c[i+3], c[i+4], c[i+5], c[i+6], c[i+7]);
			len -= 8;
			i += 8;
		}
		while (len >= 4) {
			printk("%02x %02x %02x %02x\n",
					c[i], c[i+1], c[i+2], c[i+3]);
			len -= 4;
			i += 4;
		}
		while (len >= 1) {
			pr_error("%02x ", c[i]);
			len -= 1;
			i += 1;
		}
	}
}
#endif
static void _ts_pkt_cb (SWDMX_TsPacket *pkt, SWDMX_Ptr data)
{
//	prdump("ts_data", pkt->packet, 32);

	struct dmx_ts_feed * source_feed = (struct dmx_ts_feed *)data;
	struct sw_demux_ts_feed *ts_feed = (struct sw_demux_ts_feed *)data;

	if (ts_feed->state != DMX_STATE_GO) {
		return ;
	}
	if (ts_feed->ts_cb) {
		ts_feed->ts_cb(pkt->packet,pkt->packet_len,NULL,0,source_feed);
	}
}
static void _sec_cb (SWDMX_UInt8 *sec, SWDMX_Int len, SWDMX_Ptr data)
{
	struct dmx_section_filter *source_filter = (struct dmx_section_filter *)data;
	struct sw_demux_sec_feed *sec_feed = (struct sw_demux_sec_feed *)source_filter->parent;

//	prdump("sec_data", sec, 32);

	if (sec_feed->state != DMX_STATE_GO) {
		return ;
	}

	if (sec_feed->sec_cb) {
		sec_feed->sec_cb(sec,len,NULL, 0,source_filter);
	}
}

static int _dmx_ts_feed_set(struct dmx_ts_feed *ts_feed, u16 pid, int ts_type,
			   enum dmx_ts_pes pes_type,
			   size_t circular_buffer_size, ktime_t timeout)
{
	struct sw_demux_ts_feed *feed = (struct sw_demux_ts_feed *)ts_feed;
	struct aml_dmx *demux = (struct aml_dmx *)ts_feed->parent->priv;

	pr_inf("_dmx_ts_feed_set pid:0x%0x\n",pid);

	if (pid >= SWDMX_MAX_PID)
		return -EINVAL;

	if (mutex_lock_interruptible(demux->pmutex))
		return -ERESTARTSYS;

	if (ts_type & TS_DECODER) {
		if (pes_type >= DMX_PES_OTHER) {
			mutex_unlock(demux->pmutex);
			return -EINVAL;
		}
	}

	feed->pid = pid;
	feed->ts_type = ts_type;
//	feed->type = DMX_TYPE_PES; //ts_type;
	feed->pes_type = pes_type;
	feed->state = DMX_STATE_READY;

#if 0
	feed->buffer_size = circular_buffer_size;

	if (feed->buffer_size) {
		feed->buffer = vmalloc(feed->buffer_size);
		if (!feed->buffer) {
			mutex_unlock(&demux->mutex);
			return -ENOMEM;
		}
	}
#endif

	/*enable hw pid filter*/
	if (demux->used_hwdmx)
		hwdmx_set_pid(feed->tschan,feed->type, feed->pes_type, feed->pid);

	mutex_unlock(demux->pmutex);

	return 0;
}

static int _dmx_ts_feed_start_filtering(struct dmx_ts_feed *ts_feed)
{
	struct sw_demux_ts_feed *feed = (struct sw_demux_ts_feed *)ts_feed;
	struct aml_dmx *demux = (struct aml_dmx *)ts_feed->parent->priv;
	SWDMX_TsFilterParams  tsfp;

	pr_inf("_dmx_ts_feed_start_filtering\n");
	if (mutex_lock_interruptible(demux->pmutex)) {

		pr_error("%s line:%d\n",__func__,__LINE__);
		return -ERESTARTSYS;
	}

	if (feed->state != DMX_STATE_READY || feed->type != DMX_TYPE_TS) {
		mutex_unlock(demux->pmutex);
		return -EINVAL;
	}

	tsfp.pid = feed->pid;
	swdmx_ts_filter_set_params(feed->tsf, &tsfp);
	swdmx_ts_filter_add_ts_packet_cb(feed->tsf, _ts_pkt_cb, ts_feed);

	if (swdmx_ts_filter_enable(feed->tsf) != SWDMX_OK) {
		mutex_unlock(demux->pmutex);
		return -EINVAL;
	}

	if (feed->type == TS_PACKET) {
		dmx_ext_add_pvr_pid(demux->id, feed->pid);
	}
	spin_lock_irq(demux->pslock);
	ts_feed->is_filtering = 1;
	feed->state = DMX_STATE_GO;
	spin_unlock_irq(demux->pslock);
	mutex_unlock(demux->pmutex);

	return 0;
}

static int _dmx_ts_feed_stop_filtering(struct dmx_ts_feed *ts_feed)
{
	struct sw_demux_ts_feed *feed = (struct sw_demux_ts_feed *)ts_feed;
	struct aml_dmx *demux = (struct aml_dmx *)ts_feed->parent->priv;

	pr_inf("_dmx_ts_feed_stop_filtering \n");
	mutex_lock(demux->pmutex);

	if (feed->state < DMX_STATE_GO) {
		mutex_unlock(demux->pmutex);
		return -EINVAL;
	}

	if (swdmx_ts_filter_disable(feed->tsf) != SWDMX_OK) {
		mutex_unlock(demux->pmutex);
		return -EINVAL;
	}
	if (feed->type == TS_PACKET) {
		dmx_ext_remove_pvr_pid(demux->id, feed->pid);
	}

	spin_lock_irq(demux->pslock);
	ts_feed->is_filtering = 0;
	feed->state = DMX_STATE_ALLOCATED;
	spin_unlock_irq(demux->pslock);
	mutex_unlock(demux->pmutex);

	return 0;
}
static int _dmx_section_feed_allocate_filter(struct dmx_section_feed *feed,
					    struct dmx_section_filter **filter)
{
	struct aml_dmx *demux = (struct aml_dmx *)feed->parent->priv;
	struct sw_demux_sec_filter *sec_filter;

	pr_inf("_dmx_section_feed_allocate_filter \n");

	if (mutex_lock_interruptible(demux->pmutex))
		return -ERESTARTSYS;

	sec_filter = _dmx_dmx_sec_filter_alloc((struct sw_demux_sec_feed *)feed);
	if (!sec_filter) {
		mutex_unlock(demux->pmutex);
		return -EBUSY;
	}
	sec_filter->secf = swdmx_demux_alloc_sec_filter(demux->swdmx);
	if (!sec_filter->secf) {
		sec_filter->state = DMX_STATE_FREE;
		mutex_unlock(demux->pmutex);
		return -EBUSY;
	}
	spin_lock_irq(demux->pslock);
	*filter = &sec_filter->section_filter;
	(*filter)->parent = feed;
//	(*filter)->priv = sec_filter;
	spin_unlock_irq(demux->pslock);
	mutex_unlock(demux->pmutex);
	return 0;
}

static int _dmx_section_feed_set(struct dmx_section_feed *feed,
				u16 pid, size_t circular_buffer_size,
				int check_crc)
{
	struct sw_demux_sec_feed *sec_feed = (struct sw_demux_sec_feed *)feed;
	struct aml_dmx *demux = (struct aml_dmx *)feed->parent->priv;

	pr_inf("_dmx_section_feed_set \n");

	if (pid >= SWDMX_MAX_PID)
		return -EINVAL;

	if (mutex_lock_interruptible(demux->pmutex))
		return -ERESTARTSYS;

	sec_feed->pid = pid;
	sec_feed->check_crc = check_crc;
	sec_feed->type = DMX_TYPE_SEC;
	sec_feed->state = DMX_STATE_READY;

	/*enable hw pid filter*/
	if (demux->used_hwdmx)
		hwdmx_set_pid(sec_feed->secchan,sec_feed->type, 0, sec_feed->pid);

	mutex_unlock(demux->pmutex);

	return 0;
}
static int _dmx_section_feed_start_filtering(struct dmx_section_feed *feed)
{
	struct sw_demux_sec_feed *sec_feed = (struct sw_demux_sec_feed *)feed;
	struct aml_dmx *demux = (struct aml_dmx *)feed->parent->priv;
	SWDMX_SecFilterParams params;
	int i = 0;
	int start_flag = 0;

	pr_inf("_dmx_section_feed_start_filtering\n");
	if (mutex_lock_interruptible(demux->pmutex))
		return -ERESTARTSYS;

	if (feed->is_filtering) {
		mutex_unlock(demux->pmutex);
		return -EBUSY;
	}
	for (i=0; i<MAX_FILTER_PER_SEC_FEED; i++) {
		if (sec_feed->filter[i].state == DMX_STATE_ALLOCATED) {
			params.pid = sec_feed->pid;
			params.crc32 = sec_feed->check_crc;

			memcpy(&sec_feed->filter[i].section_filter.filter_value[1],&sec_feed->filter[i].section_filter.filter_value[3],SWDMX_SEC_FILTER_LEN-1);
			memcpy(&sec_feed->filter[i].section_filter.filter_mask[1],&sec_feed->filter[i].section_filter.filter_mask[3],SWDMX_SEC_FILTER_LEN-1);
			memcpy(&sec_feed->filter[i].section_filter.filter_mode[1],&sec_feed->filter[i].section_filter.filter_mode[3],SWDMX_SEC_FILTER_LEN-1);
			_invert_mode(&sec_feed->filter[i].section_filter);

			memcpy(params.value,sec_feed->filter[i].section_filter.filter_value,SWDMX_SEC_FILTER_LEN);
			memcpy(params.mask,sec_feed->filter[i].section_filter.filter_mask,SWDMX_SEC_FILTER_LEN);
			memcpy(params.mode,sec_feed->filter[i].section_filter.filter_mode,SWDMX_SEC_FILTER_LEN);

			if (swdmx_sec_filter_set_params(sec_feed->filter[i].secf,&params) != SWDMX_OK) {
				continue;
			}

			swdmx_sec_filter_add_section_cb(sec_feed->filter[i].secf, _sec_cb, &sec_feed->filter[i].section_filter);

			if (swdmx_sec_filter_enable(sec_feed->filter[i].secf) != SWDMX_OK) {
				continue;
			}
			sec_feed->filter[i].state = DMX_STATE_GO;
			start_flag = 1;
		}
		else if (sec_feed->filter[i].state == DMX_STATE_READY) {
			if (swdmx_sec_filter_enable(sec_feed->filter[i].secf) != SWDMX_OK) {
				continue;
			}
			sec_feed->filter[i].state = DMX_STATE_GO;
			start_flag = 1;
		}
	}
	if (start_flag != 1) {
		pr_error("%s fail \n",__FUNCTION__);
		return -1;
	}
	sec_feed->state = DMX_STATE_GO;

	spin_lock_irq(demux->pslock);
	feed->is_filtering = 1;
	spin_unlock_irq(demux->pslock);
	mutex_unlock(demux->pmutex);

	return 0;
}

static int _dmx_section_feed_stop_filtering(struct dmx_section_feed *feed)
{
	struct sw_demux_sec_feed *sec_feed = (struct sw_demux_sec_feed *)feed;
	struct aml_dmx *demux = (struct aml_dmx *)feed->parent->priv;
	int i = 0;
	int start_flag = 0;

	pr_inf("_dmx_section_feed_stop_filtering \n");

	if (mutex_lock_interruptible(demux->pmutex))
		return -ERESTARTSYS;

	if (feed->is_filtering == 0) {
		mutex_unlock(demux->pmutex);
		return -EINVAL;
	}
	for (i=0; i<MAX_FILTER_PER_SEC_FEED; i++) {
		if (sec_feed->filter[i].state == DMX_STATE_GO) {
			if (swdmx_sec_filter_disable(sec_feed->filter[i].secf) != SWDMX_OK) {
				continue;
			}
			sec_feed->filter[i].state = DMX_STATE_READY;
			start_flag = 1;
		}
	}
	if (start_flag != 1) {
		pr_error("%s no found start filter \n",__FUNCTION__);
		mutex_unlock(demux->pmutex);
		return 0;
	}

	spin_lock_irq(demux->pslock);
	feed->is_filtering = 0;
	spin_unlock_irq(demux->pslock);
	mutex_unlock(demux->pmutex);

	return 0;
}

static int _dmx_section_feed_release_filter(struct dmx_section_feed *feed,
					   struct dmx_section_filter *filter)
{
	struct sw_demux_sec_feed *sec_feed = (struct sw_demux_sec_feed *)feed;
	struct aml_dmx *demux = (struct aml_dmx *)feed->parent->priv;
	int i = 0;

	pr_inf("_dmx_section_feed_release_filter\n");

	if (mutex_lock_interruptible(demux->pmutex))
		return -ERESTARTSYS;

	if (sec_feed->type != DMX_TYPE_SEC) {
		mutex_unlock(demux->pmutex);
		return -EINVAL;
	}
	for (i=0; i<MAX_FILTER_PER_SEC_FEED; i++) {
		if (sec_feed->filter[i].state != DMX_STATE_FREE && (&sec_feed->filter[i].section_filter) == filter) {
			swdmx_sec_filter_free(sec_feed->filter[i].secf);
			sec_feed->filter[i].secf = NULL;
			memset(filter,0,sizeof(struct dmx_section_filter));
			sec_feed->filter[i].state = DMX_STATE_FREE;
			break;
		}
	}

	mutex_unlock(demux->pmutex);

	return 0;
}

static int _dmx_allocate_ts_feed(struct dmx_demux *dmx,
				   struct dmx_ts_feed **ts_feed,
				   dmx_ts_cb callback)
{
	struct aml_dmx *demux = (struct aml_dmx *)dmx->priv;
	struct sw_demux_ts_feed *feed;

	pr_inf("_dmx_allocate_ts_feed line:%d\n",__LINE__);
	if (mutex_lock_interruptible(demux->pmutex))
		return -ERESTARTSYS;

	if (!(feed = _dmx_ts_feed_alloc(demux))) {
		mutex_unlock(demux->pmutex);
		pr_error("_dmx_allocate_ts_feed line:%d\n",__LINE__);
		return -EBUSY;
	}

	feed->type = DMX_TYPE_TS;
	feed->ts_cb = callback;

	(*ts_feed) = &feed->ts_feed;
	(*ts_feed)->parent = dmx;
//	(*ts_feed)->priv = feed;
	(*ts_feed)->is_filtering = 0;
	(*ts_feed)->start_filtering = _dmx_ts_feed_start_filtering;
	(*ts_feed)->stop_filtering = _dmx_ts_feed_stop_filtering;
	(*ts_feed)->set = _dmx_ts_feed_set;

	feed->tsf = swdmx_demux_alloc_ts_filter(demux->swdmx);
	if (!feed->tsf)
	{
		pr_error("_dmx_allocate_ts_feed line:%d\n",__LINE__);
		feed->state = DMX_STATE_FREE;
		mutex_unlock(demux->pmutex);
		return ERESTARTSYS;
	}
	if (demux->used_hwdmx) {
		feed->tschan = hwdmx_alloc_chan(demux->hwdmx);
		if (!feed->tschan) {
			pr_error("hwdmx_alloc_chan fail line:%d\n",__LINE__);
			swdmx_ts_filter_free(feed->tsf);
			feed->state = DMX_STATE_FREE;
			mutex_unlock(demux->pmutex);
			return ERESTARTSYS;
		}
	}
	mutex_unlock(demux->pmutex);

	return 0;
}

static int _dmx_release_ts_feed(struct dmx_demux *dmx,
				  struct dmx_ts_feed *ts_feed)
{
	struct aml_dmx *demux = (struct aml_dmx *)dmx->priv;
	struct sw_demux_ts_feed *feed;

	pr_inf(" _dmx_release_ts_feed\n");
	if (!ts_feed) {
		return 0;
	}
	if (mutex_lock_interruptible(demux->pmutex))
		return -ERESTARTSYS;

	feed = (struct sw_demux_ts_feed *)ts_feed;

	if (demux->used_hwdmx)
		hwdmx_free_chan(feed->tschan);
	swdmx_ts_filter_free(feed->tsf);
	feed->state = DMX_STATE_FREE;

	mutex_unlock(demux->pmutex);
	return 0;
}

static int _dmx_allocate_section_feed(struct dmx_demux *dmx,
					struct dmx_section_feed **feed,
					dmx_section_cb callback)
{
	struct aml_dmx *demux = (struct aml_dmx *)dmx->priv;
	struct sw_demux_sec_feed *sec_feed;
	int i;

	pr_inf("_dmx_allocate_section_feed \n");
	if (mutex_lock_interruptible(demux->pmutex))
		return -ERESTARTSYS;

	if (!(sec_feed = _dmx_section_feed_alloc(demux))) {
		mutex_unlock(demux->pmutex);
		return -EBUSY;
	}

	sec_feed->sec_filter_num = MAX_FILTER_PER_SEC_FEED;
	sec_feed->filter = vmalloc(sizeof(struct sw_demux_sec_filter) * sec_feed->sec_filter_num);
	if (!sec_feed->filter) {
		mutex_unlock(demux->pmutex);
		return -EBUSY;
	}
	for (i=0; i<sec_feed->sec_filter_num; i++) {
		sec_feed->filter[i].state = DMX_STATE_FREE;
	}
	sec_feed->sec_cb = callback;
	sec_feed->type = DMX_TYPE_SEC;

	(*feed) = &sec_feed->sec_feed;
	(*feed)->parent = dmx;
//	(*feed)->priv = sec_feed;
	(*feed)->is_filtering = 0;

	(*feed)->set = _dmx_section_feed_set;
	(*feed)->allocate_filter = _dmx_section_feed_allocate_filter;
	(*feed)->start_filtering = _dmx_section_feed_start_filtering;
	(*feed)->stop_filtering = _dmx_section_feed_stop_filtering;
	(*feed)->release_filter = _dmx_section_feed_release_filter;

	if (demux->used_hwdmx) {
		sec_feed->secchan = hwdmx_alloc_chan(demux->hwdmx);
		if (!sec_feed->secchan) {
			pr_error("%s error\n",__FUNCTION__);
		}
	}
	mutex_unlock(demux->pmutex);
	return 0;
}

static int _dmx_release_section_feed(struct dmx_demux *dmx,
				       struct dmx_section_feed *feed)
{
	struct aml_dmx *demux = (struct aml_dmx *)dmx->priv;
	struct sw_demux_sec_feed *sec_feed;

	pr_inf(" _dmx_release_section_feed\n");
	if (mutex_lock_interruptible(demux->pmutex))
		return -ERESTARTSYS;

	sec_feed = (struct sw_demux_sec_feed *)feed;
	sec_feed->state = DMX_STATE_FREE;

	if (demux->used_hwdmx) {
		if (sec_feed->secchan) {
			hwdmx_free_chan(sec_feed->secchan);
			sec_feed->secchan = NULL;
		}
	}
	mutex_unlock(demux->pmutex);
	return 0;
}
static int _dmx_add_frontend(struct dmx_demux *dmx,
			       struct dmx_frontend *frontend)
{
	struct aml_dmx *demux = (struct aml_dmx *)dmx->priv;
	struct list_head *head = &demux->frontend_list;

	list_add(&(frontend->connectivity_list), head);

	return 0;
}

static int _dmx_remove_frontend(struct dmx_demux *dmx,
				  struct dmx_frontend *frontend)
{
	struct aml_dmx *demux = (struct aml_dmx *)dmx->priv;
	struct list_head *pos, *n, *head = &demux->frontend_list;

	list_for_each_safe(pos, n, head) {
		if (DMX_FE_ENTRY(pos) == frontend) {
			list_del(pos);
			return 0;
		}
	}

	return -ENODEV;
}

static struct list_head *_dmx_get_frontends(struct dmx_demux *dmx)
{
	struct aml_dmx *demux = (struct aml_dmx *)dmx->priv;

	if (list_empty(&demux->frontend_list))
		return NULL;

	return &demux->frontend_list;
}

static int _dmx_connect_frontend(struct dmx_demux *dmx,
				   struct dmx_frontend *frontend)
{
	struct aml_dmx *demux = (struct aml_dmx *)dmx->priv;

	if (dmx->frontend)
		return -EINVAL;

	mutex_lock(demux->pmutex);

	dmx->frontend = frontend;
	mutex_unlock(demux->pmutex);
	return 0;
}

static int _dmx_disconnect_frontend(struct dmx_demux *dmx)
{
	struct aml_dmx *demux = (struct aml_dmx *)dmx;

	mutex_lock(demux->pmutex);

	dmx->frontend = NULL;
	mutex_unlock(demux->pmutex);
	return 0;
}

static int _dmx_get_pes_pids(struct dmx_demux *dmx, u16 * pids)
{
	struct aml_dmx *demux = (struct aml_dmx *)dmx;

	memcpy(pids, demux->pids, 5 * sizeof(u16));
	return 0;
}
static int _dmx_write(struct aml_dmx *pdmx, const u8 *buf, size_t count)
{
	int n = 0;

	if (pdmx->tsp == NULL)
	{
		pr_error("_dmx_write invalid tsp\n");
		return -1;
	}

	if (mutex_lock_interruptible(pdmx->pmutex))
		return -ERESTARTSYS;

	n = swdmx_ts_parser_run(pdmx->tsp, (SWDMX_UInt8 *)buf, count);
	if (n != 0) {
//		printk("call swdmx_ts_parser_run,len 0x%zx,Remaining len: 0x%0x\n",count,n);
	}
	mutex_unlock(pdmx->pmutex);

	return n;
}

int dmx_init(struct aml_dmx *pdmx, struct dvb_adapter *dvb_adapter)
{
	int ret;
	int i = 0;

	pdmx->dmx.capabilities =
		(DMX_TS_FILTERING | DMX_SECTION_FILTERING |
		 DMX_MEMORY_BASED_FILTERING);
	pdmx->dmx.priv = pdmx;

	pdmx->ts_feed_num = MAX_TS_FEED_NUM;
	pdmx->ts_feed = vmalloc(sizeof(struct sw_demux_ts_feed)*(pdmx->ts_feed_num));
	if (!pdmx->ts_feed) {
		return -ENOMEM;
	}
	for (i=0; i<pdmx->ts_feed_num; i++) {
		pdmx->ts_feed[i].state = DMX_STATE_FREE;
	}

	pdmx->sec_feed_num = MAX_SEC_FEED_NUM;
	pdmx->section_feed = vmalloc(sizeof(struct sw_demux_sec_feed)*(pdmx->sec_feed_num));
	if (!pdmx->section_feed) {
		vfree(pdmx->ts_feed);
		pdmx->ts_feed = NULL;
		return -ENOMEM;
	}

	for (i=0; i<pdmx->sec_feed_num; i++) {
		pdmx->section_feed[i].state = DMX_STATE_FREE;
	}
	INIT_LIST_HEAD(&pdmx->frontend_list);

	for (i = 0; i < DMX_PES_OTHER; i++) {
		pdmx->pids[i] = 0xffff;
	}

	pdmx->used_hwdmx = 1;

	pdmx->dmx.open = _dmx_open;
	pdmx->dmx.close = _dmx_close;
	pdmx->dmx.write = _dmx_write_from_user;
	pdmx->dmx.allocate_ts_feed = _dmx_allocate_ts_feed;
	pdmx->dmx.release_ts_feed = _dmx_release_ts_feed;
	pdmx->dmx.allocate_section_feed = _dmx_allocate_section_feed;
	pdmx->dmx.release_section_feed = _dmx_release_section_feed;

	pdmx->dmx.add_frontend = _dmx_add_frontend;
	pdmx->dmx.remove_frontend = _dmx_remove_frontend;
	pdmx->dmx.get_frontends = _dmx_get_frontends;
	pdmx->dmx.connect_frontend = _dmx_connect_frontend;
	pdmx->dmx.disconnect_frontend = _dmx_disconnect_frontend;
	pdmx->dmx.get_pes_pids = _dmx_get_pes_pids;

	pdmx->dev.filternum = (MAX_TS_FEED_NUM+MAX_SEC_FEED_NUM);
	pdmx->dev.demux = &pdmx->dmx;
	pdmx->dev.capabilities = 0;
	ret = dvb_dmxdev_init(&pdmx->dev, dvb_adapter);
	if (ret < 0) {
		pr_error("dvb_dmxdev_init failed: error %d\n", ret);
		vfree(pdmx->ts_feed);
		return -1;
	}
	pdmx->dev.dvr_dvbdev->writers = MAX_SW_DEMUX_USERS;

	pdmx->mem_fe.source = DMX_MEMORY_FE;
	ret = pdmx->dmx.add_frontend(&pdmx->dmx,&pdmx->mem_fe);
	if (ret <0) {
		pr_error("dvb_dmxdev_init add frontend: error %d\n", ret);
		vfree(pdmx->ts_feed);
		return -1;
	}
	pdmx->dmx.connect_frontend(&pdmx->dmx,&pdmx->mem_fe);
	if (ret <0) {
		pr_error("dvb_dmxdev_init connect frontend: error %d\n", ret);
		vfree(pdmx->ts_feed);
		return -1;
	}
	pdmx->buf_warning_level = 60;
	pdmx->init = 1;
//	dvb_net_init(dvb_adapter, &dmx->dvb_net, &pdmx->dmx);

	return 0;

}
int dmx_destroy(struct aml_dmx *pdmx) {

	if (pdmx->init) {
		vfree(pdmx->ts_feed);
	//	mutex_destroy(pdmx->mutex);

		swdmx_demux_free(pdmx->swdmx);
		dvb_dmxdev_release(&pdmx->dev);
		pdmx->init = 0;
	}
	return 0;
}
int dmx_set_work_mode(struct aml_dmx *pdmx,int work_mode){

	if (pdmx->init == 0) {
		return -1;
	}
	pdmx->used_hwdmx = work_mode;
	return 0;
}
int dmx_get_work_mode(struct aml_dmx *pdmx,int *work_mode){

	if (pdmx->init == 0) {
		return -1;
	}
	*work_mode = pdmx->used_hwdmx;
	return 0;
}

int dmx_get_buf_warning_status(struct aml_dmx *pdmx, int *status){
	int i = 0;
	ssize_t free_mem = 0;
	ssize_t total_mem = 0;

	struct dmxdev *pdev = &pdmx->dev;

	if (pdmx->init == 0) {
		return -1;
	}

	for (i = 0; i < pdev->filternum; i++) {
		if ((pdev->filter[i].state < DMXDEV_STATE_SET) ||
			(pdev->filter[i].type != DMXDEV_TYPE_PES))
			continue;

		free_mem = dvb_ringbuffer_free(&pdev->filter[i].buffer);
		total_mem = pdev->filter[i].buffer.size;

		if ((total_mem-free_mem)*100/total_mem >= pdmx->buf_warning_level) {
			*status = 1;
			return 0;
		}
	}
	*status = 0;
	return 0;
}

int dmx_set_buf_warning_level(struct aml_dmx *pdmx, int level) {
	pdmx->buf_warning_level = level;
	return 0;
}
int dmx_write_sw_from_user(struct aml_dmx *pdmx, const char __user *buf, size_t count)
{
	void *p;
	int ret = 0;

	p = memdup_user(buf, count);
	if (IS_ERR(p)) {
		pr_error("get fail mem pointer\n");
		return PTR_ERR(p);
	}
	ret = _dmx_write(pdmx, p, count);

	kfree(p);

	if (signal_pending(current))
		return -EINTR;
	return ret;
}

