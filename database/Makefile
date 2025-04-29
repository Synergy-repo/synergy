# Makefile for rocksdb
# https://github.com/facebook/rocksdb/blob/main/INSTALL.md
# https://github.com/facebook/rocksdb/blob/main/examples/Makefile

CFLAGS += -Wall -Wextra -O2 -Wno-unused-variable -Wno-unused-parameter

C_SRCS = $(wildcard ./*.c)
H_SRCS = $(wildcard ./*.h)
OBJS = $(C_SRCS:.c=.o)

INCLUDE += -I../deps/rocksdb/include -I../deps/leveldb/include
ROCKS_LDLIBS = ../deps/rocksdb/librocksdb.a -lpthread -lrt -ldl -lsnappy -lgflags -lz -lbz2 -llz4 -lzstd

LEVEL_LDLIBS = ../deps/leveldb/build/libleveldb.a -lpthread -lsnappy

rocksdb_createdb: rocksdb_createdb.o
	$(CXX) -o $@ $^ $(ROCKS_LDLIBS)

leveldb_createdb: leveldb_createdb.o
	$(CXX) -o $@ $^ $(LEVEL_LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE)

.PHONY: clean
clean:
	rm -f *.o $(APP)
