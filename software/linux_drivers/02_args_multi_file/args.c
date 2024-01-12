/*
 * args.c - Test module arguments
 * - Example usage: insmod args.ko myint=3 mystring="yes" myarray=1,2,3,4
 */
#include <linux/init.h>
#include <linux/kernel.h> /* for ARRAY_SIZE */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/printk.h>
#include <linux/stat.h> /* file permission macros */

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("IObundle");
MODULE_DESCRIPTION("Args Module. Test reading arguments from command line.");

#define MAX_ARRAY_SIZE (5)

/* arguments to be passed to module */
static int myint = 420;
static char *mystring = "default";
static int myarray[MAX_ARRAY_SIZE] = {-1, -1, -1, -1, -1};
static int myarray_argc = 0;

/*
 * module_param(name, type, perm)
 * name: variable name
 * type: variable type
 * perm: permission bits of file in sysfs
 */
module_param(myint, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(myint, "An integer");
module_param(mystring, charp, 0000);
MODULE_PARM_DESC(mystring, "A character string");

/*
 * module_param_array(name, type, num, perm)
 * name: variable name
 * type: array element type
 * num: pointer to variable that stores array size
 * perm: permission bits
 */
module_param_array(myarray, int, &myarray_argc, 0000);
MODULE_PARM_DESC(myarray, "An array of integers");

static int __init args_init(void) {
  int i;
  pr_info("Hello args module\n");
  pr_info("\n===\n");
  pr_info("myint is: %d\n", myint);
  pr_info("mystring is: %s\n ", mystring);
  for (i = 0; i < ARRAY_SIZE(myarray); i++) {
    pr_info("myarray[%d] = %d\n", i, myarray[i]);
  }
  pr_info("got %d arguments for myarray.\n", myarray_argc);
  return 0;
}

module_init(args_init);
