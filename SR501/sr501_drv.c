#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/wait.h>
#include <linux/poll.h>

static int major;
static struct class *sr501_class;
static struct gpio_desc *sr501_gpio;
static int irq;
static int sr501_data = 0; // Used to pass data between ISR and the read function
static wait_queue_head_t sr501_wq; // Define a wait queue head


/* Implementation of the read function */
static ssize_t sr501_drv_read(struct file *file, char __user *buf, size_t size, loff_t *offset) {
    int len = (size < 4) ? size : 4;
    int ret;
    /* Wait for the condition sr501_data to become true */
    wait_event_interruptible(sr501_wq, sr501_data);

    /* Condition met, copy data to user space */
    ret = copy_to_user(buf, &sr501_data, len);
    if (ret) {
        printk("copy_to_user failed\n");
        return -EFAULT; // Return "Bad address" error code
    }
    
    /* Reset the data flag */
    sr501_data = 0;

    return len;
}

/* Define the file_operations structure */
static struct file_operations sr501_fops = {
    .owner = THIS_MODULE,
    .read  = sr501_drv_read,
};

/* ISR: Wake up the process */
static irqreturn_t sr501_isr(int irq, void *dev_id) {
    printk("Interrupt occurred! Waking up readers...\n");
    sr501_data = 1;
    wake_up_interruptible(&sr501_wq); // Wake up processes waiting on sr501_wq
    return IRQ_HANDLED;
}

static int sr501_probe(struct platform_device *pdev) {
    printk("sr501_probe called\n");

    /* 1. Get GPIO from device tree */
    sr501_gpio = gpiod_get(&pdev->dev, NULL, 0); // "NULL" means get the GPIO at index 0
    if (IS_ERR(sr501_gpio)) {
        printk("Failed to get GPIO\n");
        return PTR_ERR(sr501_gpio);
    }

    /* 2. Set GPIO as input */
    gpiod_direction_input(sr501_gpio);

    irq = gpiod_to_irq(sr501_gpio);
    if (request_irq(irq, sr501_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "sr501", NULL)) {
        printk("Failed to request IRQ %d\n", irq); // Print error message
        gpiod_put(sr501_gpio); // Release the already requested GPIO
        return -EBUSY;         // Return an error code
    }

    /* 3. Create the device node */
    device_create(sr501_class, NULL, MKDEV(major, 0), NULL, "sr501");

    return 0;
}

static int sr501_remove(struct platform_device *pdev) {
    free_irq(irq, NULL); // Free the interrupt
    printk("sr501_remove called\n");
    gpiod_put(sr501_gpio); // Release the GPIO
    device_destroy(sr501_class, MKDEV(major, 0));
    return 0;
}

static const struct of_device_id ask100_sr501[] = {
    { .compatible = "100ask,sr501" },
    { },
};

/* Define the platform_driver structure */
static struct platform_driver sr501_driver = {
    .probe      = sr501_probe,
    .remove     = sr501_remove,
    .driver     = {
        .name   = "100ask_sr501",
        .of_match_table = ask100_sr501,
    },
};

/* Module entry function */
static int __init sr501_init(void) {
    /* 1. Register a character device */
    major = register_chrdev(0, "sr501", &sr501_fops);
    if (major < 0) {
        printk("Failed to register char device\n");
        return major;
    }

    /* 2. Create a device class */
    sr501_class = class_create(THIS_MODULE, "sr501_class");
    if (IS_ERR(sr501_class)) {
        unregister_chrdev(major, "sr501");
        return PTR_ERR(sr501_class);
    }
    printk("sr501 driver initialized\n");

    init_waitqueue_head(&sr501_wq);

    platform_driver_register(&sr501_driver);

    return 0;
}

/* Module exit function */
static void __exit sr501_exit(void) {
    platform_driver_unregister(&sr501_driver);
    class_destroy(sr501_class);
    unregister_chrdev(major, "sr501");
    printk("sr501 driver exited\n");
}

module_init(sr501_init);
module_exit(sr501_exit);
MODULE_LICENSE("GPL");