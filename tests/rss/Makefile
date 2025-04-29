# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2010-2014 Intel Corporation

# binary name
APP = echo

# all source are stored in SRCS-y
SRCS-y := main.c dpdk.c stat.c

PKGCONF ?= pkg-config

PKG_PATH = ../../deps/dpdk/build/lib/x86_64-linux-gnu/pkgconfig/

ifneq ($(shell PKG_CONFIG_PATH=$(PKG_PATH) $(PKGCONF) --exists libdpdk && echo 0),0)
$(error "no installation of DPDK found")
endif

all: static
.PHONY: shared static
shared: build/$(APP)-shared
	ln -sf $(APP)-shared build/$(APP)
static: build/$(APP)-static
	ln -sf $(APP)-static build/$(APP)

PC_FILE := $(shell PKG_CONFIG_PATH=$(PKG_PATH) $(PKGCONF) --path libdpdk 2>/dev/null)
LDFLAGS_SHARED = $(shell PKG_CONFIG_PATH=$(PKG_PATH) $(PKGCONF) --libs libdpdk)
CFLAGS += -O3 -mtune=intel -Wall -Wextra
CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_PATH) $(PKGCONF) --cflags libdpdk)
LDFLAGS_STATIC = $(shell PKG_CONFIG_PATH=$(PKG_PATH) $(PKGCONF) --libs --static libdpdk)

ifeq ($(MAKECMDGOALS),static)
# check for broken pkg-config
ifeq ($(shell echo $(LDFLAGS_STATIC) | grep 'whole-archive.*l:lib.*no-whole-archive'),)
$(warning "pkg-config output list does not contain drivers between 'whole-archive'/'no-whole-archive' flags.")
$(error "Cannot generate statically-linked binaries with this version of pkg-config")
endif
endif

#CFLAGS += -DALLOW_EXPERIMENTAL_API

build/$(APP)-shared: $(SRCS-y) Makefile $(PC_FILE) | build
	$(CC) $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)

build/$(APP)-static: $(SRCS-y) Makefile $(PC_FILE) | build
	$(CC) $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS) $(LDFLAGS_STATIC)

build:
	@mkdir -p $@

.PHONY: clean
clean:
	rm -f build/$(APP) build/$(APP)-static build/$(APP)-shared
	test -d build && rmdir -p build || true
