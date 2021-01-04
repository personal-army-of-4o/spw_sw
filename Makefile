include colors.mk

SHELL = /bin/bash
CC = gcc
CFLAGS = -Wall -Wpedantic
CFLAGS += -std=gnu99 -Itest -Isrc
CFLAGS += -DTEST
#this gets rid of annoying debug __VA_ARGS warnings
CFLAGS += -Wno-gnu-zero-variadic-macro-arguments

# log everything but DEBUG by default. set with `make LOGLEVEL=N` where N is 0 to 4
LOGLEVEL ?= 3
CFLAGS += -DLOGLEVEL=$(LOGLEVEL)

# enable NDEBUG flag by default. run with `make NDEBUG=0` to disable
NDEBUG ?= 1
ifeq ($(NDEBUG), 1)
	CFLAGS += -DNDEBUG
endif

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

test_all: test_1 test_2

test_1: build_x86
	$(CC) $(CFLAGS) test/test.c -o $(OUTDIR)/$@ $(OUTDIR)/spacewire.o
	@echo "${GREEN}running tests${RESET}"
	$(OUTDIR)/$@

test_2: build_x86
	$(CC) $(CFLAGS) test/main_x86.c -o $(OUTDIR)/$@ -pthread
	@echo "${GREEN}running tests${RESET}"
	$(OUTDIR)/$@

build_dir:
	@echo "${GREEN}BUILDDIR${RESET}"
	mkdir -p $(OUTDIR)

clean:
	rm -rf $(OUTDIR)
