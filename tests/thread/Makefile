
CFLAGS += -ggdb -O2
LDFLAGS += -pthread


APP = thread
OBJS = main.o thread_swap.o timer.o

ALL: $(APP)

$(APP): $(OBJS)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

thread_swap.o: thread_swap.S
	$(CC) -c $^ -o $@


.PHONY: clean
clean:
	rm *.o

