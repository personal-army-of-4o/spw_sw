CC = gcc
CFLAGS = -Wall -Wpedantic
CROSS_COMPILE ?= riscv32-unknown-elf-
RISCV32_CC = $(CROSS_COMPILE)$(CC)

.PHONY : all build_riscv build_x86 test build_dir clean

all: build_riscv

build_riscv: src/spacewire.c | build_dir
	$(RISCV32_CC) $(CFLAGS) $< -o build/spacewire.o -c

build_x86: src/spacewire.c | build_dir
	$(CC) $(CFLAGS) $< -o build/spacewire.o -c

test: build_x86
	$(CC) $(CFLAGS) test/test.c -o build/$@ build/spacewire.o
	./build/test

build_dir:
	mkdir -p build

clean:
	rm -rf build/
