/* test_counter1.c: mock driver for test counter
 * does not actually access hardware:
 * Option 1: run user1.sh script: sh user1.sh
 * Option 2: manually run commands: 
 *   1. add kernel module: insmod test_counter1.ko
 *   2. add device file: mknod /dev/test_counter c [major] 0
 *     2.1 [major] is defined in cat /proc/devices | grep test_counter
 *   3. read device: cat /dev/test_counter
 *   4. write to device: echo "1" > /dev/test_counter
 *    4.1: reset: echo "0" > /dev/test_counter
 *    4.2: increment: echo "1" > /dev/test_counter
 *    4.3: sample: echo "2" > /dev/test_counter
 *    4.4: set: echo "[3-9]" > /dev/test_counter
 */
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#include "test_counter.h"

#define CHIP_ID 0x1234

#define BUF_LEN 80
static char msg[BUF_LEN + 1];

static struct {
  dev_t devnum;
  struct cdev cdev;
  unsigned int id;
  unsigned int counter;
  unsigned int data;
} test_counter_data;

static void test_counter_reset(void) {
  test_counter_data.id = CHIP_ID;
  test_counter_data.counter = 0;
  test_counter_data.data = 0;
}

static ssize_t test_counter_read(struct file *file, char __user *buf,
                                 size_t count, loff_t *ppos) {
  int size = 0;

  /* check if EOF */
  if (*ppos > 0)
    return 0;

  sprintf(msg, "ID: %d\tCounter: %d\tSampled data: %x\n", test_counter_data.id,
          test_counter_data.counter, test_counter_data.data);

  size = strlen(msg);
  if (size > count)
    size = count;

  if (copy_to_user(buf, msg, size))
    return -EFAULT;
  *ppos += size;

  return size;
}

static ssize_t test_counter_write(struct file *file, const char __user *buf,
                                  size_t count, loff_t *ppos) {
  char kbuf = 0;

  if (copy_from_user(&kbuf, buf, 1))
    return -EFAULT;

  switch (kbuf) {
  case '0': // reset counter
    test_counter_data.counter = 0;
    pr_info("Reset counter!\n");
    break;
  case '1': // increment counter
    test_counter_data.counter++;
    pr_info("Increment counter!\n");
    break;
  case '2': // sample counter
    test_counter_data.data = test_counter_data.counter;
    pr_info("Sample counter!\n");
    break;
  default:
    // if kbuf is 3-9, set counter to kbuf
    pr_info("default case\n");
    if (kbuf >= '3' && kbuf <= '9') {
      int val = kbuf - '0';
      test_counter_data.counter = val;
      pr_info("Set counter!\n");
    }
    break;
  }

  return count;
}

static const struct file_operations test_counter_fops = {
    .owner = THIS_MODULE,
    .write = test_counter_write,
    .read = test_counter_read,
};

static int __init test_counter_init(void) {
  int result;

  result = alloc_chrdev_region(&test_counter_data.devnum, 0, 1, DRIVER_NAME);
  if (result) {
    pr_err("%s: Failed to allocate device number!\n", DRIVER_NAME);
    return result;
  }

  cdev_init(&test_counter_data.cdev, &test_counter_fops);

  // reset counter struct before adding device?
  test_counter_reset();

  result = cdev_add(&test_counter_data.cdev, test_counter_data.devnum, 1);
  if (result) {
    pr_err("%s: Char device registration failed!\n", DRIVER_NAME);
    unregister_chrdev_region(test_counter_data.devnum, 1);
    return result;
  }

  pr_info("%s: initialized.\n", DRIVER_NAME);
  return 0;
}

static void __exit test_counter_exit(void) {
  cdev_del(&test_counter_data.cdev);
  unregister_chrdev_region(test_counter_data.devnum, 1);
  pr_info("%s: exiting.\n", DRIVER_NAME);
}

module_init(test_counter_init);
module_exit(test_counter_exit);

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("IObundle");
MODULE_DESCRIPTION("Test counter driver");
MODULE_VERSION("0.1");
