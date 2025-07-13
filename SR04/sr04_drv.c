#include <linux/module.h>
#include <linux/poll.h>

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <asm/current.h>
#include <linux/delay.h>

static int major;
static struct class *sr04_class;
static struct gpio_desc *sr04_trig;
static struct gpio_desc *sr04_echo;
static int irq;
static u64 sr04_data_ns = 0;  
static wait_queue_head_t sr04_wq;

static ssize_t sr04_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{	
	int timeout;

	gpiod_set_value(sr04_trig, 1);
	udelay(15);
	gpiod_set_value(sr04_trig, 0);

	timeout = wait_event_interruptible_timeout(sr04_wq, sr04_data_ns, HZ);	
	if (timeout)
	{
		copy_to_user(buf, &sr04_data_ns, 4);
		sr04_data_ns = 0;
		return 4;
	}
	else
	{
		return -EAGAIN;
	}
	
}

static unsigned int sr04_drv_poll(struct file *fp, poll_table * wait)
{
	return 0;
}


static struct file_operations sr04_fops = {
	.owner	 = THIS_MODULE,
	.read    = sr04_drv_read,
	.poll    = sr04_drv_poll,
};


static irqreturn_t sr04_isr(int irq, void *dev_id)
{
	int val = gpiod_get_value(sr04_echo);

	if (val)
	{
		sr04_data_ns = ktime_get_ns();
	}
	else
	{
		sr04_data_ns = ktime_get_ns() - sr04_data_ns;
		wake_up(&sr04_wq);
	}
	
	return IRQ_HANDLED;
}


static int sr04_probe(struct platform_device *pdev)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

	sr04_trig = gpiod_get(&pdev->dev, "trig", GPIOD_OUT_LOW);
	sr04_echo = gpiod_get(&pdev->dev, "echo", GPIOD_IN);

	irq = gpiod_to_irq(sr04_echo);

	request_irq(irq, sr04_isr, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, "sr04", NULL);

	device_create(sr04_class, NULL, MKDEV(major, 0), NULL, "sr04");

	return 0;
}

static int sr04_remove(struct platform_device *pdev)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	device_destroy(sr04_class, MKDEV(major, 0));
	free_irq(irq, NULL);
	gpiod_put(sr04_trig);
	gpiod_put(sr04_echo);
	return 0;
}


static const struct of_device_id ask100_sr04[] = {
    { .compatible = "100ask,sr04" },
    { },
};

static struct platform_driver sr04s_driver = {
    .probe      = sr04_probe,
    .remove     = sr04_remove,
    .driver     = {
        .name   = "100ask_sr04",
        .of_match_table = ask100_sr04,
    },
};

static int __init sr04_init(void)
{
    int err;
    
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

	major = register_chrdev(0, "sr04", &sr04_fops);  

	sr04_class = class_create(THIS_MODULE, "sr04_class");
	if (IS_ERR(sr04_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "sr04");
		return PTR_ERR(sr04_class);
	}

	init_waitqueue_head(&sr04_wq);

	
    err = platform_driver_register(&sr04s_driver); 
	
	return err;
}

static void __exit sr04_exit(void)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    platform_driver_unregister(&sr04s_driver);
	class_destroy(sr04_class);
	unregister_chrdev(major, "sr04");
}

module_init(sr04_init);
module_exit(sr04_exit);

MODULE_LICENSE("GPL");


