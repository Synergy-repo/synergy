
include ../synergy_CONFIG

SRCS = $(wildcard *.c) $(wildcard *.cc)
OBJS = $(SRCS:.cc=.o)
OBJS := $(OBJS:.c=.o)

ifeq ($(ENABLE_DEBUG),y)
CFLAGS += $(CFLAGS_DEBUG)
else
CFLAGS += $(CFLAGS_RELEASE)
endif

CFLAGS+= -D_GNU_SOURCE

all: libshim.a

libshim.a: $(OBJS)
	$(AR) rcs $@ $^

%.o: %.cc
	$(CC) $(CFLAGS) -c $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: clean
clean:
	rm *.o *.a

print-%  : ; @echo $* = $($*)
