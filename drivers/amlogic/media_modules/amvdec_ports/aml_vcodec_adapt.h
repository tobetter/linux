#ifndef VDEC_ADAPT_H
#define VDEC_ADAPT_H

#include <linux/amlogic/media/utils/vformat.h>
#include "aml_vcodec_drv.h"

struct aml_vdec_adapt {
	enum vformat_e id;
	void *vsi;
	int32_t failure;
	uint32_t inst_addr;
	unsigned int signaled;
	struct aml_vcodec_ctx *ctx;
	struct platform_device *dev;
	wait_queue_head_t wq;
	struct file *filp;
	//ipi_handler_t handler;
};

int video_decoder_init(struct aml_vdec_adapt *vdec);

int video_decoder_release(struct aml_vdec_adapt *vdec);

int vdec_vbuf_write(struct file *file, const char *buf, unsigned int count);

int vdec_vframe_write(struct file *file, const char *buf,
	unsigned int count, unsigned long int pts);

int is_need_to_buf(void);

void aml_decoder_flush(void);

void aml_codec_reset(void);

extern void dump_write(const char __user *buf, size_t count);

bool is_decoder_ready(void);

#endif /* VDEC_ADAPT_H */

