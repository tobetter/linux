// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2021 BayLibre, SAS
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 */

#include <linux/bitfield.h>

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_device.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_gem_atomic_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_plane_helper.h>

#include "meson_cursor.h"
#include "meson_registers.h"
#include "meson_viu.h"

struct meson_cursor {
	struct drm_plane base;
	struct meson_drm *priv;
};
#define to_meson_cursor(x) container_of(x, struct meson_cursor, base)

static int meson_cursor_atomic_check(struct drm_plane *plane,
				    struct drm_atomic_state *state)
{
	struct drm_plane_state *new_plane_state = drm_atomic_get_new_plane_state(state,
										 plane);
	struct drm_crtc_state *crtc_state;

	if (!new_plane_state->crtc)
		return 0;

	crtc_state = drm_atomic_get_crtc_state(state,
					       new_plane_state->crtc);
	if (IS_ERR(crtc_state))
		return PTR_ERR(crtc_state);

	return drm_atomic_helper_check_plane_state(new_plane_state,
						   crtc_state,
						   DRM_PLANE_HELPER_NO_SCALING,
						   DRM_PLANE_HELPER_NO_SCALING,
						   true, true);
}

/* Takes a fixed 16.16 number and converts it to integer. */
static inline int64_t fixed16_to_int(int64_t value)
{
	return value >> 16;
}

static void meson_cursor_atomic_update(struct drm_plane *plane,
				      struct drm_atomic_state *state)
{
	struct meson_cursor *meson_cursor = to_meson_cursor(plane);
	struct drm_plane_state *new_state = drm_atomic_get_new_plane_state(state,
									   plane);
	struct drm_rect dest = drm_plane_state_dest(new_state);
	struct meson_drm *priv = meson_cursor->priv;
	struct drm_framebuffer *fb = new_state->fb;
	struct drm_gem_cma_object *gem;
	unsigned long flags;
	int dst_w, dst_h;

	/*
	 * Update Coordinates
	 * Update Formats
	 * Update Buffer
	 * Enable Plane
	 */
	spin_lock_irqsave(&priv->drm->event_lock, flags);

	/* Enable OSD and BLK0, set max global alpha */
	priv->viu.osd2_ctrl_stat = OSD_ENABLE |
				   (0xFF << OSD_GLOBAL_ALPHA_SHIFT) |
				   OSD_BLK0_ENABLE;

	priv->viu.osd2_ctrl_stat2 = readl(priv->io_base +
					  _REG(VIU_OSD2_CTRL_STAT2));

	/* Set up BLK0 to point to the right canvas */
	priv->viu.osd2_blk0_cfg[0] = priv->canvas_id_osd2 << OSD_CANVAS_SEL;
	priv->viu.osd2_blk0_cfg[0] |= OSD_ENDIANNESS_LE;

	/* On GXBB, Use the old non-HDR RGB2YUV converter */
	if (meson_vpu_is_compatible(priv, VPU_COMPATIBLE_GXBB))
		priv->viu.osd2_blk0_cfg[0] |= OSD_OUTPUT_COLOR_RGB;

	switch (fb->format->format) {
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_ARGB8888:
		priv->viu.osd2_blk0_cfg[0] |= OSD_BLK_MODE_32 |
			OSD_COLOR_MATRIX_32_ARGB;
		break;
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_ABGR8888:
		priv->viu.osd2_blk0_cfg[0] |= OSD_BLK_MODE_32 |
			OSD_COLOR_MATRIX_32_ABGR;
		break;
	case DRM_FORMAT_RGB888:
		priv->viu.osd2_blk0_cfg[0] |= OSD_BLK_MODE_24 |
			OSD_COLOR_MATRIX_24_RGB;
		break;
	case DRM_FORMAT_RGB565:
		priv->viu.osd2_blk0_cfg[0] |= OSD_BLK_MODE_16 |
			OSD_COLOR_MATRIX_16_RGB565;
		break;
	}

	switch (fb->format->format) {
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_XBGR8888:
		/* For XRGB, replace the pixel's alpha by 0xFF */
		priv->viu.osd2_ctrl_stat2 |= OSD_REPLACE_EN;
		break;
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_ABGR8888:
		/* For ARGB, use the pixel's alpha */
		priv->viu.osd2_ctrl_stat2 &= ~OSD_REPLACE_EN;
		break;
	}

	dst_w = new_state->crtc_w;
	dst_h = new_state->crtc_h;

	if (new_state->crtc->mode.flags & DRM_MODE_FLAG_INTERLACE)
		priv->viu.osd2_interlace = true;
	else
		priv->viu.osd2_interlace = false;

	/*
	 * The format of these registers is (x2 << 16 | x1),
	 * where x2 is exclusive.
	 * e.g. +30x1920 would be (1919 << 16) | 30
	 */
	priv->viu.osd2_blk0_cfg[1] =
				((fixed16_to_int(new_state->src.x2) - 1) << 16) |
				fixed16_to_int(new_state->src.x1);
	priv->viu.osd2_blk0_cfg[2] =
				((fixed16_to_int(new_state->src.y2) - 1) << 16) |
				fixed16_to_int(new_state->src.y1);
	priv->viu.osd2_blk0_cfg[3] = ((dest.x2 - 1) << 16) | dest.x1;
	priv->viu.osd2_blk0_cfg[4] = ((dest.y2 - 1) << 16) | dest.y1;

	if (meson_vpu_is_compatible(priv, VPU_COMPATIBLE_G12A)) {
		priv->viu.osd_blend_din3_scope_h = ((dest.x2 - 1) << 16) | dest.x1;
		priv->viu.osd_blend_din3_scope_v = ((dest.y2 - 1) << 16) | dest.y1;
		priv->viu.osb_blend1_size = dst_h << 16 | dst_w;
	}

	/* Update Canvas with buffer address */
	gem = drm_fb_cma_get_gem_obj(fb, 0);

	priv->viu.osd2_addr = gem->paddr;
	priv->viu.osd2_stride = fb->pitches[0];
	priv->viu.osd2_height = fb->height;
	priv->viu.osd2_width = fb->width;

	/* TOFIX: Reset OSD2 before enabling it on GXL+ SoCs ? */

	priv->viu.osd2_enabled = true;

	spin_unlock_irqrestore(&priv->drm->event_lock, flags);
}

static void meson_cursor_atomic_disable(struct drm_plane *plane,
				       struct drm_atomic_state *state)
{
	struct meson_cursor *meson_cursor = to_meson_cursor(plane);
	struct meson_drm *priv = meson_cursor->priv;

	/* Disable OSD2 */
	if (meson_vpu_is_compatible(priv, VPU_COMPATIBLE_G12A))
		writel_bits_relaxed(OSD_BLEND_POSTBLD_SRC_OSD2, 0,
				    priv->io_base + _REG(OSD2_BLEND_SRC_CTRL));
	else
		writel_bits_relaxed(VPP_OSD2_POSTBLEND, 0,
				    priv->io_base + _REG(VPP_MISC));

	priv->viu.osd2_enabled = false;
}

static const struct drm_plane_helper_funcs meson_cursor_helper_funcs = {
	.atomic_check	= meson_cursor_atomic_check,
	.atomic_disable	= meson_cursor_atomic_disable,
	.atomic_update	= meson_cursor_atomic_update,
};

static const struct drm_plane_funcs meson_cursor_funcs = {
	.update_plane		= drm_atomic_helper_update_plane,
	.disable_plane		= drm_atomic_helper_disable_plane,
	.destroy		= drm_plane_cleanup,
	.reset			= drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_plane_destroy_state,
};

static const uint32_t supported_drm_formats[] = {
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_RGB565,
};

static const uint64_t format_modifiers_default[] = {
	DRM_FORMAT_MOD_LINEAR,
	DRM_FORMAT_MOD_INVALID,
};

int meson_cursor_create(struct meson_drm *priv)
{
	struct meson_cursor *meson_cursor;
	struct drm_plane *cursor;

	meson_cursor = devm_kzalloc(priv->drm->dev, sizeof(*meson_cursor),
				   GFP_KERNEL);
	if (!meson_cursor)
		return -ENOMEM;

	meson_cursor->priv = priv;
	cursor = &meson_cursor->base;

	drm_universal_plane_init(priv->drm, cursor, 0xFF,
				 &meson_cursor_funcs,
				 supported_drm_formats,
				 ARRAY_SIZE(supported_drm_formats),
				 format_modifiers_default,
				 DRM_PLANE_TYPE_CURSOR, "meson_cursor_plane");

	drm_plane_helper_add(cursor, &meson_cursor_helper_funcs);

	/* For now, OSD Cursor is always on top of the primary plane */
	drm_plane_create_zpos_immutable_property(cursor, 2);

	priv->cursor_plane = cursor;

	return 0;
}
