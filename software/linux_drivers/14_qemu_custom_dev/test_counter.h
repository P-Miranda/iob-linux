#ifndef H_TEST_COUNTER2_H
#define H_TEST_COUNTER2_H

#define DRIVER_NAME "test_counter"
#define DEVICE_FILE "/dev/test_counter"

/* Register map */
#define REG_ID 0x00     // R: ID/Version
#define REG_RST 0x04    // W: Reset counter
#define REG_INCR 0x08   // W: Increment counter
#define REG_SAMPLE 0x0C // W: Sample counter
#define REG_DATA 0x10   // R: Read sampled data
#define REG_SET 0x14    // W: Set counter to value

#define REG_SIZE 0x4

#endif
