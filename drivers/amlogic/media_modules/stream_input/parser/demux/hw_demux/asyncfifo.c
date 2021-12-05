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
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/wait.h>

#include "c_stb_define.h"
#include "c_stb_regs_define.h"
#include "dvb_reg.h"

#include "../aml_dvb.h"
#include "hwdemux.h"
#include "hwdemux_internal.h"
#include "asyncfifo.h"

struct aml_asyncfifo {
	int	id;
	int	init;
	enum aml_dmx_id_t	source;
	unsigned long	pages;
	unsigned long   pages_map;
	int	buf_len;
	int	buf_write;
	int buf_read;
	int flush_size;
	struct aml_dvb *dvb;
	HWDMX_Demux *pdmx;
	int secure_enable;
	unsigned long secure_mem;
	HWDMX_Cb cb;
	void *udata;

	void *cache;
	int cache_len;
	int remain_len;
	int flush_time_ms;
	spinlock_t  slock;
	unsigned int wakeup;
	wait_queue_head_t wait_queue;
	struct task_struct *asyncfifo_task;
	struct timer_list asyncfifo_timer;
};

#define ASYNCFIFO_COUNT 3
#define ASYNCFIFO_BUFFER_SIZE_DEFAULT (512*1024)

#define ASYNCFIFO_CACHE_LEN 		  (188)

#define READ_PERI_REG			READ_CBUS_REG
#define WRITE_PERI_REG			WRITE_CBUS_REG

#define READ_ASYNC_FIFO_REG(i, r) \
	((i) ? ((i-1)?READ_PERI_REG(ASYNC_FIFO1_##r):\
	READ_PERI_REG(ASYNC_FIFO2_##r)) : READ_PERI_REG(ASYNC_FIFO_##r))

#define WRITE_ASYNC_FIFO_REG(i, r, d)\
	do {\
		if (i == 2) {\
			WRITE_PERI_REG(ASYNC_FIFO1_##r, d);\
		} else if (i == 0) {\
			WRITE_PERI_REG(ASYNC_FIFO_##r, d);\
		} else {\
			WRITE_PERI_REG(ASYNC_FIFO2_##r, d);\
		} \
	} while (0)

#ifndef MIN
#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#endif


#define CLEAR_ASYNC_FIFO_REG_MASK(i, reg, mask) \
	WRITE_ASYNC_FIFO_REG(i, reg, \
	(READ_ASYNC_FIFO_REG(i, reg)&(~(mask))))

#define pr_error(fmt, args...) printk("DVB: " fmt, ## args)
#define pr_inf(fmt, args...)   printk("DVB: " fmt, ## args)
#define pr_dbg(fmt, args...)   printk(KERN_DEBUG fmt, ## args)

#define asyncfifo_get_dev(afifo) ((afifo)->dvb->dev)
#define ASYNCFIFO_TIMER    		50

static int async_fifo_total_count = 2;
static struct aml_asyncfifo asyncfifo[ASYNCFIFO_COUNT];
static int asyncfifo_buf_len = ASYNCFIFO_BUFFER_SIZE_DEFAULT;

static int asyncfifo_flush_time = ASYNCFIFO_TIMER;
module_param(asyncfifo_flush_time, int, 0644);
MODULE_PARM_DESC(asyncfifo_flush_time,
		"asyncfifo flush time ms");

static int _asyncfifo_reset_all(void);

int asyncfifo_probe(struct platform_device *pdev) {

	char buf[32];
	u32 value = 0;
	int ret = 0;

	memset(&asyncfifo, 0,sizeof(asyncfifo))	;

	memset(buf, 0, 32);
	snprintf(buf, sizeof(buf), "asyncfifo_count");
	ret = of_property_read_u32(pdev->dev.of_node, buf, &value);
	if (!ret) {
		pr_inf("%s: 0x%x\n", buf, value);
		async_fifo_total_count = value;
	}

	memset(buf, 0, 32);
	snprintf(buf, sizeof(buf), "asyncfifo_buf_len");
	ret = of_property_read_u32(pdev->dev.of_node, buf, &value);
	if (!ret) {
		pr_inf("%s: 0x%x\n", buf, value);
		asyncfifo_buf_len = value;
	}
	_asyncfifo_reset_all();
	return 0;
}

int asyncfifo_remove(void) {
	return 0;
}
static inline int _dmx_get_order(unsigned long size)
{
	int order;

	order = -1;
	do {
		size >>= 1;
		order++;
	} while (size);

	return order;
}

static void dvr_process_channel(struct aml_asyncfifo *afifo)
{
	int cnt;
	int used_len;
	int w_size = 0;
	u8 *remain_addr;
	int buf_read = 0;
	int buf_write = 0;
	unsigned long flags;

	spin_lock_irqsave(&afifo->slock,flags);
	buf_read = afifo->buf_read;
	buf_write = afifo->buf_write;
	spin_unlock_irqrestore(&afifo->slock,flags);

	pr_dbg("%s enter\n",__FUNCTION__);

	if (buf_read > buf_write) {
		cnt = afifo->buf_len - buf_read;
		if (afifo->remain_len != 0) {
			cnt = MIN((afifo->cache_len-afifo->remain_len),cnt);
		}

		if (!afifo->secure_enable) {
			dma_sync_single_for_cpu(asyncfifo_get_dev(afifo),
				afifo->pages_map+buf_read,
				cnt,
				DMA_FROM_DEVICE);
			if (afifo->init && afifo->pdmx && afifo->cb) {
				if (afifo->remain_len == 0) {
					w_size = afifo->cb((u8 *)afifo->pages+buf_read,cnt,afifo->udata);
					afifo->remain_len = cnt-w_size;
					if (afifo->remain_len) {
						remain_addr = (u8 *)afifo->pages+buf_read+cnt-afifo->remain_len;
						memcpy(afifo->cache,(u8 *)remain_addr,afifo->remain_len);
					}
				} else {
					memcpy(afifo->cache+afifo->remain_len,(u8 *)afifo->pages+buf_read,cnt);
					used_len = cnt+afifo->remain_len;
					w_size	= afifo->cb(afifo->cache, used_len, afifo->udata);
					afifo->remain_len = used_len - w_size;
					if (afifo->remain_len) {
						printk("cache:total:%d,w_size:%d,remain:%d\n",\
							used_len,w_size,afifo->remain_len);
					}
				}
			}
		}
		spin_lock_irqsave(&afifo->slock,flags);
		afifo->buf_read = (buf_read+cnt)%afifo->buf_len;
		spin_unlock_irqrestore(&afifo->slock,flags);
	}
	else if (buf_write > buf_read) {
		cnt = buf_write - buf_read;

		if (afifo->remain_len != 0) {
			cnt = MIN((afifo->cache_len-afifo->remain_len),cnt);
		}

		if (!afifo->secure_enable) {
			dma_sync_single_for_cpu(asyncfifo_get_dev(afifo),
				afifo->pages_map+buf_read,
				cnt,
				DMA_FROM_DEVICE);
			if (afifo->init && afifo->pdmx && afifo->cb) {
				if (afifo->remain_len == 0) {
					w_size = afifo->cb((u8 *)afifo->pages+buf_read,cnt,afifo->udata);
					afifo->remain_len = cnt - w_size;
					if (afifo->remain_len) {
						remain_addr = (u8 *)afifo->pages+buf_read+cnt-afifo->remain_len;
						memcpy(afifo->cache,(u8 *)remain_addr,afifo->remain_len);
					}
				} else {
					memcpy(afifo->cache+afifo->remain_len,(u8 *)afifo->pages+buf_read,cnt);
					used_len = cnt+afifo->remain_len;
					w_size = afifo->cb(afifo->cache, used_len, afifo->udata);
					afifo->remain_len = used_len - w_size;
					if (afifo->remain_len) {
						printk("cache:total:%d,w_size:%d,remain:%d\n",\
							used_len,w_size,afifo->remain_len);
					}
				}
			}
		}
		spin_lock_irqsave(&afifo->slock,flags);
		afifo->buf_read = (buf_read+cnt);
		spin_unlock_irqrestore(&afifo->slock,flags);
	}
	//pr_dbg_irq_dvr("write data to dvr\n");
}
static int check_wakeup(struct aml_asyncfifo *afifo){
	if (afifo->wakeup) {
		afifo->wakeup = 0;
		return 1;
	}
	return 0;
}
static int dvr_task_asyncfifo_func(void *data)
{
	int timeout = 0;
	struct aml_asyncfifo *afifo = (struct aml_asyncfifo *)data;
	HWDMX_Demux *pdmx;
	struct aml_dvb *dvb = afifo->dvb;

	pr_dbg("%s enter,line:%d\n",__FUNCTION__,__LINE__);
	while (!kthread_should_stop()) {
		timeout = wait_event_interruptible_timeout(afifo->wait_queue, check_wakeup(afifo), 3*HZ);
		if (timeout <= 0)
			continue;
		if (dvb && afifo->source >= AM_DMX_0 &&
				afifo->source < AM_DMX_MAX) {
			pdmx = afifo->pdmx;
			if (pdmx->init && afifo->init && afifo->cb) {
				int buf_read = 0;
				int buf_write = 0;
				unsigned long flags;

loop:
				dvr_process_channel(afifo);
				spin_lock_irqsave(&afifo->slock,flags);
				buf_read = afifo->buf_read;
				buf_write = afifo->buf_write;
				spin_unlock_irqrestore(&afifo->slock,flags);
				if (buf_read != buf_write)
					goto loop;
			}
		}
	}
	return 0;
}
static void timer_asyncfifo_func(unsigned long arg)
{
	struct aml_asyncfifo *afifo = (struct aml_asyncfifo *)arg;
	HWDMX_Demux *pdmx;
	struct aml_dvb *dvb = afifo->dvb;
	u32 start_addr;
	int reg_val;
	int buf_write = 0;
	int buf_read = 0;
	unsigned long flags;

	if (dvb && afifo->source >= AM_DMX_0 && afifo->source < AM_DMX_MAX) {
		pdmx = afifo->pdmx;
		if (pdmx->init && afifo->init && afifo->cb) {
			reg_val = READ_ASYNC_FIFO_REG(afifo->id, REG0);
			start_addr = virt_to_phys((void *)afifo->pages);

			spin_lock_irqsave(&afifo->slock,flags);
			afifo->buf_write = reg_val - start_addr;
			buf_write = afifo->buf_write;
			buf_read = afifo->buf_read;
			spin_unlock_irqrestore(&afifo->slock,flags);

			if (buf_write != buf_read) {
				afifo->wakeup = 1;
				wake_up_interruptible(&afifo->wait_queue);
			}
		}
	}
	mod_timer(&afifo->asyncfifo_timer,
		  jiffies + msecs_to_jiffies(asyncfifo_flush_time));
}

/*Allocate ASYNC FIFO Buffer*/
static unsigned long _asyncfifo_alloc_buffer(int len)
{
	unsigned long pages = __get_free_pages(GFP_KERNEL, get_order(len));

	if (!pages) {
		pr_error("cannot allocate async fifo buffer\n");
		return 0;
	}
	return pages;
}
static void _asyncfifo_free_buffer(unsigned long buf, int len)
{
	free_pages(buf, get_order(len));
}

static int _asyncfifo_set_buffer(struct aml_asyncfifo *afifo,
					int len, unsigned long buf)
{
	if (afifo->pages)
		return -1;

	afifo->buf_write = 0;
	afifo->buf_read   = 0;
	afifo->buf_len = len;
	pr_error("++++async fifo %d buf size %d, flush size %d\n",
			afifo->id, afifo->buf_len, afifo->flush_size);

	if ((afifo->flush_size <= 0)
			|| (afifo->flush_size > (afifo->buf_len>>1))) {
		afifo->flush_size = afifo->buf_len>>1;
	} else if (afifo->flush_size < 128) {
		afifo->flush_size = 128;
	} else {
		int fsize;

		for (fsize = 128; fsize < (afifo->buf_len>>1); fsize <<= 1) {
			if (fsize >= afifo->flush_size)
				break;
		}

		afifo->flush_size = fsize;
	}

	afifo->pages = buf;
	if (!afifo->pages)
		return -1;

	afifo->pages_map = dma_map_single(asyncfifo_get_dev(afifo),
			(void *)afifo->pages, afifo->buf_len, DMA_FROM_DEVICE);

	return 0;
}
static void _asyncfifo_put_buffer(struct aml_asyncfifo *afifo, int freemem)
{
	if (afifo->pages) {
		dma_unmap_single(asyncfifo_get_dev(afifo),
			afifo->pages_map, afifo->buf_len, DMA_FROM_DEVICE);
		if (freemem)
			_asyncfifo_free_buffer(afifo->pages, afifo->buf_len);
		afifo->pages_map = 0;
		afifo->pages = 0;
	}
}

static int _async_fifo_init(struct aml_asyncfifo *afifo, int initirq,
			int buf_len, unsigned long buf)
{
	int ret = 0;

	if (afifo->init)
		return -1;

	afifo->source  = AM_DMX_MAX;
	afifo->pages = 0;
	afifo->buf_write = 0;
	afifo->buf_read = 0;
	afifo->buf_len = 0;
	afifo->wakeup = 0;
	spin_lock_init(&afifo->slock);

	init_waitqueue_head(&afifo->wait_queue);

	afifo->asyncfifo_task = kthread_run(dvr_task_asyncfifo_func,(void *)afifo, "asyncfifo%d", afifo->id);
	if (!afifo->asyncfifo_task) {
		pr_error("create asyncfifo task fail\n");
	}
	afifo->flush_time_ms = asyncfifo_flush_time;
	init_timer(&afifo->asyncfifo_timer);
	afifo->asyncfifo_timer.function = timer_asyncfifo_func;
	afifo->asyncfifo_timer.expires =
		jiffies + msecs_to_jiffies(afifo->flush_time_ms);
	afifo->asyncfifo_timer.data = (unsigned long)afifo;
	add_timer(&afifo->asyncfifo_timer);

	/*alloc buffer*/
	ret = _asyncfifo_set_buffer(afifo, buf_len, buf);

	afifo->init = 1;

	return ret;
}
static int _async_fifo_deinit(struct aml_asyncfifo *afifo, int freeirq, int freemem)
{
	if (!afifo->init)
		return 0;

	CLEAR_ASYNC_FIFO_REG_MASK(afifo->id, REG1, 1 << ASYNC_FIFO_FLUSH_EN);
	CLEAR_ASYNC_FIFO_REG_MASK(afifo->id, REG2, 1 << ASYNC_FIFO_FILL_EN);

	if (freemem)
		_asyncfifo_put_buffer(afifo,1);
	else
		_asyncfifo_put_buffer(afifo,0);

	kfree(afifo->cache);

	afifo->source  = AM_DMX_MAX;
	afifo->buf_write = 0;
	afifo->buf_read = 0;
	afifo->buf_len = 0;
	afifo->cache_len = 0;
	afifo->remain_len = 0;

	del_timer(&afifo->asyncfifo_timer);

	if (afifo->asyncfifo_task) {
		kthread_stop(afifo->asyncfifo_task);
		afifo->asyncfifo_task = NULL;
	}

	afifo->init = 0;

	return 0;
}
static inline int dmx_get_order(unsigned long size)
{
	int order;

	order = -1;
	do {
		size >>= 1;
		order++;
	} while (size);

	return order;
}

static void async_fifo_set_regs(struct aml_asyncfifo *afifo, int source_val)
{
	u32 start_addr = virt_to_phys((void *)afifo->pages);
	u32 size = afifo->buf_len;
	u32 flush_size = afifo->flush_size;
	int factor = dmx_get_order(size / flush_size);

	if (afifo->secure_enable) {
		start_addr = afifo->secure_mem;
	}

	pr_error("ASYNC FIFO id=%d, link to DMX%d, start_addr %x, buf_size %d,"
		"source value 0x%x, factor %d\n",
		afifo->id, afifo->source, start_addr, size, source_val, factor);
	/* Destination address */
	WRITE_ASYNC_FIFO_REG(afifo->id, REG0, start_addr);

	/* Setup flush parameters */
	WRITE_ASYNC_FIFO_REG(afifo->id, REG1,
			(0 << ASYNC_FIFO_TO_HIU) |
			(0 << ASYNC_FIFO_FLUSH) |
			/* don't flush the path */
			(1 << ASYNC_FIFO_RESET) |
			/* reset the path */
			(1 << ASYNC_FIFO_WRAP_EN) |
			/* wrap enable */
			(0 << ASYNC_FIFO_FLUSH_EN) |
			/* disable the flush path */
			/*(0x3 << ASYNC_FIFO_FLUSH_CNT_LSB);
			 * flush 3 x 32  32-bit words
			 */
			/*(0x7fff << ASYNC_FIFO_FLUSH_CNT_LSB);
			 * flush 4MBytes of data
			 */
			(((size >> 7) & 0x7fff) << ASYNC_FIFO_FLUSH_CNT_LSB));
			/* number of 128-byte blocks to flush */

	/* clear the reset signal */
	WRITE_ASYNC_FIFO_REG(afifo->id, REG1,
		     READ_ASYNC_FIFO_REG(afifo->id,
					REG1) & ~(1 << ASYNC_FIFO_RESET));
	/* Enable flush */
	WRITE_ASYNC_FIFO_REG(afifo->id, REG1,
		     READ_ASYNC_FIFO_REG(afifo->id,
				REG1) | (1 << ASYNC_FIFO_FLUSH_EN));

	/*Setup Fill parameters */
	WRITE_ASYNC_FIFO_REG(afifo->id, REG2,
			     (1 << ASYNC_FIFO_ENDIAN_LSB) |
			     (0 << ASYNC_FIFO_FILL_EN) |
			     /* disable fill path to reset fill path */
			     /*(96 << ASYNC_FIFO_FILL_CNT_LSB);
			      *3 x 32  32-bit words
			      */
			     (0 << ASYNC_FIFO_FILL_CNT_LSB));
				/* forever FILL; */
	WRITE_ASYNC_FIFO_REG(afifo->id, REG2,
			READ_ASYNC_FIFO_REG(afifo->id, REG2) |
				(1 << ASYNC_FIFO_FILL_EN));/*Enable fill path*/

	/* generate flush interrupt */
	WRITE_ASYNC_FIFO_REG(afifo->id, REG3,
			(READ_ASYNC_FIFO_REG(afifo->id, REG3) & 0xffff0000) |
				((((size >> (factor + 7)) - 1) & 0x7fff) <<
					ASYNC_FLUSH_SIZE_IRQ_LSB));

	/* Connect the STB DEMUX to ASYNC_FIFO */
	WRITE_ASYNC_FIFO_REG(afifo->id, REG2,
			READ_ASYNC_FIFO_REG(afifo->id, REG2) |
			(source_val << ASYNC_FIFO_SOURCE_LSB));
}

static void reset_async_fifos(struct aml_asyncfifo *afifo) {

	pr_dbg("Disable ASYNC FIFO id=%d\n", afifo->id);
	CLEAR_ASYNC_FIFO_REG_MASK(afifo->id, REG1,
				  1 << ASYNC_FIFO_FLUSH_EN);
	CLEAR_ASYNC_FIFO_REG_MASK(afifo->id, REG2,
				  1 << ASYNC_FIFO_FILL_EN);
	if (READ_ASYNC_FIFO_REG(afifo->id, REG2) &
			(1 << ASYNC_FIFO_FILL_EN) ||
		READ_ASYNC_FIFO_REG(afifo->id, REG1) &
			(1 << ASYNC_FIFO_FLUSH_EN)) {
		pr_error("Set reg failed\n");
	} else
		pr_dbg("Set reg ok\n");
	afifo->buf_write = 0;
	afifo->buf_read = 0;

	if (afifo->pdmx->dmx_id == AM_DMX_0)
		async_fifo_set_regs(afifo,0x3);
	else if (afifo->pdmx->dmx_id == AM_DMX_1)
		async_fifo_set_regs(afifo,0x2);
	else
		async_fifo_set_regs(afifo,0x0);
}

int asyncfifo_init(int id,HWDMX_Demux *pdmx) {
	int ret;
	int len;
	unsigned long buf;

	pr_inf("%s enter\n",__FUNCTION__);

	if (id >= async_fifo_total_count)
		return -1;

	asyncfifo[id].id = id;
	asyncfifo[id].init = 0;
	asyncfifo[id].flush_size = 256 * 1024;
	asyncfifo[id].secure_enable = 0;
	asyncfifo[id].secure_mem = 0;
	asyncfifo[id].dvb = aml_get_dvb_device();
	asyncfifo[id].pdmx = pdmx;
	asyncfifo[id].source = -1;
	asyncfifo[id].cache_len = ASYNCFIFO_CACHE_LEN;
	asyncfifo[id].cache = kmalloc(asyncfifo[id].cache_len,GFP_KERNEL);
	if (!asyncfifo[id].cache)
		return -1;

	asyncfifo[id].remain_len = 0;

	len = asyncfifo_buf_len;
	buf = _asyncfifo_alloc_buffer(len);
	if (!buf) {
		kfree(asyncfifo[id].cache);
		return -1;
	}

	ret = _async_fifo_init(&asyncfifo[id], 1, len, buf);
	if (ret < 0) {
		kfree(asyncfifo[id].cache);
		_asyncfifo_free_buffer(buf, len);
	}

	pr_inf("%s exit\n",__FUNCTION__);

	return ret;
}
int asyncfifo_deinit(int id) {
	int ret;

	if (id >= async_fifo_total_count)
		return -1;

	ret = _async_fifo_deinit(&asyncfifo[id], 1, 1);

	return ret;
}
int asyncfifo_reset(int id) {
	unsigned long	buf;
	int buf_len;
	int source;
	int ret = 0;
	HWDMX_Demux *pdmx;

	if (id >= async_fifo_total_count)
		return -1;
	if (!asyncfifo[id].init)
		return -1;

	source = asyncfifo[id].source;

	if (!asyncfifo[id].secure_enable) {
		buf = asyncfifo[id].pages;
		buf_len = asyncfifo[id].buf_len;
		pdmx = asyncfifo[id].pdmx;

		ret = _async_fifo_deinit(&asyncfifo[id], 1, 0);
		if (ret != 0)
			return ret;
		ret = _async_fifo_init(&asyncfifo[id], 0, buf_len, buf);
	}

	if (source >= AM_DMX_0 && source < AM_DMX_MAX) {
		reset_async_fifos(&asyncfifo[id]);
	}

	return ret;
}
static int _asyncfifo_reset_all(void) {
	int i = 0;
	WRITE_MPEG_REG(RESET6_REGISTER, (1<<11)|(1<<12));

	for (i = 0; i < async_fifo_total_count; i++) {
		asyncfifo_reset(i);
	}
	return 0;
}

int asyncfifo_set_source(int id, enum aml_dmx_id_t source) {
	if (id >= async_fifo_total_count)
		return -1;

	pr_inf("%s enter\n",__FUNCTION__);

	if (asyncfifo[id].source != source) {
		asyncfifo[id].source = source;
		reset_async_fifos(&asyncfifo[id]);
	}
	return 0;
}

int asyncfifo_set_cb(int id, HWDMX_Cb cb, void *udata)
{
	if (id >= async_fifo_total_count)
		return -1;

	asyncfifo[id].cb = cb;
	asyncfifo[id].udata = udata;
	return 0;
}

int asyncfifo_set_security_buf(int id, unsigned long pstart, int size){
	unsigned long	buf;
	int buf_len;
	int source = AM_DMX_MAX;
	int ret;

	if (id >= async_fifo_total_count)
		return -1;
	if (!asyncfifo[id].init)
		return -1;

	if (!asyncfifo[id].secure_enable) {
		buf = asyncfifo[id].pages;
		buf_len = asyncfifo[id].buf_len;
		source = asyncfifo[id].source;

		ret = _async_fifo_deinit(&asyncfifo[id], 1, 1);
		if (ret != 0)
			return ret;

		_asyncfifo_free_buffer(buf, buf_len);

		asyncfifo[id].source = source;
		asyncfifo[id].init = 1;
	} else {
		source = asyncfifo[id].source;
	}
	asyncfifo[id].secure_enable = 1;
	asyncfifo[id].secure_mem = pstart;
	asyncfifo[id].buf_len = size;

	if (source >= AM_DMX_0 && source < AM_DMX_MAX) {
		reset_async_fifos(&asyncfifo[id]);
	}
	return 0;
}
