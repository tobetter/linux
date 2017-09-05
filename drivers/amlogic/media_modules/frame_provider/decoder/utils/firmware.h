#ifndef __VIDEO_FIRMWARE_HEADER_
#define __VIDEO_FIRMWARE_HEADER_

#include "../../../common/firmware/firmware_type.h"
#include <linux/amlogic/media/utils/vformat.h>

struct firmware_s {
	unsigned int len;
	char data[0];
};

extern int get_decoder_firmware_data(enum vformat_e type,
	const char *file_name, char *buf, int size);
extern int get_data_from_name(const char *name, char *buf);
extern int get_firmware_data(enum firmware_type_e type, char *buf);

#endif
