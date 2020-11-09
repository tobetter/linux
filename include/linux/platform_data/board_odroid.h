#ifndef __BOARD_ODROID_H
#define __BOARD_ODROID_H

#include <linux/types.h>

#define BOARD_ODROIDN2			0x09221000
#define BOARD_ODROIDC4			0x09051000
#define BOARD_ODROIDHC4			0x09052000

#if defined(CONFIG_ARCH_MESON64_ODROID_COMMON)

extern u32 odroid_model(void);
extern bool odroid_amlogic_usb3(void);

#define __board_is_odroid(x)	(odroid_model() == (x))
#else
#define __board_is_odroid(x)	false
#define odroid_amlogic_usb3(void)	false
#endif

#define board_is_odroidn2()	__board_is_odroid(BOARD_ODROIDN2)
#define board_is_odroidc4()	__board_is_odroid(BOARD_ODROIDC4)
#define board_is_odroidhc4()	__board_is_odroid(BOARD_ODROIDHC4)

#endif
