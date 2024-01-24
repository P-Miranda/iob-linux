#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "test_counter.h"

uint32_t read_reg(int fd, uint32_t addr, uint32_t *value) {
  ssize_t ret = -1;

  // printf("[User] read addr: 0x%x\n", addr);

  if (fd == 0) {
    perror("[User] Invalid file descriptor");
    return -1;
  }

  // Point to register address
  if (lseek(fd, addr, SEEK_SET) == -1) {
    perror("[User] Failed to seek to register");
    return -1;
  }

  // Read value from device
  ret = read(fd, value, sizeof(*value));
  if (ret == -1) {
    perror("[User] Failed to read from device");
  }

  return ret;
}

uint32_t write_reg(int fd, uint32_t addr, uint32_t value) {
  ssize_t ret = -1;

  // printf("[User] write addr: 0x%x\tvalue: 0x%x\n", addr, value);

  if (fd == 0) {
    perror("[User] Invalid file descriptor");
    return -1;
  }

  // Point to register address
  if (lseek(fd, addr, SEEK_SET) == -1) {
    perror("[User] Failed to seek to register");
    return -1;
  }

  // Write value to device
  ret = write(fd, &value, sizeof(value));
  if (ret == -1) {
    perror("[User] Failed to write to device");
  }

  return ret;
}

/* Print ID | sampled data */
uint32_t get_counter_status(int fd) {
  uint32_t ret = -1;
  uint32_t id = 0;
  uint32_t sampled_data = 0;

  ret = read_reg(fd, REG_ID, &id);
  if (ret == -1) {
    close(fd);
    return ret;
  }
  ret = read_reg(fd, REG_DATA, &sampled_data);
  if (ret == -1) {
    close(fd);
    return ret;
  }

  printf("[User] ID: 0x%x\tSampled data: 0x%x\n", id, sampled_data);
  return ret;
}

int main(int argc, char *argv[]) {
  printf("[User] User 2 application\n");

  int fd = 0;

  // Open device for read and write
  fd = open(DEVICE_FILE, O_RDWR);
  if (fd == -1) {
    perror("[User] Failed to open the device file");
    return EXIT_FAILURE;
  }

  if (get_counter_status(fd) == -1) {
    return EXIT_FAILURE;
  }
  // Increment x3
  int i = 0;
  for (i = 0; i < 3; i++) {
    if (write_reg(fd, REG_INCR, 1) == -1) {
      return EXIT_FAILURE;
    }
  }
  if (get_counter_status(fd) == -1) {
    return EXIT_FAILURE;
  }
  // Sample
  if (write_reg(fd, REG_SAMPLE, 1) == -1) {
    return EXIT_FAILURE;
  }
  if (get_counter_status(fd) == -1) {
    return EXIT_FAILURE;
  }
  // Set
  if (write_reg(fd, REG_SET, 8) == -1) {
    return EXIT_FAILURE;
  }
  if (write_reg(fd, REG_SAMPLE, 1) == -1) {
    return EXIT_FAILURE;
  }
  if (get_counter_status(fd) == -1) {
    return EXIT_FAILURE;
  }
  // Reset
  if (write_reg(fd, REG_RST, 1) == -1) {
    return EXIT_FAILURE;
  }
  if (write_reg(fd, REG_SAMPLE, 1) == -1) {
    return EXIT_FAILURE;
  }
  if (get_counter_status(fd) == -1) {
    return EXIT_FAILURE;
  }

  close(fd);
  return EXIT_SUCCESS;
}
