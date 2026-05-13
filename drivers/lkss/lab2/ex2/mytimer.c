#include <linux/module.h>	/* Core header for loading LKMs into the kernel */
#include <linux/fs.h>		/* File operations structure and related functions */
#include <linux/cdev.h>		/* Character device support */
#include <linux/kstrtox.h>	/* String parsing */
#include <linux/uaccess.h>	/* Functions for user space/kernel space access */

/* Define device name and class name */
#define DEVICE_NAME     "mytimer"
#define CLASS_NAME      "mytimer_class"
#define MAX_BUF_SIZE    256

static int major;                                  /* Major number assigned to the driver */
static char buf[MAX_BUF_SIZE];                     /* Buffer to store user data */
static struct class *mytimer_class;                /* Device class pointer */
static struct timer_list timer;
static unsigned long start;                        /* Store the starting time of the timer */

/* List declaration - this is the HEAD node */
LIST_HEAD(my_list);

/* Entry in my_list. Has to store a ptr to the next/prev elements (list_head) */
struct timer_list_entry {
        int interval;
        struct list_head list;
};

/**
 * Convert string to signed integer
 *
 * @param buf string to convert
 * @param out resulted integer
 *
 * @return int conversion exit code
 */
static int string_to_int(char *buf, int *out)
{
        /* TODO: Convert string to int (Hint: <linux/kstrtox.h>) */
        return kstrtos32(buf, 10, out);
}

/**
 * Inserts an entry of type timer_list_entry into
 * my_list. The entry is populated with an interval.
 *
 * @param interval value to be inserted
 */
static void insert_interval(int interval)
{
        struct timer_list_entry *entry = kmalloc(sizeof(*entry), GFP_KERNEL);

        if (!entry)
                return;

        /* TODO: populate entry and add it to the list using list_add_tail */
        entry->interval = interval;
        list_add_tail(&entry->list, &my_list);
}

/**
 * Consume the last entry from the list and
 * return the value it contained.
 *
 * @return int value of interval
 */
static int get_next_interval(void)
{
        struct list_head *head, *tmp;
        struct timer_list_entry *entry;
        int interval = 0;

        /* We use this method so that we can also delete the entry */

        list_for_each_safe(head, tmp, &my_list) {
                entry = list_entry(head, struct timer_list_entry, list);
                interval = entry->interval;

                /* TODO: use list_del to delete the entry */
                list_del(head);

                /* TODO: free entry using kfree */
                kfree(entry);

                /* Only get first one */
                return interval;
        }

        return interval;
}

/**
 * Delete any remaining nodes in the list
 */
static void destroy_list(void)
{
        struct list_head *head, *tmp;
        struct timer_list_entry *entry;

        list_for_each_safe(head, tmp, &my_list) {
                /* TODO: Delete every entry - similar to get_next_interval */
                entry = list_entry(head, struct timer_list_entry, list);
                list_del(head);
                kfree(entry);
        }
}

static void timer_cb(struct timer_list *t)
{
        int interval = 0;

        /* TODO: print the the interval (use the saved start time) */
        pr_info("Elapsed time %ds\n", jiffies_to_msecs(jiffies - start) / 1000);

        /* TODO: Restart timer if needed */
        interval = get_next_interval();

        if (!interval)
                return;

        start = jiffies;
        mod_timer(&timer, jiffies + msecs_to_jiffies(interval * 1000));
}

/**
 * Called when the device is opened
 */
static int dev_open(struct inode *inodep, struct file *filep) {
        return 0;
}

/**
 * Called when the device is closed
 */
static int dev_release(struct inode *inodep, struct file *filep) {
        return 0;
}

/**
 * Called when user writes to the device
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
        int err;
        int interval;

        /* Limit message length to prevent overflow */
        len = MIN(len, 255);

        /* TODO: Copy data from user space to kernel buffer */
        err = copy_from_user(buf, buffer, len);
        if (err) {
                pr_err("Failed to copy buf from user!\n");
                return -EINVAL;
        }

        buf[len] = '\0';

        /* TODO: Parse the string */
        err = string_to_int(buf, &interval);
        if (err) {
                printk(KERN_ERR "%s: Invalid arguments!\n", DEVICE_NAME);
                return -EINVAL;
        }

        /* TODO: If timer is running, postpone interval using list */
        if (timer_pending(&timer)) {
                insert_interval(interval);
                return len;
        }

        if (interval) {
                /* TODO: Save current time and start timer */
                start = jiffies;
                mod_timer(&timer, jiffies + msecs_to_jiffies(interval * 1000));
        }

        return len;/* Return number of bytes written */
}

/**
 * File operations structure for this driver
 */
static struct file_operations fops = {
	.open = dev_open,
	.write = dev_write,
	.release = dev_release
};

/**
 * Initialization function for the module
 */
static int __init mytimer_init(void) {
        /* Initialize timer */
        timer_setup(&timer, timer_cb, 0);

        /* TODO: Register character device and get dynamically assigned major number */
        major = register_chrdev(0, "mytimer_mod", &fops);

        /* TODO: Create a device class in sysfs (/sys/class/mytimer_class) */
        mytimer_class = class_create("mytimer_class");

        /* TODO: Create the device file (/dev/mytimer) */
        device_create(mytimer_class, NULL,
                      MKDEV(major, 0), NULL,
                      DEVICE_NAME);

        pr_info("%s: Device initialized\n", DEVICE_NAME);
        return 0;
}

/**
 * Cleanup function for the module
 */
static void __exit mytimer_exit(void) {
        /* TODO: Remove /dev entry, use device_destroy() */
        device_destroy(mytimer_class, MKDEV(major, 0));

        /* TODO: Unregister class */
        class_unregister(mytimer_class);

        /* TODO: Destroy class */
        class_destroy(mytimer_class);

        /* TODO: Unregister char device */
        unregister_chrdev(major, "mytimer_mod");

        /* Delete timer */
        timer_delete_sync(&timer);

        destroy_list();

        pr_info("%s: Goodbye!\n", DEVICE_NAME);
}

module_init(mytimer_init);
module_exit(mytimer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NXP Linux Kernel Summer School");
MODULE_DESCRIPTION("Lab2 Ex2: Command a timer using a char device");
