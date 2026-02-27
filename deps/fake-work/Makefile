
ROOT_PATH=../..
include $(ROOT_PATH)/synergy_CONFIG

ifeq ($(ENABLE_DEBUG),y)
CFLAGS += $(CFLAGS_DEBUG)
else
CFLAGS += $(CFLAGS_RELEASE)
endif

LIBFAKEWORK=libfakework.a

ifeq ($(INTERRUPT_METHOD),CONCORD)
CC=clang-9
include CONCORD_FLAGS
LIBFAKEWORK+=libfakework_concord.a libfakework_concord_rdtsc.a
endif

ifeq ($(INTERRUPT_METHOD),CONCORD)
all: | calibrate \
	$(LIBFAKEWORK) \
	test concord_test concord_rdtsc_test \
	run_test
else
all: | calibrate $(LIBFAKEWORK) test run_test
endif

calibrate: calibrate.o calibrete_fake_work.o
	$(CC) $(CFLAGS) $^ -o $@ -pthread -march=native

calibrete_fake_work.o: fake_work.c
	$(CC) $(CFLAGS) -c -DCALIBRATE $^ -o $@


concord_calibrate: calibrate.o calibrate_fake_work_concord.o
	$(CC) $(CFLAGS) $^ -o $@ -pthread -march=native

calibrate_fake_work_concord.o: fake_work.c
# generate LLVM IR files from C source files
	$(CC) $(CFLAGS) $(PASS_FLAGS) -DCALIBRATE $< -o $<.bc
# run default LLVM passes
	$(OPT) $(OPT_CONFIG) < $<.bc > $<.opt1.bc
# run concord pass
	$(OPT) -load $(CONCORD_PASS) -yield < $<.opt1.bc > $<.opt2.bc
# generate obj from passes output
	$(LLC) -filetype=obj $<.opt2.bc -o $@


concord_rdtsc_calibrate: calibrate.o calibrate_fake_work_concord_rdtsc.o
	$(CC) $(CFLAGS) $^ -o $@ -pthread -march=native

calibrate_fake_work_concord_rdtsc.o: fake_work.c
# generate LLVM IR files from C source files
	$(CC) $(CFLAGS) $(PASS_FLAGS) -DCALIBRATE $< -o $<.bc
# run default LLVM passes
	$(OPT) $(OPT_CONFIG) < $<.bc > $<.opt1.bc
# run concord pass
	$(OPT) -load $(CONCORD_RDTSC_PASS) -yield < $<.opt1.bc > $<.opt2.bc
# generate obj from passes output
	$(LLC) -filetype=obj $<.opt2.bc -o $@

######################

libfakework.a: fake_work.o
	ar rcs $@ $^

libfakework_concord.a: fake_work_concord.o
	ar rcs $@ $^

libfakework_concord_rdtsc.a: fake_work_concord_rdtsc.o
	ar rcs $@ $^

# Rule to run LLVM passes on each C source file and generate obj file
fake_work_concord.o: fake_work.c
# generate LLVM IR files from C source files
#	$(CC) $(CFLAGS) $(PASS_FLAGS) $(shell ./concord_calibrate) $< -o $<.bc
	$(CC) $(CFLAGS) $(PASS_FLAGS) $(ITERATIONS) $< -o $<.bc
# run default LLVM passes
	$(OPT) $(OPT_CONFIG) < $<.bc > $<.opt1.bc
# run concord pass
	$(OPT) -load $(CONCORD_PASS) -yield < $<.opt1.bc > $<.opt2.bc
# generate obj from passes output
	$(LLC) -filetype=obj $<.opt2.bc -o $@

# Rule to run LLVM passes on each C source file and generate obj file
fake_work_concord_rdtsc.o: fake_work.c
# generate LLVM IR files from C source files
#	$(CC) $(CFLAGS) $(PASS_FLAGS) -DCONCORD_RDTSC $(shell ./concord_rdtsc_calibrate) $< -o $<.bc
	$(CC) $(CFLAGS) $(PASS_FLAGS) -DCONCORD_RDTSC $(ITERATIONS) $< -o $<.bc
# run default LLVM passes
	$(OPT) $(OPT_CONFIG) < $<.bc > $<.opt1.bc
# run concord pass
	$(OPT) -load $(CONCORD_RDTSC_PASS) -yield < $<.opt1.bc > $<.opt2.bc
# generate obj from passes output
	$(LLC) -filetype=obj $<.opt2.bc -o $@

fake_work.o: fake_work.c
	$(eval ITERATIONS=$(shell ./calibrate))
#	$(CC) $(CFLAGS) -c $(shell ./calibrate) $^ -o $@
	$(CC) $(CFLAGS) -c $(ITERATIONS) $^ -o $@

test: test_fake_work.c
	$(CC) $(CFLAGS) $^ -o $@ -pthread ./libfakework.a

concord_test: test_fake_work.c
	$(CC) $(CFLAGS) $^ -o $@ -pthread ./libfakework_concord.a

concord_rdtsc_test: test_fake_work.c
	$(CC) $(CFLAGS) -DCONCORD_RDTSC $^ -o $@ -pthread ./libfakework_concord_rdtsc.a

hello_rdtsc: hello_rdtsc.c
	$(CC) $(CFLAGS) $^ -o $@ ./libfakework_concord_rdtsc

.PHONY: run_test
run_test:
	./test
	./concord_test
	./concord_rdtsc_test

.PHONY: clean
clean:
	rm calibrate *.o *.a test *bc

