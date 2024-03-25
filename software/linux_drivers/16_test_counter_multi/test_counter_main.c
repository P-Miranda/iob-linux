/* test_counter_main.c: driver for test_counter
 * using device platform. No hardcoded hardware address:
 * 1. load driver: insmod test_counter.ko
 * 2. run user app: ./user/user
 */

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>

#include "iob_class/iob_class_utils.h"
#include "test_counter.h"

static int test_counter_probe(struct platform_device *);
static int test_counter_remove(struct platform_device *);

static ssize_t test_counter_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t test_counter_write(struct file *, const char __user *, size_t,
                               loff_t *);
static loff_t test_counter_llseek(struct file *, loff_t, int);
static int test_counter_open(struct inode *, struct file *);
static int test_counter_release(struct inode *, struct file *);

static struct iob_data test_counter_data = {0};
DEFINE_MUTEX(test_counter_mutex);

#include "test_counter_sysfs.h"

static const struct file_operations test_counter_fops = {
    .owner = THIS_MODULE,
    .write = test_counter_write,
    .read = test_counter_read,
    .llseek = test_counter_llseek,
    .open = test_counter_open,
    .release = test_counter_release,
};

static const struct of_device_id of_test_counter_match[] = {
    {.compatible = "test-counter"},
    {},
};

static struct platform_driver test_counter_driver = {
    .driver =
        {
            .name = "test_counter",
            .owner = THIS_MODULE,
            .of_match_table = of_test_counter_match,
        },
    .probe = test_counter_probe,
    .remove = test_counter_remove,
};

//
// Module init and exit functions
//
static int test_counter_probe(struct platform_device *pdev) {
  struct resource *res;
  int result = 0;

  if (test_counter_data.device != NULL) {
    pr_err("[Driver] %s: No more devices allowed!\n", TEST_COUNTER_DRIVER_NAME);

    return -ENODEV;
  }

  pr_info("[Driver] %s: probing.\n", TEST_COUNTER_DRIVER_NAME);

  // Get the I/O region base address
  res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (!res) {
    pr_err("[Driver]: Failed to get I/O resource!\n");
    result = -ENODEV;
    goto r_get_resource;
  }

  // Request and map the I/O region
  test_counter_data.regbase = devm_ioremap_resource(&pdev->dev, res);
  if (IS_ERR(test_counter_data.regbase)) {
    result = PTR_ERR(test_counter_data.regbase);
    goto r_ioremmap;
  }
  test_counter_data.regsize = resource_size(res);

  // Alocate char device
  result =
      alloc_chrdev_region(&test_counter_data.devnum, 0, 1, TEST_COUNTER_DRIVER_NAME);
  if (result) {
    pr_err("%s: Failed to allocate device number!\n", TEST_COUNTER_DRIVER_NAME);
    goto r_alloc_region;
  }

  cdev_init(&test_counter_data.cdev, &test_counter_fops);

  result = cdev_add(&test_counter_data.cdev, test_counter_data.devnum, 1);
  if (result) {
    pr_err("%s: Char device registration failed!\n", TEST_COUNTER_DRIVER_NAME);
    goto r_cdev_add;
  }

  // Create device class // todo: make a dummy driver just to create and own the
  // class: https://stackoverflow.com/a/16365027/8228163
  if ((test_counter_data.class =
           class_create(THIS_MODULE, TEST_COUNTER_DRIVER_CLASS)) == NULL) {
    printk("Device class can not be created!\n");
    goto r_class;
  }

  // Create device file
  test_counter_data.device =
      device_create(test_counter_data.class, NULL, test_counter_data.devnum, NULL,
                    TEST_COUNTER_DRIVER_NAME);
  if (test_counter_data.device == NULL) {
    printk("Can not create device file!\n");
    goto r_device;
  }

  result = test_counter_create_device_attr_files(test_counter_data.device);
  if (result) {
    pr_err("Cannot create device attribute file......\n");
    goto r_dev_file;
  }

  dev_info(&pdev->dev, "initialized.\n");
  goto r_ok;

r_dev_file:
  test_counter_remove_device_attr_files(&test_counter_data);
r_device:
  class_destroy(test_counter_data.class);
r_class:
  cdev_del(&test_counter_data.cdev);
r_cdev_add:
  unregister_chrdev_region(test_counter_data.devnum, 1);
r_alloc_region:
  // iounmap is managed by devm
r_ioremmap:
r_get_resource:
r_ok:

  return result;
}

static int test_counter_remove(struct platform_device *pdev) {
  test_counter_remove_device_attr_files(&test_counter_data);
  class_destroy(test_counter_data.class);
  cdev_del(&test_counter_data.cdev);
  unregister_chrdev_region(test_counter_data.devnum, 1);
  // Note: no need for iounmap, since we are using devm_ioremap_resource()

  dev_info(&pdev->dev, "exiting.\n");

  return 0;
}

static int __init test_counter_init(void) {
  pr_info("[Driver] %s: initializing.\n", TEST_COUNTER_DRIVER_NAME);

  return platform_driver_register(&test_counter_driver);
}

static void __exit test_counter_exit(void) {
  pr_info("[Driver] %s: exiting.\n", TEST_COUNTER_DRIVER_NAME);
  platform_driver_unregister(&test_counter_driver);
}

//
// File operations
//

static int test_counter_open(struct inode *inode, struct file *file) {
  pr_info("[Driver] test_counter device opened\n");

  if (!mutex_trylock(&test_counter_mutex)) {
    pr_info("Another process is accessing the device\n");

    return -EBUSY;
  }

  return 0;
}

static int test_counter_release(struct inode *inode, struct file *file) {
  pr_info("[Driver] test_counter device closed\n");

  mutex_unlock(&test_counter_mutex);

  return 0;
}

static ssize_t test_counter_read(struct file *file, char __user *buf, size_t count,
                              loff_t *ppos) {
  int size = 0;
  u32 value = 0;

  /* read value from register */
  switch (*ppos) {
  case TEST_COUNTER_ID_ADDR:
    value = iob_data_read_reg(test_counter_data.regbase, TEST_COUNTER_ID_ADDR,
                              TEST_COUNTER_ID_W);
    size = (TEST_COUNTER_ID_W >> 3); // bit to bytes
    pr_info("[Driver] Read ID\n");
    break;
  case TEST_COUNTER_DATA_ADDR:
    value = iob_data_read_reg(test_counter_data.regbase, TEST_COUNTER_DATA_ADDR,
                              TEST_COUNTER_DATA_W);
    size = (TEST_COUNTER_DATA_W >> 3); // bit to bytes
    pr_info("[Driver] Read sampled data!\n");
    break;
  default:
    // invalid address - no bytes read
    return 0;
  }

  // Read min between count and REG_SIZE
  if (size > count)
    size = count;

  if (copy_to_user(buf, &value, size))
    return -EFAULT;

  return count;
}

static ssize_t test_counter_write(struct file *file, const char __user *buf,
                               size_t count, loff_t *ppos) {
  int size = 0;
  u32 value = 0;

  switch (*ppos) {
  case TEST_COUNTER_RST_ADDR:
    size = (TEST_COUNTER_RST_W >> 3); // bit to bytes
    // if (read_user_data(buf, size, &value))
    //   return -EFAULT;
    iob_data_write_reg(test_counter_data.regbase, 0x1, TEST_COUNTER_RST_ADDR,
                       TEST_COUNTER_RST_W);
    iob_data_write_reg(test_counter_data.regbase, 0x0, TEST_COUNTER_RST_ADDR,
                       TEST_COUNTER_RST_W);
    pr_info("[Driver] Reset counter!\n");
    break;
  case TEST_COUNTER_INCR_ADDR:
    size = (TEST_COUNTER_INCR_W >> 3); // bit to bytes
    // if (read_user_data(buf, size, &value))
    //   return -EFAULT;
    iob_data_write_reg(test_counter_data.regbase, 0x1, TEST_COUNTER_INCR_ADDR,
                       TEST_COUNTER_INCR_W);
    pr_info("[Driver] Increment counter!\n");
    break;
  case TEST_COUNTER_SAMPLE_ADDR:         // sample counter
    size = (TEST_COUNTER_SAMPLE_W >> 3); // bit to bytes
    // if (read_user_data(buf, size, &value))
    //   return -EFAULT;
    iob_data_write_reg(test_counter_data.regbase, 0x1, TEST_COUNTER_SAMPLE_ADDR,
                       TEST_COUNTER_SAMPLE_W);
    pr_info("[Driver] Sample counter!\n");
    break;
  case TEST_COUNTER_SET_ADDR:         // sample counter
    size = (TEST_COUNTER_SET_W >> 3); // bit to bytes
    if (read_user_data(buf, size, &value))
      return -EFAULT;
    iob_data_write_reg(test_counter_data.regbase, value, TEST_COUNTER_SET_ADDR,
                       TEST_COUNTER_SET_W);
    pr_info("[Driver] Set counter to 0x%x!\n", value);
    break;
  default:
    pr_info("[Driver] Invalid write address 0x%x\n", (unsigned int)*ppos);
    // invalid address - no bytes written
    return 0;
  }

  return count;
}

/* Custom lseek function
 * check: lseek(2) man page for whence modes
 */
static loff_t test_counter_llseek(struct file *filp, loff_t offset, int whence) {
  loff_t new_pos = -1;

  switch (whence) {
  case SEEK_SET:
    new_pos = offset;
    break;
  case SEEK_CUR:
    new_pos = filp->f_pos + offset;
    break;
  case SEEK_END:
    new_pos = (1 << TEST_COUNTER_SWREG_ADDR_W) + offset;
    break;
  default:
    return -EINVAL;
  }

  // Check for valid bounds
  if (new_pos < 0 || new_pos > test_counter_data.regsize) {
    return -EINVAL;
  }

  // Update file position
  filp->f_pos = new_pos;

  return new_pos;
}

module_init(test_counter_init);
module_exit(test_counter_exit);

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("IObundle");
MODULE_DESCRIPTION("IOb-Timer Drivers");
MODULE_VERSION("0.10");
