CC ?= cc
CFLAGS ?= -Wall -Wextra -O2 -g
PROG := mytimeout
SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)

all: $(PROG)
$(PROG): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(PROG) src/*.o
.PHONY: all clean
