CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
LIB_SRCS = mm.c memlib.c
LIB_OBJS = $(LIB_SRCS:.c=.o)
TEST = test/smoke

.PHONY: all clean test

all: $(LIB_OBJS) $(TEST)

$(TEST): test/smoke.c $(LIB_OBJS)
	$(CC) $(CFLAGS) -I. -o $@ test/smoke.c $(LIB_OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(TEST)
	./$(TEST)

clean:
	rm -f $(LIB_OBJS) $(TEST)
