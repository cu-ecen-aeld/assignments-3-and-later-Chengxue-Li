/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h> // kmalloc, kfree, krealloc
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Chengxue-Li"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev;
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev; // for other methods to access the device
    PDEBUG("Device opened successfully");
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    PDEBUG("Device released successfully");
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev;
    size_t entry_offset_byte; // Offset within the entry
    struct aesd_buffer_entry *buffer_entry;
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    dev = filp->private_data;
    if (down_interruptible(&dev->sem)) {
        PDEBUG("Failed to acquire semaphore");
        return -ERESTARTSYS; // Interrupted by a signal
    }
    PDEBUG("Acquired semaphore for reading");

    buffer_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->circular_buffer, *f_pos, &entry_offset_byte);
    if (buffer_entry == NULL) {
        PDEBUG("No entry found for the given file position");
        retval = 0; // No data to read
        goto out; // Exit with no data read
    }
    PDEBUG("Found entry for file position, size: %zu", buffer_entry->size);
    count = min(count, buffer_entry->size - entry_offset_byte); // Limit count to available data size
    if (copy_to_user(buf, buffer_entry->buffptr + entry_offset_byte, count)) {
        PDEBUG("Failed to copy data to user space");
        retval = -EFAULT; // Copy failed
        goto out; // Exit with error
    }
    PDEBUG("Data read successfully, bytes read: %zu", count);
    retval = count; // Set the return value to the number of bytes read


    out:
    up(&dev->sem); // Release the semaphore
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    struct aesd_dev *dev;
    const char *dst;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
    dev = filp->private_data;

    if (down_interruptible(&dev->sem)) {
        PDEBUG("Failed to acquire semaphore");
        return -ERESTARTSYS; // Interrupted by a signal
    }
    PDEBUG("Acquired semaphore for writing");
    
    // If the write entry buffer is NULL, allocate memory for it.
    if (dev->write_entry.buffptr == NULL) {
        dev->write_entry.buffptr = kmalloc(count, GFP_KERNEL);
        if (dev->write_entry.buffptr == NULL) {
            PDEBUG("Failed to allocate memory for write entry buffer");
            goto out; // Memory allocation failed
        }
        dst = dev->write_entry.buffptr; // Set dst to the newly allocated buffer
        dev->write_entry.size = 0; // Initialize size to 0
    } else {
        // If the write entry buffer is not NULL, reallocate memory to accommodate the new data.
        char *new_buffptr = krealloc(dev->write_entry.buffptr, dev->write_entry.size + count, GFP_KERNEL);
        if (new_buffptr == NULL) {
            PDEBUG("Failed to reallocate memory for write entry buffer");
            goto out; // Memory reallocation failed
        }
        dev->write_entry.buffptr = new_buffptr;
        dst = dev->write_entry.buffptr + dev->write_entry.size; // Set dst to the end of the current buffer
    }
    
    // Copy data from user space to the write entry buffer
    if (copy_from_user((void *)dst, buf, count)) {
        PDEBUG("Failed to copy data from user space");
        retval = -EFAULT; // Copy failed
        goto out; // Exit with error
    }
    dev->write_entry.size += count; // Update the size of the write entry buffer
    PDEBUG("Data written successfully, size now %zu", dev->write_entry.size);

    // Check if the write entry ends with a newline character
    if (dev->write_entry.size > 0 && dev->write_entry.buffptr[dev->write_entry.size - 1] == '\n') {
        // If it does, add the write entry to the circular buffer
        aesd_circular_buffer_add_entry(&dev->circular_buffer, &dev->write_entry);
        PDEBUG("Write entry added to circular buffer");
        dev->write_entry.buffptr = NULL; // Reset the write entry buffer pointer
        dev->write_entry.size = 0; // Reset the size of the write entry
    }

    retval = count; // Set the return value to the number of bytes written
    PDEBUG("Write operation completed successfully, returning %zd", retval);

    out:
    up(&dev->sem); // Release the semaphore
    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */
    aesd_device.write_entry.buffptr = NULL; // Initialize write entry buffer pointer
    aesd_device.write_entry.size = 0; // Initialize write entry size
    aesd_circular_buffer_init(&aesd_device.circular_buffer);
    sema_init(&aesd_device.sem, 1); // Initialize semaphore for synchronization

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    struct aesd_buffer_entry  *entry;
    uint8_t index;
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    
    PDEBUG("Cleaning up AESD device");
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.circular_buffer, index) {
        if (entry->buffptr) {
            kfree((void *)entry->buffptr); // Free the buffer pointer
            entry->buffptr = NULL; // Set pointer to NULL after freeing
        }
    }
    if (aesd_device.write_entry.buffptr) {
        kfree((void *)aesd_device.write_entry.buffptr); // Free the write entry buffer
        aesd_device.write_entry.buffptr = NULL; // Set pointer to NULL after freeing
    }
    
    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
