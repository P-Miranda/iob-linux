#ifndef H_MAIN_H
#define H_MAIN_H

#include <linux/cdev.h>

u32 iob_data_read_reg(void __iomem *, u32, u32);
void iob_data_write_reg(void __iomem *, u32, u32, u32);
int read_user_data(const char *, int, u32 *);
u32 char_to_u32(char *, u32);

struct iob_data {
	dev_t devnum;
	struct cdev cdev;
	void __iomem *regbase;
	resource_size_t regsize;
	struct device *device;
};

//extern struct class *iob_class;

#endif // H_MAIN_H
