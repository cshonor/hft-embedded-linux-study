# ch10 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/05-Embedded-C-Self-Cultivation/ch10-multitasking-and-os/demo
make all
./demo01_foreground
./demo02_tcb_coop
./demo03_preempt
./demo04_semaphore
./demo05_queue
./demo06_mini_rtos
```

## rtos/ 模块化组件（衔接 ch09）

```
rtos/
  tcb.h sched.c   TCB + 协作式 setjmp 切换
  sem.h  sem.c    二值/计数信号量
  queue.h queue.c 环形消息队列
arm/
  context_switch.S  ARM PendSV 骨架（参考）
```

## demo02/03 说明

主机上用 **setjmp/longjmp** 模拟上下文切换，观察独立任务栈与优先级调度。  
真机抢占需 **SysTick/PendSV + `stmfd`/`ldmfd`**（见 `arm/context_switch.S`、ch03）。

## QEMU（可选，需 ch03 交叉工具链）

```bash
# 将 demo 移植到裸机后：
qemu-system-arm -M mps2-an385 -kernel demo.elf
gdb-multiarch demo.elf
```

## GDB

```bash
gdb ./demo02_tcb_coop
break task_yield
run
info registers sp
```
