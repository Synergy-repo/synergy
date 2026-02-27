
ROOT_PATH=../..
include ../shared.mk

APP := fake-app

SRCS = main.c
OBJS = $(SRCS:.c=.o)

ifeq ($(INTERRUPT_METHOD),CONCORD)
FAKE_LIB = $(ROOT_PATH)/deps/fake-work/libfakework_concord.a
else
FAKE_LIB = $(ROOT_PATH)/deps/fake-work/libfakework.a
endif

LDLIBS += $(FAKE_LIB)

FAKE_INC += -I$(ROOT_PATH)/deps/fake-work/

INC += $(FAKE_INC)

.PHONY: all
all: $(APP)

$(APP): $(OBJS) $(synergy_DEPS)
	$(CC) $(OBJS) $(CFLAGS) -o $@ -I$(INC) $(LDLIBS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -I$(INC) -c $^ -o $@

.PHONY: run
run:
	sudo ./$(APP) -n 2 -l $(CPUS) -a $(NIC_PCI)

.PHONY: clean
clean:
	@rm $(APP) *.o
