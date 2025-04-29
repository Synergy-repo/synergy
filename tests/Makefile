CFLAGS += -Wno-unused-parameter
CFLAGS +=-Wall -Wextra -O2 -DSTDDEV

LDFLAGS += -pthread -lm

PROGS= signal signal_thread signal_core signal_multthread fake_work
PROGS+= ipi_core ipi_multthread hrtimer tsc_hz
PROGS+= rocksdb leveldb
#PROGS+= dune_core

ROCKS_DIR=../deps/rocksdb
ROCKS_INCLUDE += $(ROCKS_DIR)/include
ROCKS_LDLIBS += $(ROCKS_DIR)/librocksdb.a -lpthread -lrt -ldl -lsnappy -lgflags -lz -lbz2 -llz4 -lzstd

LEVELDB_DIR=../deps/leveldb
LEVELDB_INCLUDE=$(LEVELDB_DIR)/include
LEVELDB_LIBS=$(LEVELDB_DIR)/build/libleveldb.a -lpthread -lsnappy

DUNE_DIR=../deps/dune
DUNE_INCLUDE= $(DUNE_DIR)
DUNE_LDLIBS= -static -L $(DUNE_DIR)/libdune -ldune

SHINJUKU_DUNE_DIR=../deps/shinjuku_dune
SHINJUKU_DUNE_INCLUDE= $(SHINJUKU_DUNE_DIR)
SHINJUKU_DUNE_LDLIBS= -static -L $(SHINJUKU_DUNE_DIR)/libdune -ldune

# concord
CONCORD_PASS=../deps/concord/src/cache-line-pass/build/src/libConcordPass.so
PASS_FLAGS=-c -emit-llvm
OPT_CONFIG=-postdomtree -mem2reg -indvars -loop-simplify -branch-prob -scalar-evolution

KMOD_INCLUDE=../kmod

INCLUDES = -I $(ROCKS_INCLUDE)
INCLUDES += -I $(DUNE_INCLUDE)
INCLUDES += -I $(KMOD_INCLUDE)
INCLUDES += -I $(LEVELDB_INCLUDE)

CC = gcc
CXX = g++

.PHONY: all clean

ALL: $(PROGS)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS) -lm

fake_work: fake_work.o
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS) -lm

overhead_instrumentation: instrumentation_main.o instrumentation_loop.o
	clang-9 $^ -o $@ $(CFLAGS) $(LDFLAGS) -lm

instrumentation_loop.o: instrumentation_loop.c
# generate LLVM IR files from C source files
	clang -DCONCORD $(CFLAGS) $(PASS_FLAGS) $< -o $<.bc
# run default LLVM passes
	opt-9 $(OPT_CONFIG) < $<.bc > $<.opt1.bc
# run concord pass
	opt-9 -load $(CONCORD_PASS) -yield < $<.opt1.bc > $<.opt2.bc
# generate obj from passes output
	llc-9 -filetype=obj $<.opt2.bc -o $@

instrumentation_main.o: instrumentation_main.c
	clang-9 -c $^ -o $@ $(CFLAGS) $(INCLUDES)

fake_work_concord: fake_work_concord.o
	clang-9 $^ -o $@ $(CFLAGS) $(LDFLAGS) -lm

fake_work_concord.o: fake_work.c
# generate LLVM IR files from C source files
	clang -DCONCORD $(CFLAGS) $(PASS_FLAGS) $< -o $<.bc
# run default LLVM passes
	opt-9 $(OPT_CONFIG) < $<.bc > $<.opt1.bc
# run concord pass
	opt-9 -load $(CONCORD_PASS) -yield < $<.opt1.bc > $<.opt2.bc
# generate obj from passes output
	llc-9 -filetype=obj $<.opt2.bc -o $@

leveldb: leveldb.o
	$(CXX) $^ -o $@ $(LEVELDB_LIBS) $(CFLAGS)

rocksdb: rocksdb.o
	$(CXX) $^ -o $@ $(ROCKS_LDLIBS) $(CFLAGS)

ipi_core: ipi_core.o ../kmod/ipi.o ../kmod/ipi_trap.o
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

uintr_core: uintr_core.c ../kmod/uintr.o ../kmod/uintr_trap.o
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

ipi_multthread: ipi_multthread.o trap.o ../kmod/trap_lib.o
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

hrtimer: hrtimer.o trap.o
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

signal_core: signal_core.o
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

default_uintr: default_uintr.o
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

default_uintr.o: default_uintr.c
	$(CC) -c $^ -o $@ $(CFLAGS) $(LDFLAGS) -muintr

dune_core: dune_core.o
	$(CC) $^ -o $@ $(CFLAGS) $(DUNE_LDLIBS) $(LDFLAGS)

shinjuku_dune_core: shinjuku_dune_core.o
	$(CC) $^ -o $@ $(CFLAGS) $(SHINJUKU_DUNE_LDLIBS) $(LDFLAGS)

shinjuku_dune_core.o: shinjuku_dune_core.c
	$(CC) -c $^ -o $@ $(CFLAGS) -I $(SHINJUKU_DUNE_INCLUDE)

%trap.o: %trap.S
	$(CC) -c $^ -o $@

%.o: %.c
	$(CC) -c $^ -o $@ $(CFLAGS) $(INCLUDES)

clean:
	@ find . -type f -name '*.o' -delete
	@ find . -type f -name '*.bc' -delete
	@rm $(PROGS)
