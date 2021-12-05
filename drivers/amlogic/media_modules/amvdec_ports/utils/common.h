#ifndef UTILS_COMMON_H
#define UTILS_COMMON_H

#include "pixfmt.h"

#define AV_INPUT_BUFFER_PADDING_SIZE	64
#define MIN_CACHE_BITS			64

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMAX3(a,b,c) FFMAX(FFMAX(a,b),c)
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFMIN3(a,b,c) FFMIN(FFMIN(a,b),c)

#define AV_WL32(p, val)				\
	do {					\
		u32 d = (val);			\
		((u8*)(p))[0] = (d);		\
		((u8*)(p))[1] = (d) >> 8;	\
		((u8*)(p))[2] = (d) >> 16;	\
		((u8*)(p))[3] = (d) >> 24;	\
	} while(0)

#define AV_WB32(p, val)				\
	do { u32 d = (val);			\
		((u8*)(p))[3] = (d);		\
		((u8*)(p))[2] = (d) >> 8;	\
		((u8*)(p))[1] = (d) >> 16;	\
		((u8*)(p))[0] = (d) >> 24;	\
	} while(0)

#define AV_RB32(x)				\
	(((u32)((const u8*)(x))[0] << 24) |	\
	(((const u8*)(x))[1] << 16) |		\
	(((const u8*)(x))[2] <<  8) |		\
	((const u8*)(x))[3])

#define AV_RL32(x)				\
	(((u32)((const u8*)(x))[3] << 24) |	\
	(((const u8*)(x))[2] << 16) |		\
	(((const u8*)(x))[1] <<  8) |		\
	((const u8*)(x))[0])

#define NEG_SSR32(a, s)   (((int)(a)) >> ((s < 32) ? (32 - (s)) : 0))
#define NEG_USR32(a, s)   (((u32)(a)) >> ((s < 32) ? (32 - (s)) : 0))

//rounded division & shift
#define RSHIFT(a,b) ((a) > 0 ? ((a) + ((1<<(b))>>1))>>(b) : ((a) + ((1<<(b))>>1)-1)>>(b))
/* assume b>0 */
#define ROUNDED_DIV(a,b) (((a)>0 ? (a) + ((b)>>1) : (a) - ((b)>>1))/(b))

struct AVRational{
	int num; ///< numerator
	int den; ///< denominator
};

//fmt
const char *av_color_space_name(enum AVColorSpace space);
const char *av_color_primaries_name(enum AVColorPrimaries primaries);
const char *av_color_transfer_name(enum AVColorTransferCharacteristic transfer);

//math
int av_log2(u32 v);

//bitstream
int find_start_code(u8 *data, int data_sz);
int calc_nal_len(u8 *data, int len);
u8 *nal_unit_extract_rbsp(const u8 *src, u32 src_len, u32 *dst_len);

//debug
void print_hex_debug(u8 *data, u32 len, int max);

#endif