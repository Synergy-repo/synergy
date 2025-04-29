
# Common Makefile to compile apps

ifndef ROOT_PATH
$(error ROOT_PATH is not set)
endif

include $(ROOT_PATH)/synergy_CONFIG
include $(ROOT_PATH)/MACHINE_CONFIG

PKG_PATH = $(ROOT_PATH)/deps/dpdk/build/lib/x86_64-linux-gnu/pkgconfig/
PKGCONF = pkg-config

INC = $(ROOT_PATH)/src

LIBsynergy = $(ROOT_PATH)/src/lib/libsynergy.a
LIBSHIM = $(ROOT_PATH)/shim/libshim.a

# order matters
synergy_DEPS = $(LIBSHIM) $(LIBsynergy)
DPDK_LIBS = $(shell PKG_CONFIG_PATH=$(PKG_PATH) $(PKGCONF) --libs --static libdpdk)
LDLIBS +=  $(DPDK_LIBS) $(synergy_DEPS)


ifeq ($(ENABLE_DEBUG),y)
CFLAGS += $(CFLAGS_DEBUG)
else
CFLAGS += $(CFLAGS_RELEASE)
endif

# ensure synergy lib is updated before compiling apps
.PHONY: pre-build
pre-build: check_synergy_lib all

.PHONY: check_synergy_lib
check_synergy_lib:
	@echo "Checking if synergy library is updated."
	@$(MAKE) synergy -C $(ROOT_PATH)
