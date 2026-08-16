# ch10 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch10-multitasking-and-os/demo
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

---

## 代码自测

**题目 1：** 本章 demo 目录的实践代码涉及哪些知识点？如何编译运行？
```c
// 典型 demo 编译流程
cd demo/
make all    # 编译
./app       # 运行
make clean  # 清理
```
<details>
<summary>参考答案</summary>

嵌入式 C 自我修养这本书是标准 C 到内核开发的桥梁。内核代码大量使用 __attribute__/typeof/container_of/section/weak 等 GNU C 扩展，侵入式链表和函数指针实现的 OOP，以及头文件/源文件分离的模块化设计——这些标准 C 教材不讲，本书系统讲解。

学习路线：ch01 工具链 -> ch02 计算机架构 -> ch03 ARM 汇编 -> ch04 编译链接 -> ch05 内存栈管理 -> ch06 GNU C 扩展 -> ch07 数据存储与指针 -> ch08 OOP -> ch09 模块化编程 -> ch10 多任务与 OS。

</details>
