#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "iob-sysfs-common.h"
/* Test_Counter header automatically generated from:
 * /path/to/iob-linux/scripts/drivers.py test_counter
 * contains:
 * - sysfs file paths
 * - Register address and width definitions
 */
#include "test_counter.h"

#define TEST_COUNTER_CLASS_PATH "/sys/devices/virtual/test_counter/test_counter"

#define MAX_FILENAME 256
static char fname[MAX_FILENAME] = {0};

/* Print ID | sampled data */
int get_counter_status(int minor) {
  int ret = -1;
  uint32_t id = 0;
  uint32_t sampled_data = 0;

  ret = iob_sysfs_read_file(
      iob_sysfs_gen_fname(fname, TEST_COUNTER_CLASS_PATH, minor, "id"), &id);
  if (ret == -1) {
    return -1;
  }
  ret = iob_sysfs_read_file(
      iob_sysfs_gen_fname(fname, TEST_COUNTER_CLASS_PATH, minor, "data"),
      &sampled_data);
  if (ret == -1) {
    return -1;
  }

  printf("[User] ID: 0x%x\tSampled data: 0x%x\n", id, sampled_data);
  return ret;
}

int test_counter_program(int dev_minor) {
  if (get_counter_status(dev_minor) == -1) {
    return EXIT_FAILURE;
  }
  // Increment x3
  int i = 0;
  for (i = 0; i < 3; i++) {
    if (iob_sysfs_write_file(iob_sysfs_gen_fname(fname, TEST_COUNTER_CLASS_PATH,
                                                 dev_minor, "incr"),
                             1) == -1) {
      return EXIT_FAILURE;
    }
  }
  if (get_counter_status(dev_minor) == -1) {
    return EXIT_FAILURE;
  }
  // Sample
  if (iob_sysfs_write_file(iob_sysfs_gen_fname(fname, TEST_COUNTER_CLASS_PATH,
                                               dev_minor, "sample"),
                           1) == -1) {
    return EXIT_FAILURE;
  }
  if (get_counter_status(dev_minor) == -1) {
    return EXIT_FAILURE;
  }
  // Set
  if (iob_sysfs_write_file(
          iob_sysfs_gen_fname(fname, TEST_COUNTER_CLASS_PATH, dev_minor, "set"),
          8) == -1) {
    return EXIT_FAILURE;
  }
  if (iob_sysfs_write_file(iob_sysfs_gen_fname(fname, TEST_COUNTER_CLASS_PATH,
                                               dev_minor, "sample"),
                           1) == -1) {
    return EXIT_FAILURE;
  }
  if (get_counter_status(dev_minor) == -1) {
    return EXIT_FAILURE;
  }
  // Reset
  if (iob_sysfs_write_file(
          iob_sysfs_gen_fname(fname, TEST_COUNTER_CLASS_PATH, dev_minor, "rst"),
          1) == -1) {
    return EXIT_FAILURE;
  }
  if (iob_sysfs_write_file(iob_sysfs_gen_fname(fname, TEST_COUNTER_CLASS_PATH,
                                               dev_minor, "sample"),
                           1) == -1) {
    return EXIT_FAILURE;
  }
  if (get_counter_status(dev_minor) == -1) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  printf("[User] User test_counter application\n");

  if (test_counter_program(0) == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }

  if (test_counter_program(1) == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
