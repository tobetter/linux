#include "firmware_type.h"

static const struct type_name_s type_name[] = {
	{VIDEO_DEC_MPEG12, "mpeg12"},
	{VIDEO_DEC_MPEG4_3, "divx311"},
	{VIDEO_DEC_MPEG4_4, "divx4x"},
	{VIDEO_DEC_MPEG4_5, "xvid"},
	{VIDEO_DEC_H263, "h263"},
	{VIDEO_DEC_MJPEG, "mjpeg"},
	{VIDEO_DEC_MJPEG_MULTI, "mjpeg_multi"},
	{VIDEO_DEC_REAL_V8, "real_v8"},
	{VIDEO_DEC_REAL_V9, "real_v9"},
	{VIDEO_DEC_VC1, "vc1"},
	{VIDEO_DEC_AVS, "avs"},
	{VIDEO_DEC_AVS_GXM, "avs_gxm"},
	{VIDEO_DEC_AVS_NOCABAC, "avs_no_cabac"},
	{VIDEO_DEC_H264, "h264"},
	{VIDEO_DEC_H264_4k2K, "h264_4k2k"},
	{VIDEO_DEC_H264_4k2K_SINGLE, "h264_4k2k_single"},
	{VIDEO_DEC_H264_MVC, "h264_mvc"},
	{VIDEO_DEC_H264_MVC_GXM, "h264_mvc_gxm"},
	{VIDEO_DEC_H264_MULTI, "h264_multi"},
	{VIDEO_DEC_H264_MULTI_MMU, "h264_multi_mmu"},
	{VIDEO_DEC_H264_MULTI_GXM, "h264_multi_gxm"},
	{VIDEO_DEC_HEVC, "hevc"},
	{VIDEO_DEC_HEVC_MMU, "hevc_mmu"},
	{VIDEO_DEC_HEVC_G12A, "hevc_g12a"},
	{VIDEO_DEC_VP9, "vp9"},
	{VIDEO_DEC_VP9_MMU, "vp9_mmu"},
	{VIDEO_DEC_VP9_G12A, "vp9_g12a"},
	{VIDEO_DEC_AVS2, "avs2"},
	{VIDEO_DEC_AVS2_MMU, "avs2_mmu"},
	{VIDEO_ENC_H264, "h264_enc"},
	{VIDEO_ENC_JPEG, "jpeg_enc"},
	{FIRMWARE_MAX, "unknown"},
};


const char *get_firmware_type_name(enum firmware_type_e type)
{
	const char *name = "unknown";
	int i, size = ARRAY_SIZE(type_name);

	for (i = 0; i < size; i++) {
		if (type == type_name[i].type)
			name = type_name[i].name;
	}

	return name;
}
EXPORT_SYMBOL(get_firmware_type_name);

enum firmware_type_e get_firmware_type(const char *name)
{
	enum firmware_type_e type = FIRMWARE_MAX;
	int i, size = ARRAY_SIZE(type_name);

	for (i = 0; i < size; i++) {
		if (!strcmp(name, type_name[i].name))
			type = type_name[i].type;
	}

	return type;
}
EXPORT_SYMBOL(get_firmware_type);

