include colors.mk

SHELL = /bin/bash
CC = gcc
CFLAGS = -Wall -Wpedantic
CFLAGS += -std=gnu99 -pthread -Itest -Isrc
CFLAGS += -DTEST
CROSS_COMPILE ?= riscv32-unknown-elf-
OUTDIR ?= build
RISCV32_CC = $(CROSS_COMPILE)$(CC)
COMPILE_RISCV32 =

.PHONY : all setup_riscv build_riscv build_x86 build_dir clean

# add directories into source search path
VPATH = src:test

all: build_riscv

build_riscv:
	$(MAKE) COMPILE_RISCV32=yes build

build_x86:
	$(MAKE) COMPILE_RISCV32=no build

build: spacewire.o

%.o: %.c | build_dir
ifeq ($(COMPILE_RISCV32),yes)
	@echo "${GREEN}compiling for RISCV${RESET}"
	$(RISCV32_CC) -c $(CFLAGS) $< -o $(OUTDIR)/$@
else
	@echo "${GREEN}compiling for native system (most likely x86)${RESET}"
	$(CC) -c $(CFLAGS) $< -o $(OUTDIR)/$@
endif

all_tests: test_1 test_2

test_1: build_x86
	$(CC) $(CFLAGS) test/test.c -o $(OUTDIR)/$@ $(OUTDIR)/spacewire.o
	@echo "${GREEN}running tests${RESET}"
	$(OUTDIR)/$@

test_2: build_x86
	$(CC) $(CFLAGS) test/main_x86.c -o $(OUTDIR)/$@
	@echo "${GREEN}running tests${RESET}"
	$(OUTDIR)/$@

build_dir:
	@echo "${GREEN}BUILDDIR${RESET}"
	mkdir -p $(OUTDIR)

clean:
	rm -rf $(OUTDIR)
