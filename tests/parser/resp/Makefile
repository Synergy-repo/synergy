
APP=resp
CFLAGS += -Wall -Wextra -O2 -DNDEBUG -DSTDDEV
LIBS += -lm

SRC = main.c \
			resp.c

all: $(SRC)
	$(CC) -o $(APP) $(CFLAGS) $(SRC) $(LIBS)

.PHONY: clean
clean:
	rm $(APP)
