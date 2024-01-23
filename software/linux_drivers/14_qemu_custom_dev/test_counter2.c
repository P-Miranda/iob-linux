/* test_counter2.c: driver for test counter
 * hardcoded hardware access:
 * 1. setup driver: load lkm driver + add /dev/ file
 *      ./setup.sh 2
 * 2. run user app: ./user/user
 */

// TODO:
// 1. user app to test all regs and functions (emulate user.sh)

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#include "test_counter2.h"

/* Same base and size from device tree.
 * Using hardcoded values for now.
 * This will be improved in a later version.
 */
#define TEST_COUNTER_BASE 0x8000000
#define TEST_COUNTER_SIZE 0x100

static struct {
  dev_t devnum;
  struct cdev cdev;
  // unsigned int id;
  // unsigned int counter;
  // unsigned int data;
  void __iomem *regbase;
} test_counter_data;

static u32 test_counter_read_reg(u32 addr) {
  return readl(test_counter_data.regbase + addr);
}

static void test_counter_write_reg(u32 value, u32 addr) {
  writel(value, test_counter_data.regbase + addr);
}

static ssize_t test_counter_read(struct file *file, char __user *buf,
                                 size_t count, loff_t *ppos) {
  int size = REG_SIZE;
  u32 value = 0;

  /* check if EOF */
  if (*ppos > 0)
    return 0;

  /* read value from register */
  switch (*ppos) {
  case REG_ID:
    value = test_counter_read_reg(REG_ID);
    break;
  case REG_DATA:
    value = test_counter_read_reg(REG_DATA);
    break;
  default:
    value = -1;
  }

  pr_info("ADDR: %x\tValue: %d\n", (unsigned int) *ppos, value);

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
    pr_info("Reset counter!\n");
    break;
  case REG_INCR: // increment counter
    test_counter_write_reg(0x1, REG_INCR);
    pr_info("Increment counter!\n");
    break;
  case REG_SAMPLE: // sample counter
    test_counter_write_reg(0x1, REG_SAMPLE);
    pr_info("Sample counter!\n");
    break;
  case REG_SET: // set counter
    u32 value = char_to_u32(kbuf);
    test_counter_write_reg(value, REG_SET);
    pr_info("Set counter to %d!\n", value);
    break;
  default:
    pr_info("Invalid write address %x\n", (unsigned int) *ppos);
  }

  return size;
}

/* Custom lseek function
 * assumes test_counter file size is REG_SIZE
 * check: lseek(2) man page for whence modes
 */
loff_t test_counter_llseek(struct file *filp, loff_t offset, int whence) {
  loff_t new_pos = -1;

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
  if (new_pos < 0 || new_pos > TEST_COUNTER_SIZE) {
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

static int __init test_counter_init(void) {
  int result = 0;

  if (!request_mem_region(TEST_COUNTER_BASE, TEST_COUNTER_SIZE, DRIVER_NAME)) {
    pr_err("%s: Error requesting I/O!\n", DRIVER_NAME);
    result = -EBUSY;
    goto ret_err_request_mem_region;
  }

  test_counter_data.regbase = ioremap(TEST_COUNTER_BASE, TEST_COUNTER_SIZE);
  if (!test_counter_data.regbase) {
    pr_err("%s: Error mapping I/O!\n", DRIVER_NAME);
    result = -ENOMEM;
    goto err_ioremap;
  }

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

  pr_info("%s: initialized.\n", DRIVER_NAME);
  goto ret_ok;

ret_err_cdev_add:
  unregister_chrdev_region(test_counter_data.devnum, 1);
ret_err_alloc_chrdev_region:
  iounmap(test_counter_data.regbase);
err_ioremap:
  release_mem_region(TEST_COUNTER_BASE, TEST_COUNTER_SIZE);
ret_err_request_mem_region:
ret_ok:
  return result;
}

static void __exit test_counter_exit(void) {
  cdev_del(&test_counter_data.cdev);
  unregister_chrdev_region(test_counter_data.devnum, 1);
  iounmap(test_counter_data.regbase);
  release_mem_region(TEST_COUNTER_BASE, TEST_COUNTER_SIZE);
  pr_info("%s: exiting.\n", DRIVER_NAME);
}

module_init(test_counter_init);
module_exit(test_counter_exit);

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("IObundle");
MODULE_DESCRIPTION("Test counter driver");
MODULE_VERSION("0.2");
