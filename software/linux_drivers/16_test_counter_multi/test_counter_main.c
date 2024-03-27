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
#include "linux/mutex.h"
#include "test_counter.h"

#define NUM_DEVICES 2

static int test_counter_probe(struct platform_device *);
static int test_counter_remove(struct platform_device *);

static ssize_t test_counter_read(struct file *, char __user *, size_t,
                                 loff_t *);
static ssize_t test_counter_write(struct file *, const char __user *, size_t,
                                  loff_t *);
static loff_t test_counter_llseek(struct file *, loff_t, int);
static int test_counter_open(struct inode *, struct file *);
static int test_counter_release(struct inode *, struct file *);

DEFINE_MUTEX(t_counter_mutex);
struct test_counter_driver {
    dev_t devnum;
    struct class *class;
    struct list_head list;
};

static struct test_counter_driver tc_driver = {
    .devnum = 0,
    .class = NULL,
    .list = LIST_HEAD_INIT(tc_driver.list),
};

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
  struct iob_data *t_counter_data = NULL;

  mutex_lock(&t_counter_mutex);
  if (MINOR(tc_driver.devnum) >= NUM_DEVICES) {
    pr_err("[Driver] %s: too many devices.\n", TEST_COUNTER_DRIVER_NAME);
    return -ENODEV;
  }
  mutex_unlock(&t_counter_mutex);

  pr_info("[Driver] %s: probing.\n", TEST_COUNTER_DRIVER_NAME);

  t_counter_data = (struct iob_data *)devm_kzalloc(
      &pdev->dev, sizeof(struct iob_data), GFP_KERNEL);
  if (t_counter_data == NULL) {
    pr_err("[Driver]: Failed to allocate memory for iob_data struct!\n");
    return -ENOMEM;
  }

  // add device to list
  mutex_lock(&t_counter_mutex);
  list_add_tail(&t_counter_data->list, &tc_driver.list);
  mutex_unlock(&t_counter_mutex);

  // Get the I/O region base address
  res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (!res) {
    pr_err("[Driver]: Failed to get I/O resource!\n");
    result = -ENODEV;
    goto r_get_resource;
  }

  // Request and map the I/O region
  t_counter_data->regbase = devm_ioremap_resource(&pdev->dev, res);
  if (IS_ERR(t_counter_data->regbase)) {
    result = PTR_ERR(t_counter_data->regbase);
    goto r_ioremmap;
  }
  t_counter_data->regsize = resource_size(res);

  cdev_init(&t_counter_data->cdev, &test_counter_fops);
  t_counter_data->cdev.owner = THIS_MODULE;
  t_counter_data->class = NULL;

  mutex_lock(&t_counter_mutex);
  t_counter_data->devnum = tc_driver.devnum;
  tc_driver.devnum = MKDEV(MAJOR(tc_driver.devnum), MINOR(tc_driver.devnum) + 1);
  mutex_unlock(&t_counter_mutex);

  result = cdev_add(&t_counter_data->cdev, t_counter_data->devnum, 1);
  if (result) {
    pr_err("%s: Char device registration failed!\n", TEST_COUNTER_DRIVER_NAME);
    goto r_cdev_add;
  }

  t_counter_data->device =
      device_create(tc_driver.class, NULL, t_counter_data->devnum, t_counter_data, "%s%d",
                    TEST_COUNTER_DRIVER_NAME, MINOR(t_counter_data->devnum));
  if (t_counter_data->device == NULL) {
    printk("Can not create device file!\n");
    goto r_device;
  }

  // Associate iob_data to device
  pdev->dev.platform_data = t_counter_data;               // pdev functions
  t_counter_data->device->platform_data = t_counter_data; // sysfs functions

  result = test_counter_create_device_attr_files(t_counter_data->device);
  if (result) {
    pr_err("Cannot create device attribute file......\n");
    goto r_dev_file;
  }

  dev_info(&pdev->dev, "initialized with %d.\n", MINOR(t_counter_data->devnum));
  goto r_ok;

r_dev_file:
  device_destroy(t_counter_data->class, t_counter_data->devnum);
  cdev_del(&(t_counter_data->cdev));
r_device:
r_cdev_add:
  // iounmap is managed by devm
r_ioremmap:
r_get_resource:
r_ok:

  return result;
}

static int test_counter_remove(struct platform_device *pdev) {
  struct iob_data *t_counter_data = (struct iob_data *)pdev->dev.platform_data;
  test_counter_remove_device_attr_files(t_counter_data);
  cdev_del(&(t_counter_data->cdev));

  // remove from list
  mutex_lock(&t_counter_mutex);
  list_del(&t_counter_data->list);
  mutex_unlock(&t_counter_mutex);

  // Note: no need for iounmap, since we are using devm_ioremap_resource()
  dev_info(&pdev->dev, "remove.\n");

  return 0;
}

static int __init test_counter_init(void) {
  int ret = 0;
  pr_info("[Driver] %s: initializing.\n", TEST_COUNTER_DRIVER_NAME);

  // Alocate char device
  ret = alloc_chrdev_region(&tc_driver.devnum, 0, NUM_DEVICES, TEST_COUNTER_DRIVER_NAME);
  if (ret) {
    pr_err("%s: Failed to allocate device number!\n", TEST_COUNTER_DRIVER_NAME);
    goto r_exit;
  }

  // Create device class // todo: make a dummy driver just to create and own the
  // class: https://stackoverflow.com/a/16365027/8228163
  if ((tc_driver.class = class_create(THIS_MODULE, TEST_COUNTER_DRIVER_CLASS)) == NULL) {
    printk("Device class can not be created!\n");
    goto r_alloc_region;
  }

  ret = platform_driver_register(&test_counter_driver);
  if (ret < 0) {
    pr_err("%s: Failed to register platform driver!\n",
           TEST_COUNTER_DRIVER_NAME);
    goto r_class;
  }

r_class:
  class_destroy(tc_driver.class);
r_alloc_region:
  unregister_chrdev_region(tc_driver.devnum, NUM_DEVICES);
r_exit:
  return ret;
}

static void __exit test_counter_exit(void) {
  pr_info("[Driver] %s: exiting.\n", TEST_COUNTER_DRIVER_NAME);

  mutex_lock(&t_counter_mutex);
  tc_driver.devnum = MKDEV(MAJOR(tc_driver.devnum), 0);
  mutex_unlock(&t_counter_mutex);

  platform_driver_unregister(&test_counter_driver);

  pr_info("[Driver] %s: platform unregister.\n", TEST_COUNTER_DRIVER_NAME);
  // class_destroy(tc_driver.class);

  pr_info("[Driver] %s: class destroy.\n", TEST_COUNTER_DRIVER_NAME);

  unregister_chrdev_region(tc_driver.devnum, NUM_DEVICES);
  pr_info("[Driver] %s: chrdev region unregister.\n", TEST_COUNTER_DRIVER_NAME);
}

//
// File operations
//

static int test_counter_open(struct inode *inode, struct file *file) {
  struct iob_data *t_counter_data;

  pr_info("[Driver] test_counter device opened\n");

  if (!mutex_trylock(&t_counter_mutex)) {
    pr_info("Another process is accessing the device\n");

    return -EBUSY;
  }

  // assign t_counter_data to file private_data
  list_for_each_entry(t_counter_data, &tc_driver.list, list) {
    if (t_counter_data->devnum == inode->i_rdev) {
      file->private_data = t_counter_data;
      return 0;
    }
  }

  return 0;
}

static int test_counter_release(struct inode *inode, struct file *file) {
  pr_info("[Driver] test_counter device closed\n");

  mutex_unlock(&t_counter_mutex);

  return 0;
}

static ssize_t test_counter_read(struct file *file, char __user *buf,
                                 size_t count, loff_t *ppos) {
  int size = 0;
  u32 value = 0;
  struct iob_data *t_counter_data = (struct iob_data *)file->private_data;

  /* read value from register */
  switch (*ppos) {
  case TEST_COUNTER_ID_ADDR:
    value = iob_data_read_reg(t_counter_data->regbase, TEST_COUNTER_ID_ADDR,
                              TEST_COUNTER_ID_W);
    size = (TEST_COUNTER_ID_W >> 3); // bit to bytes
    pr_info("[Driver] Read ID\n");
    break;
  case TEST_COUNTER_DATA_ADDR:
    value = iob_data_read_reg(t_counter_data->regbase, TEST_COUNTER_DATA_ADDR,
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
  struct iob_data *t_counter_data = (struct iob_data *)file->private_data;

  switch (*ppos) {
  case TEST_COUNTER_RST_ADDR:
    size = (TEST_COUNTER_RST_W >> 3); // bit to bytes
    // if (read_user_data(buf, size, &value))
    //   return -EFAULT;
    iob_data_write_reg(t_counter_data->regbase, 0x1, TEST_COUNTER_RST_ADDR,
                       TEST_COUNTER_RST_W);
    iob_data_write_reg(t_counter_data->regbase, 0x0, TEST_COUNTER_RST_ADDR,
                       TEST_COUNTER_RST_W);
    pr_info("[Driver] Reset counter!\n");
    break;
  case TEST_COUNTER_INCR_ADDR:
    size = (TEST_COUNTER_INCR_W >> 3); // bit to bytes
    // if (read_user_data(buf, size, &value))
    //   return -EFAULT;
    iob_data_write_reg(t_counter_data->regbase, 0x1, TEST_COUNTER_INCR_ADDR,
                       TEST_COUNTER_INCR_W);
    pr_info("[Driver] Increment counter!\n");
    break;
  case TEST_COUNTER_SAMPLE_ADDR:         // sample counter
    size = (TEST_COUNTER_SAMPLE_W >> 3); // bit to bytes
    // if (read_user_data(buf, size, &value))
    //   return -EFAULT;
    iob_data_write_reg(t_counter_data->regbase, 0x1, TEST_COUNTER_SAMPLE_ADDR,
                       TEST_COUNTER_SAMPLE_W);
    pr_info("[Driver] Sample counter!\n");
    break;
  case TEST_COUNTER_SET_ADDR:         // sample counter
    size = (TEST_COUNTER_SET_W >> 3); // bit to bytes
    if (read_user_data(buf, size, &value))
      return -EFAULT;
    iob_data_write_reg(t_counter_data->regbase, value, TEST_COUNTER_SET_ADDR,
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
static loff_t test_counter_llseek(struct file *filp, loff_t offset,
                                  int whence) {
  loff_t new_pos = -1;
  struct iob_data *t_counter_data = (struct iob_data *)filp->private_data;

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
  if (new_pos < 0 || new_pos > t_counter_data->regsize) {
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
