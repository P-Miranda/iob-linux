MODULE=test_counter
insmod ${MODULE}1.ko
MAJOR=$(cat /proc/devices | grep $MODULE | awk '{print $1}')
DEV_FILE='/dev/$MODULE'
mknod $DEV_FILE c $MAJOR 0

# read from device
cat $DEV_FILE
# increment
echo "1" > $DEV_FILE
echo "1" > $DEV_FILE
echo "1" > $DEV_FILE
cat $DEV_FILE
# sample
echo "2" > $DEV_FILE
cat $DEV_FILE
# set
echo "8" > $DEV_FILE
cat $DEV_FILE
# reset
echo "0" > $DEV_FILE
cat $DEV_FILE
