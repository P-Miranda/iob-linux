/*
 * args2.c - Test module arguments, second file
 * - Example usage: insmod args.ko myint=3 mystring="yes" myarray=1,2,3,4
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

static void __exit args_exit(void) { pr_info("Goodbye args module!\n"); }

module_exit(args_exit);
