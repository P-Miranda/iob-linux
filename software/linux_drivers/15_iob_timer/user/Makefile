SRC = $(wildcard *.c)
# INCLUDE += -I../../../submodules/linux-5.15.98/include
INCLUDE += -I../
FLAGS = -Wall -Werror -O2
FLAGS += -static
FLAGS += -march=rv32imac
FLAGS += -mabi=ilp32
BIN = user
CC = riscv64-unknown-linux-gnu-gcc
export ARCH=riscv

all: $(SRC)
	$(CC) $(FLAGS) $(INCLUDE) -o $(BIN) $(SRC)

clean:
	rm -rf $(BIN)
