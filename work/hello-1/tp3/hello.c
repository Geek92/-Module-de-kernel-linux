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
#define DEVICE_NAME "asee_mod"
#define BUF_LEN 80
#define MY_BUF_LEN 17
#define BEFORE_0_TERMINATOR 2
#define ANTI_ZERO 2

enum {
    CDEV_NOT_USED = 0,
    CDEV_EXCLUSIVE_OPEN = 1
};

static int major; /* major number assigned to our device driver */

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *);

static char msg[BUF_LEN + 1];
static char circular_buffer[MY_BUF_LEN];
static int my_offset = 0;
static int count = 0;

static struct class *cls;
static struct file_operations chardev_fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};

/* Is device open? Used to prevent multiple access to device */
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);


static int __init chardev_init(void) {
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


static void __exit chardev_exit(void) {
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    /* Unregister the device */
    unregister_chrdev(major, DEVICE_NAME);
}


static int device_open(struct inode *inode, struct file *file) {
    static int counter = 0;

    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN))
        return -EBUSY;

    sprintf(msg, "I already told you %d times Hello world!\n", counter++);
    try_module_get(THIS_MODULE);

    return SUCCESS;
}


static int device_release(struct inode *inode, struct file *file) {
    atomic_set(&already_open, CDEV_NOT_USED);
    module_put(THIS_MODULE);
    return SUCCESS;
}


static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset) {
    int bytes_read = 0;
    char *msg_ptr = circular_buffer;

    int index_circular_buffer = 0;

    if (count > (MY_BUF_LEN - 1)) index_circular_buffer = my_offset;

    while (length && count) {
        put_user(msg_ptr[index_circular_buffer], buffer++);
        msg_ptr[index_circular_buffer] = 0;
        index_circular_buffer++; bytes_read++;
        length--; count--;
        index_circular_buffer = index_circular_buffer % MY_BUF_LEN;
    }

    return bytes_read;
}


static ssize_t device_write(struct file *filp, const char __user *buff, size_t len, loff_t *off) {
    char *msg_ptr = circular_buffer;
    count = 0;
    my_offset = 0;

    while (len && buff[count]) {
        get_user(msg_ptr[my_offset], &buff[count]);
        count++; len--;
        my_offset = count % MY_BUF_LEN;
    }

    // ne pas renvoyer zéro sinon appels infinis.
    return count > 0 ? count : ANTI_ZERO;
}


module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
