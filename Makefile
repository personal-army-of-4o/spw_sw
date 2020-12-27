CC = gcc
CFLAGS = -Wall -Wpedantic
CROSS_COMPILE ?= riscv32-unknown-elf-
RISCV32_CC = $(CROSS_COMPILE)$(CC)

.PHONY : all cross_compile build_x86 test build_dir clean

all: cross_compile

cross_compile: src/spacewire.c | build_dir
	$(RISCV32_CC) $(CFLAGS) $< -o build/spacewire.o -c

build_x86: src/spacewire.c | build_dir
	$(CC) $(CFLAGS) $< -o build/spacewire.o -c

test: build_x86 | build_dir
	$(CC) $(CFLAGS) test/test.c -o build/$@ build/spacewire.o
	./build/test

build_dir:
	mkdir -p build

clean:
	rm -rf build/
