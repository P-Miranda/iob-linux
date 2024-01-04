# Linux Drivers
This folder contains Linux Drivers.

## Linux Kernel Module flow
The linux drivers can be compiled as kernel modules in any system targetting
any other system.
Use the correct `ARCH` and `CROSS_COMPILE` variables (see
[here](01_hello_world/Makefile) for an example).

To compile a driver run:
```bash
# generic command
make -C path/to/driver/
# example
make -C software/linux_drivers/01_hello_world
```
This target performs:
1. linux driver cross compilation for riscv architecture
    - **NOTE**: correct cross compilation requires correct compiled kernel.
        - QEMU uses linux kernel compiled with `rv32_defconfig`
        - IOb-SOC uses linux kernel compiled with `iob_soc_defconfig`
2. copy kernel object `.ko` to rootfs
    - tipically, the uncompressed rootfs is located in `busybox/_install`
3. re-compress rootfs for embedded/qemu usage

### QEMU Execution
Run QEMU with target:
```bash
make run-qemu
```
Manually insert driver with:
```bash
cd path/to/driver
insmod [driver.ko]
# example
cd hello/
insmod hello.ko
```
Manually remove driver with:
```bash
rmmod driver
# example
rmmod hello
```
