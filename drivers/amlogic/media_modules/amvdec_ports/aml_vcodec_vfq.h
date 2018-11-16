/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef __AML_VCODEC_VFQ_H_
#define __AML_VCODEC_VFQ_H_

#include <linux/types.h>
#include <asm/barrier.h>

struct vfq_s {
	int rp;
	int wp;
	int size;
	int pre_rp;
	int pre_wp;
	struct vframe_s **pool;
};

static inline void vfq_lookup_start(struct vfq_s *q)
{
	q->pre_rp = q->rp;
	q->pre_wp = q->wp;
}
static inline void vfq_lookup_end(struct vfq_s *q)
{
	q->rp = q->pre_rp;
	q->wp = q->pre_wp;
}

static inline void vfq_init(struct vfq_s *q, u32 size, struct vframe_s **pool)
{
	q->rp = q->wp = 0;
	q->size = size;
	q->pool = pool;
}

static inline bool vfq_empty(struct vfq_s *q)
{
	return q->rp == q->wp;
}

static inline void vfq_push(struct vfq_s *q, struct vframe_s *vf)
{
	int wp = q->wp;

	/*ToDo*/
	smp_mb();

	q->pool[wp] = vf;

	/*ToDo*/
	smp_wmb();

	q->wp = (wp == (q->size - 1)) ? 0 : (wp + 1);
}

static inline struct vframe_s *vfq_pop(struct vfq_s *q)
{
	struct vframe_s *vf;
	int rp;

	if (vfq_empty(q))
		return NULL;

	rp = q->rp;

	/*ToDo*/
	smp_rmb();

	vf = q->pool[rp];

	/*ToDo*/
	smp_mb();

	q->rp = (rp == (q->size - 1)) ? 0 : (rp + 1);

	return vf;
}

static inline struct vframe_s *vfq_peek(struct vfq_s *q)
{
	return (vfq_empty(q)) ? NULL : q->pool[q->rp];
}

static inline int vfq_level(struct vfq_s *q)
{
	int level = q->wp - q->rp;

	if (level < 0)
		level += q->size;

	return level;
}

#endif /* __AML_VCODEC_VFQ_H_ */

