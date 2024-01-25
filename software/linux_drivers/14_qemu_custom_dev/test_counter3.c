/* test_counter3.c: driver for test counter
 * using device platform. No hardcoded hardware address:
 * 1. setup driver: load lkm driver + add /dev/ file
 *      ./setup.sh 3
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

#include "test_counter.h"

static struct {
  dev_t devnum;
  struct cdev cdev;
  void __iomem *regbase;
  resource_size_t regsize;
} test_counter_data;

static u32 test_counter_read_reg(u32 addr) {
  return ioread32(test_counter_data.regbase + addr);
}

static void test_counter_write_reg(u32 value, u32 addr) {
  iowrite32(value, test_counter_data.regbase + addr);
}

static ssize_t test_counter_read(struct file *file, char __user *buf,
                                 size_t count, loff_t *ppos) {
  int size = REG_SIZE;
  u32 value = 0;

  /* read value from register */
  switch (*ppos) {
  case REG_ID:
    value = test_counter_read_reg(REG_ID);
    pr_info("[Driver] Read ID!\n");
    break;
  case REG_DATA:
    value = test_counter_read_reg(REG_DATA);
    pr_info("[Driver] Read sampled data!\n");
    break;
  default:
    // invalid address - no bytes read
    return 0;
  }

  // pr_info("[Driver] ADDR: 0x%x\tValue: 0x%x\n", (unsigned int)*ppos, value);

  // Read min between count and REG_SIZE
  if (size > count)
    size = count;

  if (copy_to_user(buf, &value, size))
    return -EFAULT;
  *ppos += size;

  return size;
}

/* NOTE: assumes bytes[] at least 4 bytes long */
static u32 char_to_u32(char *bytes) {
  u32 value = bytes[3];
  value = (value << 8) | bytes[2];
  value = (value << 8) | bytes[1];
  value = (value << 8) | bytes[0];
  return value;
}

static ssize_t test_counter_write(struct file *file, const char __user *buf,
                                  size_t count, loff_t *ppos) {
  int size = REG_SIZE;
  char kbuf[REG_SIZE] = {0};

  // Write min between size and count
  if (size > count)
    size = count;

  if (copy_from_user(&kbuf, buf, size))
    return -EFAULT;

  switch (*ppos) {
  case REG_RST: // reset counter
    test_counter_write_reg(0x1, REG_RST);
    test_counter_write_reg(0x0, REG_RST);
    pr_info("[Driver] Reset counter!\n");
    break;
  case REG_INCR: // increment counter
    test_counter_write_reg(0x1, REG_INCR);
    pr_info("[Driver] Increment counter!\n");
    break;
  case REG_SAMPLE: // sample counter
    test_counter_write_reg(0x1, REG_SAMPLE);
    pr_info("[Driver] Sample counter!\n");
    break;
  case REG_SET: // set counter
    u32 value = char_to_u32(kbuf);
    test_counter_write_reg(value, REG_SET);
    pr_info("[Driver] Set counter to 0x%x!\n", value);
    break;
  default:
    pr_info("[Driver] Invalid write address 0x%x\n", (unsigned int)*ppos);
    // invalid address - no bytes written
    return 0;
  }

  return size;
}

/* Custom lseek function
 * assumes test_counter file size is REG_SIZE
 * check: lseek(2) man page for whence modes
 */
loff_t test_counter_llseek(struct file *filp, loff_t offset, int whence) {
  loff_t new_pos = -1;

  // pr_info("[Driver] Lseek: offset %lld, whence %d\n", offset, whence);

  switch (whence) {
  case SEEK_SET:
    new_pos = offset;
    break;
  case SEEK_CUR:
    new_pos = filp->f_pos + offset;
    break;
  case SEEK_END:
    new_pos = REG_SIZE + offset;
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

static const struct file_operations test_counter_fops = {
    .owner = THIS_MODULE,
    .write = test_counter_write,
    .read = test_counter_read,
    .llseek = test_counter_llseek,
};

static int test_counter_probe(struct platform_device *pdev) {
  struct resource *res;
  int result = 0;

  pr_info("[Driver] %s: probing.\n", DRIVER_NAME);

  // Get the I/O region base address
  res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (!res) {
    pr_err("[Driver]: Failed to get I/O resource!\n");
    result = -ENODEV;
    goto ret_platform_get_resource;
  }

  // Request and map the I/O region
  test_counter_data.regbase = devm_ioremap_resource(&pdev->dev, res);
  if (IS_ERR(test_counter_data.regbase)) {
    result = PTR_ERR(test_counter_data.regbase);
    goto ret_devm_ioremmap_resource;
  }
  test_counter_data.regsize = resource_size(res);

  // Alocate char device
  result = alloc_chrdev_region(&test_counter_data.devnum, 0, 1, DRIVER_NAME);
  if (result) {
    pr_err("%s: Failed to allocate device number!\n", DRIVER_NAME);
    goto ret_err_alloc_chrdev_region;
  }

  cdev_init(&test_counter_data.cdev, &test_counter_fops);

  result = cdev_add(&test_counter_data.cdev, test_counter_data.devnum, 1);
  if (result) {
    pr_err("%s: Char device registration failed!\n", DRIVER_NAME);
    unregister_chrdev_region(test_counter_data.devnum, 1);
    goto ret_err_cdev_add;
  }

  dev_info(&pdev->dev, "initialized.\n");
  goto ret_ok;

ret_err_cdev_add:
  unregister_chrdev_region(test_counter_data.devnum, 1);
ret_err_alloc_chrdev_region:
  // iounmap is managed by devm
ret_devm_ioremmap_resource:
ret_platform_get_resource:
ret_ok:
  return result;
}

static int test_counter_remove(struct platform_device *pdev) {
  // Note: no need for iounmap, since we are using devm_ioremap_resource()
  cdev_del(&test_counter_data.cdev);
  unregister_chrdev_region(test_counter_data.devnum, 1);
  dev_info(&pdev->dev, "exiting.\n");
  return 0;
}

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

/* Replaces module_init() and module_exit() */
module_platform_driver(test_counter_driver);

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("IObundle");
MODULE_DESCRIPTION("Test counter driver");
MODULE_VERSION("0.3");
