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
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <asm/errno.h>
#include <linux/wait.h> /* For putting processes to sleep and
                                   waking them up */

/*  Prototypes - this would normally go in a .h file */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t,
                            loff_t *);
static int controlCcheck(void);
static void emptybuffer(char *buffer, int buffer_length);

#define SUCCESS 0
#define DEVICE_NAME "asee_mod" /* Dev name as it appears in /proc/devices   */
#define BUF_LEN 16 /* Max length of the message from the device */
#define DEFALUT_VAL 1 /* Max length of the message from the device */


/* Global variables are declared as static, so are global within the file. */

static int major; /* major number assigned to our device driver */
static char *circular_buffer;
static int write_index = 0;
static int read_index = 0;
static int asee_buf_size = BUF_LEN;
static int asee_buf_count = 0;
/* Queue of processes who want our file */
static DECLARE_WAIT_QUEUE_HEAD(waitq);
//static char *msg_ptr;
//static int count = 0;
//static int byte_written = 0;
static struct kobject *mymodule;
//char *buff_ptr = circular_buffer;

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

// fonction pour manipuler la taille du buffer
static ssize_t asee_buf_size_show(struct kobject *kobj,
                               struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", asee_buf_size);
}

static ssize_t asee_buf_size_store(struct kobject *kobj,
                                struct kobj_attribute *attr, char *buf,
                                size_t count)
{
    int new_buffer_size = 0;
    sscanf(buf, "%du", &new_buffer_size);

    if(new_buffer_size == asee_buf_size){
        return count;
    }else if(new_buffer_size < asee_buf_size){
          if(new_buffer_size < asee_buf_count){
              pr_err("there are more characters in the actual buffer than the require size");
              return -1;
          } else {
                  asee_buf_size = new_buffer_size;
          }
    }else{
            asee_buf_size = new_buffer_size;
            char  *new_circular_buffer = (char*)kmalloc(new_buffer_size * sizeof(char),GFP_KERNEL);
            for(int i = 0 ; i < asee_buf_count; i++){
                new_circular_buffer[i] = circular_buffer[i];
            };
            kfree(circular_buffer);
            circular_buffer = new_circular_buffer;

    }
    return count;
}

static struct kobj_attribute asee_buf_size_attribute =
  __ATTR(asee_buf_size, 0660, asee_buf_size_show, (void *)asee_buf_size_store);

//fonctions pour lire le nombre de caractereqs presents dans le buffer
static ssize_t asee_buf_count_show(struct kobject *kobj,
                               struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", asee_buf_count);
}

static ssize_t asee_buf_count_store(struct kobject *kobj,
                                struct kobj_attribute *attr, char *buf,
                                size_t count)
{
      pr_err("this function is not yet implemented");
      return -1;
}


static struct kobj_attribute asee_buf_count_attribute =
  __ATTR(asee_buf_count, 0660, asee_buf_count_show, (void *)asee_buf_count_store);


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

    int error = 0;

    pr_info("mymodule: initialised\n");

    mymodule = kobject_create_and_add("mymodule", kernel_kobj);
    if (!mymodule)
        return -ENOMEM;

    error = sysfs_create_file(mymodule, &asee_buf_size_attribute.attr);
    if (error) {
        pr_info("failed to create the asee_buf_size file "
                "in /sys/kernel/mymodule\n");
    }
    error = sysfs_create_file(mymodule, &asee_buf_count_attribute.attr);
    if (error) {
        pr_info("failed to create the asee_buf_count file "
                "in /sys/kernel/mymodule\n");
    }
    return SUCCESS;
}

static void __exit chardev_exit(void)
{
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);

    pr_info("mymodule: Exit success\n");
    kobject_put(mymodule);
    /* Unregister the device */
    unregister_chrdev(major, DEVICE_NAME);

}

/* Methods */

//vider le buffer
static void emptybuffer(char *buffer, int buffer_length){
   for(int i = 0; i < buffer_length; i++){
       *buffer++ = '\0';
   }
}

static int controlCcheck(){
            int i, is_sig = 0; 

            for (i = 0; i < _NSIG_WORDS && !is_sig; i++) 
                is_sig = current->pending.signal.sig[i] & ~current->blocked.sig[i]; 
            if (is_sig) {         
                module_put(THIS_MODULE); 
                return -EINTR; 
            } 
            return DEFALUT_VAL;
} 
/* Called when a process tries to open the device file, like
 * "sudo cat /dev/chardev"
 */
static int device_open(struct inode *inode, struct file *file)
{    
    // on initialise le buffer
    circular_buffer = (char*)kmalloc(BUF_LEN * sizeof(char),GFP_KERNEL);
    //static int counter = 0;

    //if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN))
        //return -EBUSY;

    //sprintf(msg, "I already told you %d times Hello world!\n", counter++);
    try_module_get(THIS_MODULE);

    return SUCCESS;
}

/* Called when a process closes the device file. */
static int device_release(struct inode *inode, struct file *file)
{
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

     if(asee_buf_count == 0)
        wait_event_interruptible(waitq, asee_buf_count > 0);
     controlCcheck();
     int bytes_read = 0;
     int end = asee_buf_count;
     char *msg_ptr = circular_buffer;

     while (length && end){
           put_user(msg_ptr[read_index], buffer++);
           read_index = (read_index  + 1) % asee_buf_size;
           bytes_read++;
           length--;
           end--;
     }
     //emptybuffer(circular_buffer, asee_buf_size);
     asee_buf_count = 0;
     read_index = 0;
     write_index = 0;
     wake_up(&waitq);
     return bytes_read;
 }

 static ssize_t device_write(struct file *filp, const char __user *buff, size_t len, loff_t *off) {

     //wait_event_interruptible(waitq, asee_buf_size > asee_buf_count);
     if( asee_buf_size == asee_buf_count)
          wait_event_interruptible(waitq, asee_buf_size > asee_buf_count);
     controlCcheck();
     char *msg_ptr = circular_buffer;
     for(int i = 0; i < len -1; i++){
           get_user(msg_ptr[write_index], buff++);
           asee_buf_count++;
           wake_up(&waitq);
           write_index = (write_index  + 1) % asee_buf_size;
           if( asee_buf_size == asee_buf_count)
                wait_event_interruptible(waitq, asee_buf_size > asee_buf_count);
           controlCcheck();
     }

     return asee_buf_count;
 }


module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
