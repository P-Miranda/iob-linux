obj-m += hello.o

LINUX_KERNEL_DIR = ../../../submodules/linux-5.15.98

# Options to cross compile for RISCV system in a x86 host machine
CROSS_COMPILE_OPS = "ARCH=riscv"
CROSS_COMPILE_OPS += "CROSS_COMPILE=riscv64-unknown-linux-gnu-"

all:
	make -C $(LINUX_KERNEL_DIR) $(CROSS_COMPILE_OPS) M=$(PWD) modules

clean:
	make -C $(LINUX_KERNEL_DIR) M=$(PWD) clean
