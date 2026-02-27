
ROOT_PATH=../..
include ../shared.mk

APP ?= leveldb-app

SRCS = main.c
OBJS = $(SRCS:.c=.o)


LEVEL_INC = $(ROOT_PATH)/deps/leveldb/include
ifeq ($(INTERRUPT_METHOD),CONCORD)
LEVEL_LIB = $(ROOT_PATH)/deps/concord_leveldb/concord_libleveldb.a
else
LEVEL_LIB = $(ROOT_PATH)/deps/leveldb/build/libleveldb.a
endif

LDLIBS += $(LEVEL_LIB)
LDLIBS += -pthread -lsnappy

.PHONY: all
all: $(APP)

$(APP): $(OBJS) $(synergy_DEPS) $(LEVEL_LIB)
	$(CXX) $(CFLAGS) $(OBJS) -o $@  $(LDLIBS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@ -I$(INC) -I$(LEVEL_INC)

.PHONY: run
run:
	sudo ./$(APP) -n 2 -l $(CPUS) -a $(NIC_PCI)

.PHONY: clean
clean:
	rm $(APP) *.o *.so $(ALLOCATOR)_test

