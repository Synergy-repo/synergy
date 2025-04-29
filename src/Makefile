
include ../synergy_CONFIG

LIB_DIR = lib
LIB_NAME = libsynergy.a

C_SRCS = $(wildcard ./*.c)
H_SRCS = $(wildcard ./*.h)

OBJS = $(C_SRCS:.c=.o) thread_switch.o

ifeq ($(INTERRUPT_METHOD),KMOD_IPI)
OBJS += ../kmod/ipi.o \
				../kmod/ipi_trap.o
endif

ifeq ($(INTERRUPT_METHOD),UINTR)
OBJS +=	../kmod/uintr.o \
				../kmod/uintr_trap.o
endif

PKGCONF ?= pkg-config

PKG_PATH = ../deps/dpdk/build/lib/x86_64-linux-gnu/pkgconfig/

ifneq ($(shell PKG_CONFIG_PATH=$(PKG_PATH) $(PKGCONF) --exists libdpdk && echo 0),0)
$(error "no installation of DPDK found")
endif

ifeq ($(ENABLE_DEBUG),y)
CFLAGS += $(CFLAGS_DEBUG)
else
CFLAGS += $(CFLAGS_RELEASE) -DNDEBUG2
endif

CFLAGS += -D$(INTERRUPT_METHOD)

ifdef QUANTUM
CFLAGS += -DQUANTUM=$(QUANTUM)
endif

ifdef QUANTUM_FACTOR
CFLAGS += -DQUANTUM_FACTOR=$(QUANTUM_FACTOR)
endif

ifdef CONGESTED_QUANTUM_FACTOR
CFLAGS += -DCONGESTED_QUANTUM_FACTOR=$(CONGESTED_QUANTUM_FACTOR)
endif

ifdef CONGESTED_THRESH
CFLAGS += -DCONGESTED_THRESH=$(CONGESTED_THRESH)
endif

ifeq ($(ENABLE_CFCFS),y)
CFLAGS += -DENABLE_CFCFS
endif

ifeq ($(ENABLE_WORKSTEALING),y)
CFLAGS += -DENABLE_WORKSTEALING
endif

ifeq ($(ENABLE_WORKSTEALING_BW),y)
CFLAGS += -DENABLE_WORKSTEALING_BW
endif

ifeq ($(ENABLE_INTERRUPT),y)
CFLAGS += -DENABLE_INTERRUPT
endif

ifeq ($(ENABLE_QUANTUM_AUTOMATIC),y)
CFLAGS += -DQUANTUM_AUTOMATIC
endif

ifeq ($(ENABLE_TIMER_WORKER),y)
CFLAGS += -DENABLE_TIMER_WORKER
endif

ifeq ($(ENABLE_WAIT_QUEUE),y)
CFLAGS += -DENABLE_WAIT_QUEUE
endif

ifeq ($(ENABLE_CONDITIONAL_INTERRUPT),y)
CFLAGS += -DENABLE_CONDITIONAL_INTERRUPT
endif

ifeq ($(ENABLE_FEEDBACK),y)
CFLAGS += -DENABLE_FEEDBACK
endif

CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_PATH) $(PKGCONF) --cflags libdpdk)

all: $(LIB_DIR)/$(LIB_NAME)

# create static synergy library
$(LIB_DIR)/$(LIB_NAME): $(OBJS) $(H_SRCS) | lib_dir
	ar rcs $@ $(OBJS)

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

lib_dir:
	@mkdir -p $(LIB_DIR)

.PHONY: clean
clean:
	@rm -f $(LIB_DIR)/*
	@rm *.o
