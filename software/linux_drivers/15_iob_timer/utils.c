/* iob_timer.c: driver for iob_timer
 * using device platform. No hardcoded hardware address:
 * 1. load driver: insmod iob_timer.ko
 * 2. run user app: ./user/user
 */

#include "iob_timer.h"
#include "main.h"

u32 iob_timer_read_reg(u32 addr, u32 nbits) {
	u32 value = 0;
	switch (nbits) {
	case 8:
		value = (u32) ioread8(iob_timer_data.regbase + addr);
		break;
	case 16:
		value = (u32) ioread16(iob_timer_data.regbase + addr);
		break;
	default:
		value = ioread32(iob_timer_data.regbase + addr);
		break;
	}
	return value;
}

void iob_timer_write_reg(u32 value, u32 addr, u32 nbits) {
	switch (nbits) {
	case 8:
		iowrite8(value, iob_timer_data.regbase + addr);
		break;
	case 16:
		iowrite16(value, iob_timer_data.regbase + addr);
		break;
	default:
		iowrite32(value, iob_timer_data.regbase + addr);
		break;
	}
}

/* read 1-4 bytes from char array into u32
 * NOTE: assumes bytes[] at least nbytes long
 * */
u32 char_to_u32(char *bytes, u32 nbytes) {
	u32 value = 0;
	while (nbytes--) {
		value = (value << 8) | ((u32)bytes[nbytes]);
	}
	return value;
}

/* read `size` bytes from user `buf` into `value`
 * return 0 on success, -EFAULT on error
 */
int read_user_data(const char *buf, int size, u32 *value) {
	char kbuf[4] = {0}; // max 32 bit value
	if (copy_from_user(&kbuf, buf, size))
		return -EFAULT;
	*value = char_to_u32(kbuf, size);
	return 0;
}
