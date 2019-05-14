/* ir-irmp-decoder.c - handle IR Pulse/Space by IRMP multiprotocol decoder
 *
 * Copyright (C) 2018 by Team CoreELEC
 *
 * This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

//#include <linux/bitrev.h>
#include <linux/module.h>
#include "rc-core-priv.h"
#define IRMP_PULSE_IR_DECODER
#define U_BOOT_COMPATIBLE
#define IRMP_PROTOCOL_NAMES 1
#ifdef unix
#undef unix
#endif
#include "irmp/irmp.h"
#include "irmp/irmp.c"

/**
 * ir_irmp_decode() - Decode one IRMP pulse or space
 * @dev:	the struct rc_dev descriptor of the device
 * @duration:	the struct ir_raw_event descriptor of the pulse/space
 *
 * This function returns -EINVAL if the pulse violates the state machine
 */
static int ir_irmp_decode(struct rc_dev *dev, struct ir_raw_event ev)
{
	u32 scancode;
	IRMP_DATA irmp_data;
	unsigned int pulse = TO_US(ev.duration);
	u8 protocol = 0xFF;

	if (!is_timing_event(ev)) {
		if (ev.reset)
			pulse = 0;
	}

	pulse = pulse > IRMP_TIMEOUT_TIME_MS ? 0 : pulse;

	IR_dprintk(2, "IRMP decode %uus (%uus): %s\n",
		   pulse, TO_US(ev.duration), TO_STR(ev.pulse));

	// ready with decode
	if (irmp_ISR(pulse ? ((pulse * 15) / 1000) : 0)) {
		if (irmp_get_data(&irmp_data)) {
			IR_dprintk(1, "IRMP decoded by protocol %s, command %04x, address %04x",
				irmp_protocol_names[irmp_data.protocol], irmp_data.command, irmp_data.address);

			switch (irmp_data.protocol) {
				case IRMP_NEC_PROTOCOL:
				case IRMP_SAMSUNG_PROTOCOL:
				case IRMP_SAMSUNG32_PROTOCOL:
					protocol = 0x0;
					break;
				case IRMP_RC5_PROTOCOL:
					protocol = 0x4;
					break;
				case IRMP_RC6_PROTOCOL:
					protocol = 0xb;
					break;
				case IRMP_RC6A_PROTOCOL:
					protocol = 0x5;
					break;
			}
			// transfer decoded protocol
			if (protocol != 0xFF)
				scancode = 0xA0A0A000 | protocol;
			else
				scancode = 0xB0B0B000 | irmp_data.protocol;

			// transfer decoded ir protocol
			rc_keydown(dev, RC_TYPE_IRMP, scancode, 0);

			switch (irmp_data.protocol) {
				case IRMP_RC5_PROTOCOL:
					scancode = 0x3000 | (((unsigned int)(irmp_data.address) & 0x7F) << 6) | (irmp_data.command & 0x3F);
					break;
				case IRMP_RC6_PROTOCOL:
					scancode = (((unsigned int)(irmp_data.address) & 0x1FFF) << 8) | irmp_data.command;
					break;
				case IRMP_RC6A_PROTOCOL:
					scancode = ((unsigned int)(irmp_data.address) << 16) | irmp_data.command;
					break;
				default:
					scancode = ((unsigned int)(irmp_data.command) << 16) | irmp_data.address;
					break;
			}

			// transfer decoded command/address
			rc_keydown(dev, RC_TYPE_IRMP, scancode, 0);

			switch (irmp_data.protocol) {
				case IRMP_RC5_PROTOCOL:
					scancode = 0x37FF;
					break;
				case IRMP_RC6_PROTOCOL:
					scancode = 0x1EFFFF;
					break;
				case IRMP_RC6A_PROTOCOL:
					scancode = 0xFFFF7FFF;
					break;
				default:
					scancode = 0xFFFFFFFF;
					break;
			}

			// transfer decoding mask
			rc_keydown(dev, RC_TYPE_IRMP, scancode, 0);
		}
	}

	return 0;
}

static struct ir_raw_handler irmp_handler = {
	.protocols	= RC_BIT_IRMP,
	.decode		= ir_irmp_decode,
};

static int __init ir_irmp_decode_init(void)
{
	ir_raw_handler_register(&irmp_handler);

	printk(KERN_INFO "IR IRMP decoder handler initialized\n");
	return 0;
}

static void __exit ir_irmp_decode_exit(void)
{
	ir_raw_handler_unregister(&irmp_handler);
}

module_init(ir_irmp_decode_init);
module_exit(ir_irmp_decode_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hugo Portisch");
MODULE_AUTHOR("Team CoreELEC");
MODULE_DESCRIPTION("IRMP IR protocol decoder");
