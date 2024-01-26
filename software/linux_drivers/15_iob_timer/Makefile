obj-m += iob_timer.o

LINUX_KERNEL_DIR = ../../../submodules/linux-5.15.98

# Options to cross compile for RISCV system in a x86 host machine
CROSS_COMPILE_OPS = "ARCH=riscv"
CROSS_COMPILE_OPS += "CROSS_COMPILE=riscv64-unknown-linux-gnu-"

# Linux Kernel Module variables in target Rootfs
ROOTFS_DIR = ../../../submodules/busybox/_install
DST_DIR = $(ROOTFS_DIR)/iob_timer
# path to ROOTFS_DST_DIR starting from ROOTFS_DIR
ROOTFS_DST_DIR = ../../../software/OS_build

all: compile-driver compile-userapp add-to-rootfs

compile-driver:
	make -C $(LINUX_KERNEL_DIR) $(CROSS_COMPILE_OPS) M=$(PWD) modules

compile-userapp:
	make -C ./user all

add-to-rootfs: $(DST_DIR)
	cp -r ./ $(DST_DIR)/
	cd $(ROOTFS_DIR) && \
	find -print0 | cpio -0oH newc | gzip -9 > $(ROOTFS_DST_DIR)/rootfs.cpio.gz

$(DST_DIR): $(ROOTFS_DIR)
	mkdir -p $(DST_DIR)

$(ROOTFS_DIR):
	make -C ../../../ build-rootfs

clean-userapp:
	make -C ./user clean

clean: clean-userapp
	make -C $(LINUX_KERNEL_DIR) M=$(PWD) clean
	@rm -rf $(DST_DIR)
