# Overlays for the Odroid platform

dtbo-$(CONFIG_ARCH_MESON64_ODROIDC4) += spi0.dtbo \
					i2c0.dtbo \
					i2c1.dtbo \
					pwm_ab.dtbo \
					pwm_cd.dtbo \
					pwm_cd-c.dtbo \
					pwm_ef.dtbo \
					pwm_ef-f.dtbo \
					pwm_b-backlight.dtbo \
					pwm_c-beeper.dtbo \
					sdio.dtbo \
					uart1.dtbo \
					uart1_cts_rts.dtbo \
					uart2.dtbo \
					ds3231.dtbo \
					waveshare_tft35c_lcd.dtbo \
					waveshare_tft35c_rtp.dtbo \
					waveshare_tft24_lcd.dtbo \
					custom_ir.dtbo \
					hifi_pcm5102a.dtbo \
					hifi_pcm5122.dtbo

targets	+= $(dtbo-y)
always	:= $(dtbo-y)
