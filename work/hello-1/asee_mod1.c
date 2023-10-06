#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include <asm/errno.h>

#define SUCCESS 0
#define DEVICE_NAME "chardev"
#define BUF_LEN 16  // Taille du buffer circulaire

static int major;
static atomic_t already_open = ATOMIC_INIT(0);
static char circular_buffer[BUF_LEN];
static int write_index = 0;
static int read_index = 0;

static struct class *cls;

static struct file_operations chardev_fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release,
};

static int __init chardev_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &chardev_fops);

    if (major < 0) {
        pr_alert("Registering char device failed with %d\n", major);
        return major;
    }

    pr_info("I was assigned major number %d.\n", major);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    cls = class_create(DEVICE_NAME);
#else
    cls = class_create(THIS_MODULE, DEVICE_NAME);
#endif
    device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);

    pr_info("Device created on /dev/%s\n", DEVICE_NAME);

    return SUCCESS;
}

static void __exit chardev_exit(void)
{
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);

    unregister_chrdev(major, DEVICE_NAME);
}

static int device_open(struct inode *inode, struct file *file)
{
    if (atomic_cmpxchg(&already_open, 0, 1)) {
        return -EBUSY;
    }

    try_module_get(THIS_MODULE);

    return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
    atomic_set(&already_open, 0);

    module_put(THIS_MODULE);

    return SUCCESS;
}

static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
    int bytes_read = 0;

    while (length && read_index != write_index) {
        put_user(circular_buffer[read_index], buffer);
        read_index = (read_index + 1) % BUF_LEN;
        buffer++;
        length--;
        bytes_read++;
    }

    return bytes_read;
}

static ssize_t device_write(struct file *filp, const char __user *buff, size_t len, loff_t *off)
{
    int bytes_written = 0;

    while (len && ((write_index + 1) % BUF_LEN) != read_index) {
        get_user(circular_buffer[write_index], buff);
        write_index = (write_index + 1) % BUF_LEN;
        buff++;
        len--;
        bytes_written++;
    }

    return bytes_written;
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
