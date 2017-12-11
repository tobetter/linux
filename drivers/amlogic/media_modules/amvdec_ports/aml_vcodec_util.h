#ifndef _AML_VCODEC_UTIL_H_
#define _AML_VCODEC_UTIL_H_

#include <linux/types.h>
#include <linux/dma-direction.h>
#include <linux/amlogic/media/codec_mm/codec_mm.h>

#define DEBUG

struct aml_vcodec_mem {
	size_t size;
	void *va;
	dma_addr_t dma_addr;
	unsigned int bytes_used;
};

struct aml_vcodec_ctx;
struct aml_vcodec_dev;

extern int aml_v4l2_dbg_level;
extern bool aml_vcodec_dbg;


#if defined(DEBUG)

#define aml_v4l2_debug(level, fmt, args...)				 \
	do {								 \
		if (aml_v4l2_dbg_level >= level)			 \
			pr_info("[AML_V4L2] level=%d %s(),%d: " fmt "\n",\
				level, __func__, __LINE__, ##args);	 \
	} while (0)

#define aml_v4l2_debug_enter()  aml_v4l2_debug(3, "+")
#define aml_v4l2_debug_leave()  aml_v4l2_debug(3, "-")

#define aml_vcodec_debug(h, fmt, args...)				\
	do {								\
		if (aml_vcodec_dbg)					\
			pr_info("[AML_VCODEC][%d]: %s() " fmt "\n",	\
				((struct aml_vcodec_ctx *)h->ctx)->id, \
				__func__, ##args);			\
	} while (0)

#define aml_vcodec_debug_enter(h)  aml_vcodec_debug(h, "+")
#define aml_vcodec_debug_leave(h)  aml_vcodec_debug(h, "-")

#else

#define aml_v4l2_debug(level, fmt, args...)
#define aml_v4l2_debug_enter()
#define aml_v4l2_debug_leave()

#define aml_vcodec_debug(h, fmt, args...)
#define aml_vcodec_debug_enter(h)
#define aml_vcodec_debug_leave(h)

#endif

#define aml_v4l2_err(fmt, args...)                \
	pr_err("[AML_V4L2][ERROR] %s:%d: " fmt "\n", __func__, __LINE__, \
	       ##args)

#define aml_vcodec_err(h, fmt, args...)					\
	pr_err("[AML_VCODEC][ERROR][%d]: %s() " fmt "\n",		\
	       ((struct aml_vcodec_ctx *)h->ctx)->id, __func__, ##args)

void __iomem *aml_vcodec_get_reg_addr(struct aml_vcodec_ctx *data,
				unsigned int reg_idx);
int aml_vcodec_mem_alloc(struct aml_vcodec_ctx *data,
				struct aml_vcodec_mem *mem);
void aml_vcodec_mem_free(struct aml_vcodec_ctx *data,
				struct aml_vcodec_mem *mem);
void aml_vcodec_set_curr_ctx(struct aml_vcodec_dev *dev,
	struct aml_vcodec_ctx *ctx);
struct aml_vcodec_ctx *aml_vcodec_get_curr_ctx(struct aml_vcodec_dev *dev);

#endif /* _AML_VCODEC_UTIL_H_ */
