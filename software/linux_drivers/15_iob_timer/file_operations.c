/* iob_timer.c: driver for iob_timer
 * using device platform. No hardcoded hardware address:
 * 1. load driver: insmod iob_timer.ko
 * 2. run user app: ./user/user
 */

#include "iob_timer.h"
#include "main.h"


ssize_t iob_timer_read(struct file *file, char __user *buf, size_t count,
															loff_t *ppos) {
	int size = 0;
	u32 value = 0;

	/* read value from register */
	switch (*ppos) {
	case IOB_TIMER_DATA_LOW_ADDR:
		value = iob_timer_read_reg(IOB_TIMER_DATA_LOW_ADDR, IOB_TIMER_DATA_LOW_W);
		size = (IOB_TIMER_DATA_LOW_W >> 3); // bit to bytes
		pr_info("[Driver] Read data low!\n");
		break;
	case IOB_TIMER_DATA_HIGH_ADDR:
		value = iob_timer_read_reg(IOB_TIMER_DATA_HIGH_ADDR, IOB_TIMER_DATA_HIGH_W);
		size = (IOB_TIMER_DATA_HIGH_W >> 3); // bit to bytes
		pr_info("[Driver] Read data high!\n");
		break;
	case IOB_TIMER_VERSION_ADDR:
		value = iob_timer_read_reg(IOB_TIMER_VERSION_ADDR, IOB_TIMER_VERSION_W);
		size = (IOB_TIMER_VERSION_W >> 3); // bit to bytes
		pr_info("[Driver] Read version!\n");
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
	*ppos += size;

	return size;
}

ssize_t iob_timer_write(struct file *file, const char __user *buf,
															 size_t count, loff_t *ppos) {
	int size = 0;
	u32 value = 0;

	switch (*ppos) {
	case IOB_TIMER_RESET_ADDR:
		size = (IOB_TIMER_RESET_W >> 3); // bit to bytes
		if (read_user_data(buf, size, &value))
			return -EFAULT;
		iob_timer_write_reg(value, IOB_TIMER_RESET_ADDR, IOB_TIMER_RESET_W);
		pr_info("[Driver] Reset iob_timer: 0x%x\n", value);
		break;
	case IOB_TIMER_ENABLE_ADDR:
		size = (IOB_TIMER_ENABLE_W >> 3); // bit to bytes
		if (read_user_data(buf, size, &value))
			return -EFAULT;
		iob_timer_write_reg(value, IOB_TIMER_ENABLE_ADDR, IOB_TIMER_ENABLE_W);
		pr_info("[Driver] Enable iob_timer: 0x%x\n", value);
		break;
	case IOB_TIMER_SAMPLE_ADDR:         // sample counter
		size = (IOB_TIMER_SAMPLE_W >> 3); // bit to bytes
		if (read_user_data(buf, size, &value))
			return -EFAULT;
		iob_timer_write_reg(value, IOB_TIMER_SAMPLE_ADDR, IOB_TIMER_SAMPLE_W);
		pr_info("[Driver] Sample iob_timer: 0x%x\n", value);
		break;
	default:
		pr_info("[Driver] Invalid write address 0x%x\n", (unsigned int)*ppos);
		// invalid address - no bytes written
		return 0;
	}

	return size;
}

/* Custom lseek function
 * check: lseek(2) man page for whence modes
 */
loff_t iob_timer_llseek(struct file *filp, loff_t offset, int whence) {
	loff_t new_pos = -1;

	switch (whence) {
	case SEEK_SET:
		new_pos = offset;
		break;
	case SEEK_CUR:
		new_pos = filp->f_pos + offset;
		break;
	case SEEK_END:
		new_pos = (1 << IOB_TIMER_SWREG_ADDR_W) + offset;
		break;
	default:
		return -EINVAL;
	}

	// Check for valid bounds
	if (new_pos < 0 || new_pos > iob_timer_data.regsize) {
		return -EINVAL;
	}

	// Update file position
	filp->f_pos = new_pos;

	return new_pos;
}
