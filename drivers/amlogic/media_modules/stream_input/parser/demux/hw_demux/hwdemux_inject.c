/*
 *
 * Copyright (C) 2018 Amlogic, Inc. All rights reserved.
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
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/amlogic/media/utils/vdec_reg.h>
#include "../../../../frame_provider/decoder/utils/vdec.h"

#include "../aml_dvb.h"
#include "hwdemux.h"
#include "hwdemux_internal.h"

#define FETCHBUF_SIZE   (64*1024)

#define RESET_PARSER        (1<<8)
#define PARSER_INTSTAT_FETCH_CMD    (1<<7)
#define PARSER_INT_HOST_EN_BIT      8

#define FETCH_ENDIAN                27
#define FETCH_ENDIAN_MASK           (0x7<<27)

#define PS_CFG_PFIFO_EMPTY_CNT_BIT      16
#define PS_CFG_MAX_ES_WR_CYCLE_BIT      12
#define PS_CFG_MAX_FETCH_CYCLE_BIT      0

#define pr_dbg_flag(_f, _args...)\
	do {\
		if (1&(_f))\
			printk(_args);\
	} while (0)

#define pr_dbg(args...)	pr_dbg_flag(0x1, args)

static DECLARE_WAIT_QUEUE_HEAD(wq);
static u32 fetch_done = 0;
static void *fetchbuf = NULL;

static const char tsdemux_fetch_id[] = "tsdemux-fetch-id";

static int stbuf_fetch_init(void)
{
	if (NULL != fetchbuf)
		return 0;
	pr_dbg("%s line:%d\n",__FUNCTION__,__LINE__);

	fetchbuf = (void *)__get_free_pages(GFP_KERNEL,
					 get_order(FETCHBUF_SIZE));
	if (!fetchbuf) {
		pr_info("%s: Can not allocate fetch working buffer\n",
			 __func__);
		return -ENOMEM;
	}
	return 0;
}
static void stbuf_fetch_release(void)
{
	if (0 && fetchbuf) {
		/* always don't free.for safe alloc/free*/
		free_pages((unsigned long)fetchbuf, get_order(FETCHBUF_SIZE));
		fetchbuf = 0;
	}
}
static irqreturn_t parser_isr(int irq, void *dev_id)
{
	u32 int_status = READ_PARSER_REG(PARSER_INT_STATUS);

	WRITE_PARSER_REG(PARSER_INT_STATUS, int_status);

	if (int_status & PARSER_INTSTAT_FETCH_CMD) {
		fetch_done = 1;

		wake_up_interruptible(&wq);
	}

	return IRQ_HANDLED;
}
static void _paser_reset(void) {
	WRITE_MPEG_REG(RESET1_REGISTER, RESET_PARSER);

	WRITE_PARSER_REG(PARSER_CONFIG,
				   (10 << PS_CFG_PFIFO_EMPTY_CNT_BIT) |
				   (1 << PS_CFG_MAX_ES_WR_CYCLE_BIT) |
				   (16 << PS_CFG_MAX_FETCH_CYCLE_BIT));

	WRITE_PARSER_REG(PARSER_INT_STATUS, 0xffff);
	WRITE_PARSER_REG(PARSER_INT_ENABLE,
			PARSER_INTSTAT_FETCH_CMD << PARSER_INT_HOST_EN_BIT);
}
static void _hwdmx_inject_init(HWDMX_Demux *pdmx) {

	s32 r;
	pr_info("%s line:%d\n",__FUNCTION__,__LINE__);

	if (pdmx->inject_init)
		return ;

	stbuf_fetch_init();

	r = vdec_request_irq(PARSER_IRQ, parser_isr,
			"tsdemux-fetch", (void *)tsdemux_fetch_id);
	if (r)
		goto err;

	_paser_reset();
	pr_info("%s line:%d\n",__FUNCTION__,__LINE__);

	pdmx->inject_init = 1;
	return;
err:
	pr_info("_hwdmx_inject_init, err\n");
	return ;
}
struct device *hwdemux_get_dma_device(void)
{
	return aml_get_dvb_device()->dev;
}
static int _hwdmx_inject_write(HWDMX_Demux *pdmx, const char *buf, int count) {

	size_t r = count;
	const char __user *p = buf;
	u32 len;
	int ret;
	int isphybuf = 0;
	dma_addr_t dma_addr = 0;

	if (r > 0) {
	 if (isphybuf)
		 len = count;
	 else {
		 len = min_t(size_t, r, FETCHBUF_SIZE);
		 if (copy_from_user(fetchbuf, p, len))
			 return -EFAULT;

		 dma_addr =
			 dma_map_single(hwdemux_get_dma_device(),
					 fetchbuf,
					 FETCHBUF_SIZE, DMA_TO_DEVICE);
		 if (dma_mapping_error(hwdemux_get_dma_device(),
					 dma_addr))
			 return -EFAULT;
	 }

	 fetch_done = 0;

	 wmb(); 	 /* Ensure fetchbuf  contents visible */

	 if (isphybuf) {
		 u32 buf_32 = (unsigned long)buf & 0xffffffff;
		 WRITE_PARSER_REG(PARSER_FETCH_ADDR, buf_32);
	 } else {
		 WRITE_PARSER_REG(PARSER_FETCH_ADDR, dma_addr);
		 dma_unmap_single(hwdemux_get_dma_device(), dma_addr,
				 FETCHBUF_SIZE, DMA_TO_DEVICE);
	 }

	 WRITE_PARSER_REG(PARSER_FETCH_CMD, (7 << FETCH_ENDIAN) | len);

	 ret =
		 wait_event_interruptible_timeout(wq, fetch_done != 0,
				 HZ / 2);
	 if (ret == 0) {
		 WRITE_PARSER_REG(PARSER_FETCH_CMD, 0);
		 pr_info("write timeout, retry\n");
		 ret = -EAGAIN;
		 goto ERROR_RESET;
	 } else if (ret < 0) {
		 ret = -ERESTARTSYS;
		 goto ERROR_RESET;
	 }
	 p += len;
	 r -= len;
  }
  return count - r;
ERROR_RESET:
	_paser_reset();
	return ret;
}

void hwdmx_inject_destroy(HWDMX_Demux *pdmx) {
 if (!(pdmx && pdmx->init)) {
	 return ;
 }
 stbuf_fetch_release();

 WRITE_PARSER_REG(PARSER_INT_ENABLE, 0);
 vdec_free_irq(PARSER_IRQ, (void *)tsdemux_fetch_id);
}

int hwdmx_inject(HWDMX_Demux *pdmx, const char *buf, int count) {
 int ret = 0;

 if (!(pdmx && pdmx->dmx_source == AM_TS_SRC_HIU)) {
	 return -1;
 }

 if (!(pdmx && pdmx->init)) {
	 return -1;
 }
 if (!pdmx->inject_init) {
	 _hwdmx_inject_init(pdmx);
 }
 ret = _hwdmx_inject_write(pdmx, buf, count);
 if (ret < 0) {
	ret = 0;
 }
 return ret;
}

