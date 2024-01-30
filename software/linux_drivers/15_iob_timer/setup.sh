#!/usr/bin/env sh

DRIVER=iob_timer

# Load kernel module
insmod $DRIVER.ko

# get the driver major number
MAJOR=$(grep $DRIVER /proc/devices | awk '{print $1}')
# create device file
mknod /dev/$DRIVER c $MAJOR 0
