#ifndef H_IOB_TIMER_H
#define H_IOB_TIMER_H

#define DRIVER_NAME "iob_timer"
#define DRIVER_CLASS "iob_class"
#define DEVICE_FILE "/dev/iob_timer"

#define DEVICE_CLASS "/sys/class/" DRIVER_CLASS "/" DRIVER_NAME

#define FILE_RESET DEVICE_CLASS "/reset"
#define FILE_ENABLE DEVICE_CLASS "/enable"
#define FILE_SAMPLE DEVICE_CLASS "/sample"
#define FILE_DATA_LOW DEVICE_CLASS "/data_low"
#define FILE_DATA_HIGH DEVICE_CLASS "/data_high"
#define FILE_VERSION DEVICE_CLASS "/version"

/* Register map */
// Copy from iob_timer_swreg.h
//used address space width
#define IOB_TIMER_SWREG_ADDR_W 4

//Addresses
#define IOB_TIMER_RESET_ADDR 0      // W
#define IOB_TIMER_ENABLE_ADDR 1     // W
#define IOB_TIMER_SAMPLE_ADDR 2     // W
#define IOB_TIMER_DATA_LOW_ADDR 4   // R
#define IOB_TIMER_DATA_HIGH_ADDR 8  // R
#define IOB_TIMER_VERSION_ADDR 12   // R

//Data widths (bit)
#define IOB_TIMER_RESET_W 8
#define IOB_TIMER_ENABLE_W 8
#define IOB_TIMER_SAMPLE_W 8
#define IOB_TIMER_DATA_LOW_W 32
#define IOB_TIMER_DATA_HIGH_W 32
#define IOB_TIMER_VERSION_W 16

#endif // H_IOB_TIMER_H
