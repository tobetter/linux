/*
 * drivers/amlogic/media/common/firmware/firmware_cfg.h
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

/*all firmwares in one bin.*/
{MESON_CPU_MAJOR_ID_GXBB, VIDEO_PACKAGE, "video_ucode.bin"},
{MESON_CPU_MAJOR_ID_GXTVBB, VIDEO_PACKAGE, "video_ucode.bin"},
{MESON_CPU_MAJOR_ID_GXL, VIDEO_PACKAGE, "video_ucode.bin"},
{MESON_CPU_MAJOR_ID_GXM, VIDEO_PACKAGE, "video_ucode.bin"},
{MESON_CPU_MAJOR_ID_TXL, VIDEO_PACKAGE, "video_ucode.bin"},
{MESON_CPU_MAJOR_ID_TXLX, VIDEO_PACKAGE, "video_ucode.bin"},
{MESON_CPU_MAJOR_ID_G12A, VIDEO_PACKAGE, "video_ucode.bin"},

/* Note: if the addition of new package has the same name */
/* as the firmware in the video_ucode.bin, the firmware */
/* in the video_ucode.bin will be ignored yet, because the */
/* video_ucode.bin will always be processed in the end */
{MESON_CPU_MAJOR_ID_GXM, VIDEO_PACKAGE, "h264_enc.bin"},
{MESON_CPU_MAJOR_ID_GXL, VIDEO_PACKAGE, "h264_enc.bin"},
{MESON_CPU_MAJOR_ID_TXLX, VIDEO_PACKAGE, "h264_enc.bin"},
{MESON_CPU_MAJOR_ID_TXL, VIDEO_PACKAGE, "h264_enc.bin"},
{MESON_CPU_MAJOR_ID_G12A, VIDEO_PACKAGE, "h264_enc.bin"},

/*firmware for a special format, to replace the format in the package.*/
{MESON_CPU_MAJOR_ID_GXL, VIDEO_DEC_HEVC, "h265.bin"},
{MESON_CPU_MAJOR_ID_GXL, VIDEO_DEC_H264, "h264.bin"},
{MESON_CPU_MAJOR_ID_GXL, VIDEO_DEC_H264_MULTI, "h264_multi.bin"},


