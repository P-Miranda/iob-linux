#include "iob_timer.h"
#include "main.h"

// This function will be called when we read the sysfs file
ssize_t sysfs_clocks_show(struct device *dev, struct device_attribute *attr, char *buf) {
	u64 value = (iob_timer_read_reg(IOB_TIMER_DATA_HIGH_ADDR, IOB_TIMER_DATA_HIGH_W) << 4) |
				iob_timer_read_reg(IOB_TIMER_DATA_LOW_ADDR, IOB_TIMER_DATA_LOW_W);

	pr_info("Sysfs - Read!!!%llu\n", value);

	return sprintf(buf, "%llu", value);
}

// This function will be called when we write the sysfsfs file
ssize_t sysfs_clocks_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	return FUNC_NOT_AVAILABLE;
}

////////////////////////////////////////////////////

ssize_t sysfs_reset_show(struct device *dev,
                				struct device_attribute *attr, char *buf) {
	return FUNC_NOT_AVAILABLE;
}

ssize_t sysfs_reset_store(struct device *dev, struct device_attribute *attr, const char __user *buf, size_t count) {
	u8 value = 0;
	sscanf(buf, "%c", &value);
	iob_timer_write_reg(value, IOB_TIMER_RESET_ADDR, IOB_TIMER_RESET_W);

	return 0;
}

////////////////////////////////////////////////////

ssize_t sysfs_enable_show(struct device *dev, struct device_attribute *attr, char *buf) {
	return FUNC_NOT_AVAILABLE;
}

ssize_t sysfs_enable_store(struct device *dev, struct device_attribute *attr, const char __user *buf, size_t count) {
	u8 value = 0;
	sscanf(buf, "%c", &value);
	iob_timer_write_reg(value, IOB_TIMER_ENABLE_ADDR, IOB_TIMER_ENABLE_W);

	return 0;
}
