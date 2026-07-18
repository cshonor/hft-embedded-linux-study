CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=gnu11 -g -O0 -I.
RTOS_OBJ = rtos/sched.o rtos/sem.o rtos/queue.o

all: demo01_foreground demo02_tcb_coop demo03_preempt demo04_semaphore \
     demo05_queue demo06_mini_rtos

rtos/%.o: rtos/%.c
	$(CC) $(CFLAGS) -c $< -o $@

demo01_foreground: demo01_foreground.c
	$(CC) $(CFLAGS) -o $@ $<

demo04_semaphore: demo04_semaphore.c
	$(CC) $(CFLAGS) -DNO_SCHED -o $@ demo04_semaphore.c rtos/sem.c

demo02_tcb_coop demo03_preempt demo05_queue demo06_mini_rtos: $(RTOS_OBJ)
	$(CC) $(CFLAGS) -o $@ $(filter demo%.c,$^) $^ 2>/dev/null || true

demo02_tcb_coop: demo02_tcb_coop.c $(RTOS_OBJ)
	$(CC) $(CFLAGS) -o $@ demo02_tcb_coop.c $(RTOS_OBJ)

demo03_preempt: demo03_preempt.c $(RTOS_OBJ)
	$(CC) $(CFLAGS) -o $@ demo03_preempt.c $(RTOS_OBJ)

demo05_queue: demo05_queue.c $(RTOS_OBJ)
	$(CC) $(CFLAGS) -o $@ demo05_queue.c $(RTOS_OBJ)

demo06_mini_rtos: demo06_mini_rtos.c $(RTOS_OBJ)
	$(CC) $(CFLAGS) -o $@ demo06_mini_rtos.c $(RTOS_OBJ)

clean:
	rm -f demo0* rtos/*.o

.PHONY: all clean
