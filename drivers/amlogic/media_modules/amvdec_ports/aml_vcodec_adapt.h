#ifndef VDEC_ADAPT_H
#define VDEC_ADAPT_H

#include <linux/amlogic/media/utils/vformat.h>
#include <linux/amlogic/media/utils/amstream.h>
#include "../stream_input/parser/streambuf.h"
#include "aml_vcodec_drv.h"

struct aml_vdec_adapt {
	enum vformat_e format;
	void *vsi;
	int32_t failure;
	uint32_t inst_addr;
	unsigned int signaled;
	struct aml_vcodec_ctx *ctx;
	struct platform_device *dev;
	wait_queue_head_t wq;
	struct file *filp;
	struct vdec_s *vdec;
	struct stream_port_s port;
	struct dec_sysinfo dec_prop;
	char *recv_name;
};

int video_decoder_init(struct aml_vdec_adapt *ada_ctx);

int video_decoder_release(struct aml_vdec_adapt *ada_ctx);

int vdec_vbuf_write(struct aml_vdec_adapt *ada_ctx,
	const char *buf, unsigned int count);

int vdec_vframe_write(struct aml_vdec_adapt *ada_ctx,
	const char *buf, unsigned int count, unsigned long int timestamp);

int is_need_to_buf(struct aml_vdec_adapt *ada_ctx);

void aml_decoder_flush(struct aml_vdec_adapt *ada_ctx);

int aml_codec_reset(struct aml_vdec_adapt *ada_ctx);

extern void dump_write(const char __user *buf, size_t count);

bool is_input_ready(struct aml_vdec_adapt *ada_ctx);

#endif /* VDEC_ADAPT_H */

