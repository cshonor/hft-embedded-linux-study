CC      ?= gcc
CFLAGS  := -O2 -Wall -Wno-unused -fno-pie -no-pie
CFLAGS0 := -O0 -g -Wall -Wno-unused -fno-pie -no-pie

TARGETS := t1_layout t2_bitfield t3_unaligned t4_flagword t5_bswap

all: $(TARGETS)

t1_layout: t1_layout.c
	$(CC) $(CFLAGS0) -o $@ $<
t2_bitfield: t2_bitfield.c
	$(CC) $(CFLAGS0) -o $@ $<
t3_unaligned: t3_unaligned.c
	$(CC) $(CFLAGS) -o $@ $<
t4_flagword: t4_flagword.c
	$(CC) $(CFLAGS0) -fno-strict-aliasing -o $@ $<
t5_bswap: t5_bswap.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean
