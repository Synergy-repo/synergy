
ROOT_PATH=../..
include ../shared.mk

APP ?= rocksdb-app

SRCS = main.c
OBJS = $(SRCS:.c=.o)

ROCKS_LIB = $(ROOT_PATH)/deps/rocksdb/librocksdb.a
ROCKS_DEPEND = -lsnappy -lz -lbz2 -llz4 -lzstd
ROCKS_INC = $(ROOT_PATH)/deps/rocksdb/include

LDLIBS += $(ROCKS_LIB) $(ROCKS_DEPEND)

.PHONY: all
all: $(APP)

$(APP): $(OBJS) $(synergy_DEPS) $(ROCKS_LIB)
	$(CXX) $(CFLAGS) $(OBJS) -o $@ $(LDLIBS) $(LDFLAGS)

%.o: %.c
	$(CXX) -c $^ -o $@ $(CFLAGS) -I$(INC) -I$(ROCKS_INC)

test: test.c
	$(CXX) $^ -o $@ $(CFLAGS) -g -I$(ROCKS_INC) $(ROCKS_LIB) $(ROCKS_DEPEND)

.PHONY: run
run:
	sudo ./$(APP) -n 2 -l $(CPUS) -a $(NIC_PCI)

.PHONY: clean
clean:
	rm $(APP) *.o test

