/* Low-level parallel port routines for ODROID GPIO
 *
 * Author: Dongjin Kim <tobetter@gmail.com>
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/parport.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif

struct parport_odroid_private {
	int nr_data_gpios;
	struct gpio_desc *data_gpios[8];
	char *data_label[8];

	int nr_status_gpios;
	struct gpio_desc *status_gpios[5];
	char *status_label[8];
};

static char *status_label[5] = {
	"error", "select", "paperout", "ack", "busy",
};
static const unsigned char status_mask[] = {
	PARPORT_STATUS_ERROR,
	PARPORT_STATUS_SELECT,
	PARPORT_STATUS_PAPEROUT,
	PARPORT_STATUS_ACK,
	PARPORT_STATUS_BUSY
};

static void odroid_write_data(struct parport *p, unsigned char data)
{
	int i;
	unsigned char mask = 1;
	struct parport_odroid_private *priv = p->private_data;

	for (i = 0; i < priv->nr_data_gpios; i++, mask <<= 1)
		gpiod_direction_output(priv->data_gpios[i], !!(data & mask));
}

static unsigned char odroid_read_data(struct parport *p)
{
	int i;
	int val;
	unsigned char mask = 1;
	unsigned char ret = 0;
	struct parport_odroid_private *priv = p->private_data;

	for (i = 0; i < priv->nr_data_gpios; i++, mask <<= 1) {
		gpiod_direction_input(priv->data_gpios[i]);
		val = gpiod_get_value_cansleep(priv->data_gpios[i]);
		if (!val)
			ret |= mask;
	}

	return ret;
}

static unsigned char control_odroid_to_pc(unsigned char control)
{
	return PARPORT_CONTROL_SELECT |
	      PARPORT_CONTROL_AUTOFD | PARPORT_CONTROL_STROBE;
	/* fake value: interrupt enable, select in, no reset,
	no autolf, no strobe - seems to be closest the wiring diagram */
}

static void odroid_write_control(struct parport *p, unsigned char control)
{
	/* No implementation possible */
}

static unsigned char odroid_read_control( struct parport *p)
{
	return control_odroid_to_pc(0);
}

static void odroid_init_state(struct pardevice *dev, struct parport_state *s)
{
}

static void odroid_save_state(struct parport *p, struct parport_state *s)
{
}

static void odroid_restore_state(struct parport *p, struct parport_state *s)
{
}

static unsigned char odroid_frob_control(struct parport *p,
		unsigned char mask, unsigned char val)
{
	return 0;
}

static unsigned char odroid_read_status(struct parport *p)
{
	struct parport_odroid_private *priv = p->private_data;
	unsigned char status = 0;
	int i;

	for (i = 0; i < priv->nr_status_gpios; i++) {
		if (gpiod_get_value_cansleep(priv->status_gpios[i]))
			status |= status_mask[i];
	}

	return status;
}

static void odroid_enable_irq(struct parport *p)
{
}

static void odroid_disable_irq(struct parport *p)
{
}

static void odroid_data_forward(struct parport *p)
{
}

static void odroid_data_reverse(struct parport *p)
{
}

static struct parport_operations pp_odroid_ops = {
	.write_data	= odroid_write_data,
	.read_data	= odroid_read_data,

	.write_control	= odroid_write_control,
	.read_control	= odroid_read_control,

	.init_state	= odroid_init_state,
	.save_state	= odroid_save_state,
	.restore_state	= odroid_restore_state,

	.frob_control	= odroid_frob_control,
	.read_status	= odroid_read_status,

	.enable_irq	= odroid_enable_irq,
	.disable_irq	= odroid_disable_irq,

	.data_forward	= odroid_data_forward,
	.data_reverse	= odroid_data_reverse,

	.epp_write_data	= parport_ieee1284_epp_write_data,
	.epp_read_data	= parport_ieee1284_epp_read_data,
	.epp_write_addr	= parport_ieee1284_epp_write_addr,
	.epp_read_addr	= parport_ieee1284_epp_read_addr,

	.ecp_write_data	= parport_ieee1284_ecp_write_data,
	.ecp_read_data	= parport_ieee1284_ecp_read_data,
	.ecp_write_addr	= parport_ieee1284_ecp_write_addr,

	.compat_write_data	= parport_ieee1284_write_compat,
	.nibble_read_data	= parport_ieee1284_read_nibble,
	.byte_read_data		= parport_ieee1284_read_byte,

	.owner		= THIS_MODULE,
};

static void odroid_free_datas(struct platform_device *pdev)
{
	struct parport *port = platform_get_drvdata(pdev);
	struct parport_odroid_private *priv = port->private_data;
	int i;

	for (i = 0; i < priv->nr_data_gpios; i++) {
		if (priv->data_gpios[i])
			gpiochip_free_own_desc(priv->data_gpios[i]);
	}
}

static void odroid_free_status(struct platform_device *pdev)
{
	struct parport *port = platform_get_drvdata(pdev);
	struct parport_odroid_private *priv = port->private_data;
	int i;

	for (i = 0; i < priv->nr_data_gpios; i++) {
		if (priv->status_gpios[i])
			gpiochip_free_own_desc(priv->status_gpios[i]);
	}
}

static int odroid_parallel_probe(struct platform_device *pdev)
{
	struct parport *p;
	struct device_node *np = pdev->dev.of_node;
	struct parport_odroid_private *priv;
	struct gpio_desc *desc;
	char label[64];
	int i;
	int err;

	priv = kzalloc(sizeof(struct parport_odroid_private), GFP_KERNEL);
	if (!priv) {
		printk(KERN_DEBUG "parport_odroid: no memory!\n");
		return -ENOMEM;
	}

	priv->nr_data_gpios = of_gpio_named_count(np, "data-gpios");
	if (priv->nr_data_gpios != 8) {
		dev_err(&pdev->dev, "parport: invalid data ports");
		err = -EINVAL;
		goto err_desc;
	}

	for (i = 0; i < priv->nr_data_gpios; i++) {
		int gpio = of_get_named_gpio(np, "data-gpios", i);
		if (gpio < 0) {
			/* FIXME */
			break;
		}

		desc = gpio_to_desc(gpio);

		snprintf(label, sizeof(label), "parport:data%d", i);
		priv->data_label[i] = kstrdup(label, GFP_KERNEL);

		devm_gpio_request(&pdev->dev, gpio, priv->data_label[i]);
		gpiod_direction_input(desc);
		gpiod_set_pull(desc, GPIOD_PULL_UP);

		priv->data_gpios[i] = desc;
	};

	priv->nr_status_gpios = of_gpio_named_count(np, "status-gpios");
	if (priv->nr_status_gpios > ARRAY_SIZE(priv->status_gpios))
		priv->nr_status_gpios = ARRAY_SIZE(priv->status_gpios);

	for (i = 0; i < priv->nr_status_gpios; i++) {
		int gpio = of_get_named_gpio(np, "status-gpios", i);

		if (gpio < 0) {
			err = -EINVAL;
			goto err_gpio_status;
		}

		desc = gpio_to_desc(gpio);

		snprintf(label, sizeof(label), "parport:%s", status_label[i]);
		priv->status_label[i] = kstrdup(label, GFP_KERNEL);

		devm_gpio_request(&pdev->dev, gpio, priv->status_label[i]);
		gpiod_direction_input(desc);
		gpiod_set_pull(desc, GPIOD_PULL_UP);

		priv->status_gpios[i] = desc;
	};

	p = parport_register_port(0x308, 0,
			PARPORT_DMA_NONE, &pp_odroid_ops);
	if (!p) {
		err = -EINVAL;
		goto err_register;
	}

	p->private_data = priv;

	parport_announce_port(p);

	platform_set_drvdata(pdev, p);

	return 0;

err_register:
	odroid_free_status(pdev);

err_gpio_status:
	odroid_free_datas(pdev);

err_desc:
	kfree(priv);
	return err;
}

static int odroid_parallel_remove(struct platform_device *pdev)
{
	struct parport *port = platform_get_drvdata(pdev);
	struct parport_odroid_private *priv = port->private_data;

	parport_remove_port(port);
	parport_put_port(port);
	odroid_free_status(pdev);
	odroid_free_datas(pdev);
	kfree(priv);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id parport_dt_ids[] = {
	{
		.compatible = "parport,odroid",
	},
	{},
};

MODULE_DEVICE_TABLE(of, parport_dt_ids);
#endif

static struct platform_driver odroid_parallel_driver = {
	.driver   = {
		.name	= "odroid-parallel",
#ifdef CONFIG_OF
		.of_match_table = parport_dt_ids,
#endif
	},
	.probe	= odroid_parallel_probe,
	.remove = odroid_parallel_remove,
};

module_platform_driver(odroid_parallel_driver);

MODULE_AUTHOR("Dongjin Kim <tobetter@gmail.com>");
MODULE_DESCRIPTION("Parport Driver for ODROID GPIO");
MODULE_SUPPORTED_DEVICE("ODROID GPIO Parallel Port");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:odroid-parallel");
