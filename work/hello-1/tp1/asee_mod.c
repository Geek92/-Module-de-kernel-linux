/*
 * chardev.c: Creates a read-only char device that says how many times
 * you have read from the dev file
 */

#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h> /* for sprintf() */
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/uaccess.h> /* for get_user and put_user */
#include <linux/version.h>
#include  <linux/slab.h>

#include <asm/errno.h>

/*  Prototypes - this would normally go in a .h file */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t,
                            loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "asee_mod" /* Dev name as it appears in /proc/devices   */
#define BUF_LEN 16 /* Max length of the message from the device */
#define DEFALUT_VAL 1 /* Max length of the message from the device */


/* Global variables are declared as static, so are global within the file. */

static int major; /* major number assigned to our device driver */
static char *circular_buffer = NULL;
static int write_index = 0;
static int read_index = 0;
static int byte_written = 0;


enum {
    CDEV_NOT_USED = 0,
    CDEV_EXCLUSIVE_OPEN = 1,
};

/* Is device open? Used to prevent multiple access to device */
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

//static char message[BUF_LEN]; /*the circular buffer*/

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

// we initialize the circular buffer
    circular_buffer = kmalloc(BUF_LEN*sizeof(char), GFP_KERNEL);
    if(circular_buffer == NULL){
        pr_info("error during buffer initialization");
        return -ENOMEM;
    }
    return SUCCESS;
}

static void __exit chardev_exit(void)
{
    kfree(circular_buffer);
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);

    /* Unregister the device */
    unregister_chrdev(major, DEVICE_NAME);
}

//vider le buffer
static void emptybuffer(char *buffer, int buffer_length){
   for(int i = 0; i < buffer_length; i++){
       buffer[i] = '\0';
   }
}
/* Methods */

/* Called when a process tries to open the device file, like
 * "sudo cat /dev/chardev"
 */
static int device_open(struct inode *inode, struct file *file)
{
    //static int counter = 0;

    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN))
        return -EBUSY;

    //sprintf(msg, "I already told you %d times Hello world!\n", counter++);
    try_module_get(THIS_MODULE);

    return SUCCESS;
}

/* Called when a process closes the device file. */
static int device_release(struct inode *inode, struct file *file)
{
    //we free the memory occupied by the buffer
    // kfree(circular_buffer);
    /* We're now ready for our next caller */
    atomic_set(&already_open, CDEV_NOT_USED);

    /* Decrement the usage count, or else once you opened the file, you will
     * never get rid of the module.
     */
    module_put(THIS_MODULE);

    return SUCCESS;
}

/* cette fonction est appelée losqu'on effectue la commande echo au niveau du terminal
 * read from it.
 */
 //emptybuffer(circular_buffer,BUF_LEN);

 static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset) {
     int bytes_read = 0;
     int end = byte_written;
     //char *msg_ptr = circular_buffer;

     if (byte_written > BUF_LEN){
          read_index = write_index;
          end = byte_written - write_index;
     }

     while (length && end){
           put_user(circular_buffer[read_index], buffer++);
           pr_info("the output %c\n",circular_buffer[read_index]);
           pr_info(" the adress %p\n",&circular_buffer[read_index]);
           read_index = (read_index + 1) % BUF_LEN;
           bytes_read++;
           length--;
           end--;
     }
     //put_user('\0', buffer + 1);
     byte_written = 0;
     read_index = 0;
     write_index = 0;
     emptybuffer(circular_buffer, BUF_LEN);
     return bytes_read;
 }

 /*
  * this function is used to write user input inside the circular buffer
  */
 static ssize_t device_write(struct file *filp, const char __user *buff, size_t len, loff_t *off) {
     //char *msg_ptr = circular_buffer;
     for(int i = 0; i < len -1; i++){
           get_user(circular_buffer[write_index], buff++);
           pr_info("the user input %c\n",circular_buffer[write_index]);
           pr_info(" the adress %p\n", &circular_buffer[write_index]);
           byte_written++;
           write_index = (write_index + 1) % BUF_LEN;
     }
     //pr_info("the user input ",circular_buffer);
     return byte_written > 0 ? byte_written : -EINVAL;
 }


module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
