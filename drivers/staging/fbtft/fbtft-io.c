#include <linux/export.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include "fbtft.h"

#if defined(CONFIG_ARCH_MESON64_ODROID_COMMON)
unsigned int force64b_enable;
EXPORT_SYMBOL(force64b_enable);
#endif

int fbtft_write_spi(struct fbtft_par *par, void *buf, size_t len)
{
	struct spi_transfer t = {
		.tx_buf = buf,
		.len = len,
	};
	struct spi_message m;

	fbtft_par_dbg_hex(DEBUG_WRITE, par, par->info->device, u8, buf, len,
		"%s(len=%d): ", __func__, len);

#if defined(CONFIG_ARCH_MESON64_ODROID_COMMON)
	if (force64b_enable && ((len >= 64) && ((len % 8) == 0))) {
		unsigned int i;
		unsigned char tmp;
		unsigned char *txbuf = (unsigned char *)buf;

		for (i = 0; i < len; i += 8) {
			tmp = txbuf[i + 0]; txbuf[i + 0] = txbuf[i + 7];
			txbuf[i + 7] = tmp;
			tmp = txbuf[i + 1]; txbuf[i + 1] = txbuf[i + 6];
			txbuf[i + 6] = tmp;
			tmp = txbuf[i + 2]; txbuf[i + 2] = txbuf[i + 5];
			txbuf[i + 5] = tmp;
			tmp = txbuf[i + 3]; txbuf[i + 3] = txbuf[i + 4];
			txbuf[i + 4] = tmp;
		}
	}
#endif
	if (!par->spi) {
		dev_err(par->info->device,
			"%s: par->spi is unexpectedly NULL\n", __func__);
		return -1;
	}

	spi_message_init(&m);
	if (par->txbuf.dma && buf == par->txbuf.buf) {
		t.tx_dma = par->txbuf.dma;
		m.is_dma_mapped = 1;
	}
	spi_message_add_tail(&t, &m);
	return spi_sync(par->spi, &m);
}
EXPORT_SYMBOL(fbtft_write_spi);

/**
 * fbtft_write_spi_emulate_9() - write SPI emulating 9-bit
 * @par: Driver data
 * @buf: Buffer to write
 * @len: Length of buffer (must be divisible by 8)
 *
 * When 9-bit SPI is not available, this function can be used to emulate that.
 * par->extra must hold a transformation buffer used for transfer.
 */
int fbtft_write_spi_emulate_9(struct fbtft_par *par, void *buf, size_t len)
{
	u16 *src = buf;
	u8 *dst = par->extra;
	size_t size = len / 2;
	size_t added = 0;
	int bits, i, j;
	u64 val, dc, tmp;

	fbtft_par_dbg_hex(DEBUG_WRITE, par, par->info->device, u8, buf, len,
		"%s(len=%d): ", __func__, len);

	if (!par->extra) {
		dev_err(par->info->device, "%s: error: par->extra is NULL\n",
			__func__);
		return -EINVAL;
	}
	if ((len % 8) != 0) {
		dev_err(par->info->device,
			"error: len=%zu must be divisible by 8\n", len);
		return -EINVAL;
	}

	for (i = 0; i < size; i += 8) {
		tmp = 0;
		bits = 63;
		for (j = 0; j < 7; j++) {
			dc = (*src & 0x0100) ? 1 : 0;
			val = *src & 0x00FF;
			tmp |= dc << bits;
			bits -= 8;
			tmp |= val << bits--;
			src++;
		}
		tmp |= ((*src & 0x0100) ? 1 : 0);
		*(u64 *)dst = cpu_to_be64(tmp);
		dst += 8;
		*dst++ = (u8)(*src++ & 0x00FF);
		added++;
	}

	return spi_write(par->spi, par->extra, size + added);
}
EXPORT_SYMBOL(fbtft_write_spi_emulate_9);

int fbtft_read_spi(struct fbtft_par *par, void *buf, size_t len)
{
	int ret;
	u8 txbuf[32] = { 0, };
	struct spi_transfer	t = {
			.speed_hz = 2000000,
			.rx_buf		= buf,
			.len		= len,
		};
	struct spi_message	m;

	if (!par->spi) {
		dev_err(par->info->device,
			"%s: par->spi is unexpectedly NULL\n", __func__);
		return -ENODEV;
	}

	if (par->startbyte) {
		if (len > 32) {
			dev_err(par->info->device,
				"len=%zu can't be larger than 32 when using 'startbyte'\n",
				len);
			return -EINVAL;
		}
		txbuf[0] = par->startbyte | 0x3;
		t.tx_buf = txbuf;
		fbtft_par_dbg_hex(DEBUG_READ, par, par->info->device, u8,
			txbuf, len, "%s(len=%d) txbuf => ", __func__, len);
	}

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	ret = spi_sync(par->spi, &m);
	fbtft_par_dbg_hex(DEBUG_READ, par, par->info->device, u8, buf, len,
		"%s(len=%d) buf <= ", __func__, len);

	return ret;
}
EXPORT_SYMBOL(fbtft_read_spi);

/*
 * Optimized use of gpiolib is twice as fast as no optimization
 * only one driver can use the optimized version at a time
 */
int fbtft_write_gpio8_wr(struct fbtft_par *par, void *buf, size_t len)
{
	u8 data;
	int i;
#ifndef DO_NOT_OPTIMIZE_FBTFT_WRITE_GPIO
	static u8 prev_data;
#endif

	fbtft_par_dbg_hex(DEBUG_WRITE, par, par->info->device, u8, buf, len,
		"%s(len=%d): ", __func__, len);

	while (len--) {
		data = *(u8 *)buf;

		/* Start writing by pulling down /WR */
		gpio_set_value(par->gpio.wr, 0);

		/* Set data */
#ifndef DO_NOT_OPTIMIZE_FBTFT_WRITE_GPIO
		if (data == prev_data) {
			gpio_set_value(par->gpio.wr, 0); /* used as delay */
		} else {
			for (i = 0; i < 8; i++) {
				if ((data & 1) != (prev_data & 1))
					gpio_set_value(par->gpio.db[i],
								data & 1);
				data >>= 1;
				prev_data >>= 1;
			}
		}
#else
		for (i = 0; i < 8; i++) {
			gpio_set_value(par->gpio.db[i], data & 1);
			data >>= 1;
		}
#endif

		/* Pullup /WR */
		gpio_set_value(par->gpio.wr, 1);

#ifndef DO_NOT_OPTIMIZE_FBTFT_WRITE_GPIO
		prev_data = *(u8 *)buf;
#endif
		buf++;
	}

	return 0;
}
EXPORT_SYMBOL(fbtft_write_gpio8_wr);

int fbtft_write_gpio16_wr(struct fbtft_par *par, void *buf, size_t len)
{
	u16 data;
	int i;
#ifndef DO_NOT_OPTIMIZE_FBTFT_WRITE_GPIO
	static u16 prev_data;
#endif

	fbtft_par_dbg_hex(DEBUG_WRITE, par, par->info->device, u8, buf, len,
		"%s(len=%d): ", __func__, len);

	while (len) {
		data = *(u16 *)buf;

		/* Start writing by pulling down /WR */
		gpio_set_value(par->gpio.wr, 0);

		/* Set data */
#ifndef DO_NOT_OPTIMIZE_FBTFT_WRITE_GPIO
		if (data == prev_data) {
			gpio_set_value(par->gpio.wr, 0); /* used as delay */
		} else {
			for (i = 0; i < 16; i++) {
				if ((data & 1) != (prev_data & 1))
					gpio_set_value(par->gpio.db[i],
								data & 1);
				data >>= 1;
				prev_data >>= 1;
			}
		}
#else
		for (i = 0; i < 16; i++) {
			gpio_set_value(par->gpio.db[i], data & 1);
			data >>= 1;
		}
#endif

		/* Pullup /WR */
		gpio_set_value(par->gpio.wr, 1);

#ifndef DO_NOT_OPTIMIZE_FBTFT_WRITE_GPIO
		prev_data = *(u16 *)buf;
#endif
		buf += 2;
		len -= 2;
	}

	return 0;
}
EXPORT_SYMBOL(fbtft_write_gpio16_wr);

int fbtft_write_gpio16_wr_latched(struct fbtft_par *par, void *buf, size_t len)
{
	dev_err(par->info->device, "%s: function not implemented\n", __func__);
	return -1;
}
EXPORT_SYMBOL(fbtft_write_gpio16_wr_latched);

#if defined(CONFIG_ARCH_MESON64_ODROID_COMMON)
union	reg_bitfield {
	unsigned int	wvalue;
	struct {
		unsigned int	db02: 1;	/* GPIOX.0 */
		unsigned int	db00: 1;	/* GPIOX.1 */
		unsigned int	db01: 1;	/* GPIOX.2 */
		unsigned int	bit3: 1;	/* GPIOX.3 */
		unsigned int	db07: 1;	/* GPIOX.4 */
		unsigned int	bit5: 1;	/* GPIOX.5 */
		unsigned int	bit6: 1;	/* GPIOX.6 */
		unsigned int	db06: 1;	/* GPIOX.7 */
		unsigned int	db05: 1;	/* GPIOX.8 */
		unsigned int	db04: 1;	/* GPIOX.9 */
		unsigned int	wr: 1;		/* GPIOX.10 */
		unsigned int	db03: 1;	/* GPIOX.11 */
		/* GPIOX.12 ~ GPIOX.31 */
		unsigned int	bit12_bit31 : 20;
	} bits;
};

int fbtft_write_reg_wr(struct fbtft_par *par, void *buf, size_t len)
{
	u8	data;
	union	reg_bitfield	dbus;

	if (!par->reg_gpiox) {
		pr_err("%s : ioremap gpio register fail!\n", __func__);
		return	0;
	}

	dbus.wvalue = ioread32(par->reg_gpiox + OFFSET_GPIOX_IN);

	while (len--) {
		data = *(u8 *)buf;
		dbus.bits.db00 = (data & 0x01) ? 1 : 0;
		dbus.bits.db01 = (data & 0x02) ? 1 : 0;
		dbus.bits.db02 = (data & 0x04) ? 1 : 0;
		dbus.bits.db03 = (data & 0x08) ? 1 : 0;
		dbus.bits.db04 = (data & 0x10) ? 1 : 0;
		dbus.bits.db05 = (data & 0x20) ? 1 : 0;
		dbus.bits.db06 = (data & 0x40) ? 1 : 0;
		dbus.bits.db07 = (data & 0x80) ? 1 : 0;
		/* Start writing by pulling down /WR */
		dbus.bits.wr = 0;
		iowrite32(dbus.wvalue, par->reg_gpiox + OFFSET_GPIOX_OUT);
		dbus.bits.wr = 1;
		iowrite32(dbus.wvalue, par->reg_gpiox + OFFSET_GPIOX_OUT);

		buf++;
	}

	return 0;
}
EXPORT_SYMBOL(fbtft_write_reg_wr);

#endif /* #if defined(CONFIG_ARCH_MESON64_ODROID_COMMON) */
