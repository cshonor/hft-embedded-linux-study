CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g -O0

all: demo01_sizeof demo03_subscript demo02_correct

demo01_sizeof: demo01_sizeof.c
demo03_subscript: demo03_subscript.c

demo02_correct: demo02_extern/arr_def.c demo02_extern/use_correct.c
	$(CC) $(CFLAGS) -o $@ demo02_extern/arr_def.c demo02_extern/use_correct.c

demo02_wrong: demo02_extern/arr_def.c demo02_extern/use_wrong.c
	$(CC) $(CFLAGS) -o $@ demo02_extern/arr_def.c demo02_extern/use_wrong.c

clean:
	rm -f demo01_sizeof demo03_subscript demo02_correct demo02_wrong

.PHONY: all clean demo02_wrong
