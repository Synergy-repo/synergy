
obj-m := kmod_ipi.o kmod_uintr.o
ccflags-y := -O2 -std=gnu11 -Wno-declaration-after-statement

PWD := $(CURDIR)

.PHONY: all tests clean

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

test_kmod_ipi: test_kmod_ipi.o ipi.o ipi_trap.o
	$(CC) -o $@ $^ -pthread -O2 -Wall -Wextra

test_kmod_uintr: test_kmod_uintr.o uintr.o uintr_trap.o
	$(CC) -o $@ $^ -pthread -O2 -Wall -Wextra -lm

%trap.o: %trap.S
	$(CC) -O2 -c $^ -o $@

%.o: %.c
	$(CC) -O2 -c $^ -o $@

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	find ./ -iname *.o -delete
	find ./ -type f -executable ! -name "*.sh" -delete
