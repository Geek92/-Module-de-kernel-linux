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


/* Global variables are declared as static, so are global within the file. */

static int major; /* major number assigned to our device driver */
static int read = 0; //tete de lecture
static int write = 0; //tete d'ecriture

enum {
    CDEV_NOT_USED = 0,
    CDEV_EXCLUSIVE_OPEN = 1,
};

/* Is device open? Used to prevent multiple access to device */
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

static char message[BUF_LEN]; /*the circular buffer*/

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

    /* Unregister the device */
    unregister_chrdev(major, DEVICE_NAME);
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
 static ssize_t device_write(struct file *filp,
                            const char __user *buffer,
                            size_t length,
                            loff_t *offset)
 {
     //nombre de bytes copiés dans le buffer circulaire
     int bytes_written = 0;
     for(int i = 0; i < length; i++){
          get_user(message[write], buffer + i);
          bytes_written++;
          write++;
          //si on a ecris le seizieme charactere, on remets la tete d'ecriture a la position initiale
          if(write == BUF_LEN){
              write = 0;
          }
    }
    // si on a ecris plus de 16 characters on deplace la tete de lecture sur le successeur du dernier charactere ecris
    if(bytes_written > BUF_LEN){
            read = write;
   }
   return bytes_written;
 }

//cette fonction est applelée quand on veut lire les valeurs présentes sur le device
/*static ssize_t device_read(struct file *filp,
                           char __user *message,
                           size_t length,
                           loff_t *offset)
{
    // nombre de bytes lus
    int bytes_read = 0;
    // on lit les valeurs contenues dans le buffer et on les envoie vers le terminal
    for(int i = 0; i < length; i++){
          put_user(buffer[read], message + i);
          bytes_read++;
          read++;
    }
    read = 0;
    //on vide le buffer apres chaque lecture
    for(int i = 0; i < BUF_LEN; i++){
       buffer[i] =  '\0';
    }
    // le nombre total de byte lus
    return bytes_read;
}*/
static ssize_t device_read(struct file *filp, /* see include/linux/fs.h   */
                           char __user *buffer, /* buffer to fill with data */
                           size_t length, /* length of the buffer     */
                           loff_t *offset)
{
  /* Nombre d'octets réellement lus dans le tampon */
  int bytes_read = 0;

  /* Position actuelle de lecture dans le tampon circulaire */
  //static int read_position = 0;

  /* Utiliser un mutex pour éviter les conflits d'accès concurrents si nécessaire */

  /* Boucle pour lire les données du tampon circulaire */
  while (length > 0) {
      char data;

      /* Lire un octet du tampon circulaire à la position actuelle */
      data = message[read];

      /* Si nous avons atteint la fin du message (null terminator), sortir de la boucle */
      if (data == '\0') {
          break;
      }

      /* Copier l'octet lu dans le tampon utilisateur */
      if (put_user(data, buffer) != 0) {
          /* Gestion de l'erreur de copie vers l'utilisateur si nécessaire */
          return -EFAULT;
      }

      /* Mettre à jour les compteurs et les positions */
      length--;
      buffer++;
      bytes_read++;
      read++;

      /* Si nous atteignons la fin du tampon circulaire, revenir au début */
      if (read == BUF_LEN) {
          read = 0;
      }
  }

  /* Si nous avons lu au moins un octet, réinitialiser le décalage à zéro */
  if (bytes_read > 0) {
      *offset = 0;
  }
  if (message[read] == '\0') {
       memset(message, 0, sizeof(message));
       read = 0;
   }

  return bytes_read;
}


module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
