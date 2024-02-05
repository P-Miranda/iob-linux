/* iob_timer.c: driver for iob_timer
 * using device platform. No hardcoded hardware address:
 * 1. load driver: insmod iob_timer.ko
 * 2. run user app: ./user/user
 */

#include "iob_timer.h"
#include "main.h"


static int iob_timer_probe(struct platform_device *);
static int iob_timer_remove(struct platform_device *);


struct iob_data iob_timer_data = {0};

static const struct file_operations iob_timer_fops = {
		.owner = THIS_MODULE,
		.write = iob_timer_write,
		.read = iob_timer_read,
		.llseek = iob_timer_llseek,
};

static const struct of_device_id of_iob_timer_match[] = {
		{.compatible = "iobundle,timer0"},
		{},
};

static struct platform_driver iob_timer_driver = {
		.driver =
				{
						.name = "iob_timer",
						.owner = THIS_MODULE,
						.of_match_table = of_iob_timer_match,
				},
		.probe = iob_timer_probe,
		.remove = iob_timer_remove,
};

// These below create struct variables named dev_attr_##_name (example, dev_attr_clocks).
DEVICE_ATTR(clocks, 0644, sysfs_clocks_show, sysfs_clocks_store);
DEVICE_ATTR(reset, 0644, sysfs_reset_show, sysfs_reset_store);
DEVICE_ATTR(enable, 0644, sysfs_enable_show, sysfs_enable_store);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static int iob_timer_probe(struct platform_device *pdev) {
	struct resource *res;
	int result = 0;

	pr_info("[Driver] %s: probing.\n", DRIVER_NAME);

	// Get the I/O region base address
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("[Driver]: Failed to get I/O resource!\n");
		result = -ENODEV;
		goto ret_platform_get_resource;
	}

	// Request and map the I/O region
	iob_timer_data.regbase = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(iob_timer_data.regbase)) {
		result = PTR_ERR(iob_timer_data.regbase);
		goto ret_devm_ioremmap_resource;
	}
	iob_timer_data.regsize = resource_size(res);

	// Alocate char device
	result = alloc_chrdev_region(&iob_timer_data.devnum, 0, 1, DRIVER_NAME);
	if (result) {
		pr_err("%s: Failed to allocate device number!\n", DRIVER_NAME);
		goto ret_err_alloc_chrdev_region;
	}

	// Create device class
	if ((iob_timer_data.timer_class = class_create(THIS_MODULE, DRIVER_CLASS)) ==
			NULL) {
		printk("Device class can not be created!\n");
		goto class_error;
	}

	// create device file
	if (device_create(iob_timer_data.timer_class, NULL, iob_timer_data.devnum,
										NULL, DRIVER_NAME) == NULL) {
		printk("Can not create device file!\n");
		goto file_error;
	}

	cdev_init(&iob_timer_data.cdev, &iob_timer_fops);

	result = cdev_add(&iob_timer_data.cdev, iob_timer_data.devnum, 1);
	if (result) {
		pr_err("%s: Char device registration failed!\n", DRIVER_NAME);
		goto ret_err_cdev_add;
	}

	/*Creating a directory in /sys/kernel/ */
	iob_timer_data.kobj_ref = kobject_create_and_add(DRIVER_NAME, kernel_kobj);

	/*Creating sysfs file for data_high*/
	if (sysfs_create_file(iob_timer_data.kobj_ref, &dev_attr_clocks.attr) ||
			sysfs_create_file(iob_timer_data.kobj_ref, &dev_attr_reset.attr) ||
			sysfs_create_file(iob_timer_data.kobj_ref, &dev_attr_enable.attr)) {
		pr_err("Cannot create sysfs file......\n");
		goto r_sysfs;
    }

	dev_info(&pdev->dev, "initialized.\n");
	goto ret_ok;

r_sysfs:
	kobject_put(iob_timer_data.kobj_ref);
	sysfs_remove_file(kernel_kobj, &dev_attr_clocks.attr);
	sysfs_remove_file(kernel_kobj, &dev_attr_reset.attr);
	sysfs_remove_file(kernel_kobj, &dev_attr_enable.attr);
ret_err_cdev_add:
	device_destroy(iob_timer_data.timer_class, iob_timer_data.devnum);
file_error:
	class_destroy(iob_timer_data.timer_class);
class_error:
	unregister_chrdev_region(iob_timer_data.devnum, 1);
ret_err_alloc_chrdev_region:
	// iounmap is managed by devm
ret_devm_ioremmap_resource:
ret_platform_get_resource:
ret_ok:
	return result;
}

static int iob_timer_remove(struct platform_device *pdev) {
	kobject_put(iob_timer_data.kobj_ref);
	sysfs_remove_file(kernel_kobj, &dev_attr_clocks.attr);
	// Note: no need for iounmap, since we are using devm_ioremap_resource()
	device_destroy(iob_timer_data.timer_class, iob_timer_data.devnum);
	class_destroy(iob_timer_data.timer_class);
	cdev_del(&iob_timer_data.cdev);
	unregister_chrdev_region(iob_timer_data.devnum, 1);
	dev_info(&pdev->dev, "exiting.\n");
	return 0;
}

static int __init test_counter_init(void) {
	pr_info("[Driver] %s: initializing.\n", DRIVER_NAME);

	return platform_driver_register(&iob_timer_driver);
}

static void __exit test_counter_exit(void) {
	pr_info("[Driver] %s: exiting.\n", DRIVER_NAME);
	platform_driver_unregister(&iob_timer_driver);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

module_init(test_counter_init);
module_exit(test_counter_exit);

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("IObundle");
MODULE_DESCRIPTION("IOb-Timer Drivers");
MODULE_VERSION("0.10");
