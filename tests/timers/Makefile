
APP = timers
CFLAGS += -O2 -D STDDEV
LDFLAGS += -lm

$(APP): timers.c
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)



.PHONY: clean
clean:
	rm *.o $(APP)

