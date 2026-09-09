CC      ?= gcc
CFLAGS  ?= -O0 -g -Wall -Wno-unused -fno-pie -no-pie
# -fno-pie/-no-pie：让函数地址是固定绝对地址，T2/T5 的重定位公式更直观

T1 = t1_ksymtab
T2 = t2_reloc
T3 = t3_modinfo
T4 = t4_this_module
T5L = t5_elf_loader
T5M = t5_module.o

ALL = $(T1) $(T2) $(T3) $(T4) $(T5L)

all: $(ALL) $(T5M)

$(T1): t1_ksymtab.c ; $(CC) $(CFLAGS) -o $@ $<
$(T2): t2_reloc.c   ; $(CC) $(CFLAGS) -o $@ $<
$(T3): t3_modinfo.c ; $(CC) $(CFLAGS) -o $@ $<
$(T4): t4_this_module.c ; $(CC) $(CFLAGS) -o $@ $<
$(T5L): t5_elf_loader.c ; $(CC) $(CFLAGS) -o $@ $<
$(T5M): t5_module.c ; $(CC) -O0 -g -c -o $@ $<

clean:
	rm -f $(ALL) $(T5M)

.PHONY: all clean
