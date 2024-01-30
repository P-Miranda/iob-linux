#!/usr/bin/env sh

# check arguments
if [ $# -ne 1 ]; then
    echo "Usage: $0 <version>"
    exit 1
fi

VERSION=$1
DRIVER=test_counter

# Load kernel module
insmod $DRIVER$VERSION.ko

# get the driver major number
MAJOR=$(grep $DRIVER /proc/devices | awk '{print $1}')
# create device file
mknod /dev/$DRIVER c $MAJOR 0
