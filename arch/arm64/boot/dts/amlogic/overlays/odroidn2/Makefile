# Overlays for the Odroid platform

dtbo-$(CONFIG_ARCH_MESON64_ODROIDN2) += spi0.dtbo		\
					i2c0.dtbo		\
					i2c1.dtbo		\
					uart0.dtbo		\
					uart1.dtbo		\
					hktft32.dtbo		\
					hktft35.dtbo		\
					ads7846.dtbo		\
					sx865x-i2c1.dtbo	\
					saradc.dtbo		\
					irblaster.dtbo		\
					onewire.dtbo		\
					can0.dtbo		\
					fanspeed-full.dtbo	\
					gpio_spdif.dtbo	\
					pwm_cd.dtbo		\
					pwm_ef.dtbo		\
					pwm_c-pwmfan.dtbo	\
					pwm_d-backlight.dtbo

targets	+= $(dtbo-y)
always	:= $(dtbo-y)
