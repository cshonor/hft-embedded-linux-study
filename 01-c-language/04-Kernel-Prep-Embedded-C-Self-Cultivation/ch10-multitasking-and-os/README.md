# 第 10 章 C 语言的多任务编程思想和操作系统入门

**Multitasking, RTOS & OS Fundamentals for Embedded C**

## 本章目标

作为 **《嵌入式 C 内功修炼》终章 capstone**，建立从 **裸机前后台** → **协作/抢占式 RTOS** → **Linux 进程/线程/MMU/syscall** 的完整图景。能解释 **并发 vs 并行**、实现 **TCB + 独立栈 + 上下文切换**（衔接 **ch03/ch05/ch08**），使用 **mutex/semaphore/queue** 解决同步，按 **ch09 模块化** 搭建 **迷你 RTOS（demo06）**；并理解 **中断、存储映射、寄存器、文件系统** 等与 OS 相关的硬件与 Linux 用户态接口，完成全书知识闭环。

## 前置依赖

| 章节 | 内容 |
|------|------|
| **[ch03](../ch03-arm-architecture-and-assembly/)** | ARM 汇编、`stmfd`/`ldmfd`、栈 |
| **[ch05](../ch05-memory-stack-management/)** | 任务栈、栈溢出、canary |
| **[ch08](../ch08-oop-in-c/)** | TCB 结构体、ops/vtable |
| **[ch09](../ch09-modular-programming-in-c/)** | 分层目录、Makefile、回调解耦 |

## 环境

- **主机编译**：GCC 或 Clang，**`-std=gnu11 -Wall -Wextra`**
- **可选 ARM**：`arm-none-eabi-gcc`、`qemu-system-arm`（**demo03_preempt** ARM 路径、**10.6.3**）
- **Linux 实验**：原生 Linux 或 WSL，**`strace`**、**`/proc`**（**10.4.4**、**10.9.1**）
- **构建**：`make`（**demo/** 由配套工程提供，见 Demo 清单）
- **参考 RTOS 文档**：FreeRTOS、uC/OS-III 官方手册（Hook、FromISR API）

## 快速操作 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/04-Kernel-Prep-Embedded-C-Self-Cultivation/ch10-multitasking-and-os/demo

make all

./demo01_foreground
./demo02_tcb_coop
./demo03_preempt
./demo04_semaphore
./demo05_queue
./demo06_mini_rtos

# 可选 ARM 参考：demo/arm/context_switch.S + ch03 交叉工具链 + qemu

make clean
```

## 八大知识模块

| 模块 | 目录 | 核心 |
|------|------|------|
| **1 裸机多任务** | **10.1**、**10.1.1–10.1.3** | 并发/并行、前后台、TCB、时间片 |
| **2 OS/RTOS 原理** | **10.2**、**10.2.1–10.2.4** | 调度、栈、可重入、mutex/sem/queue |
| **3 中断** | **10.3**、**10.3.1–10.3.3** | SysTick/PendSV、双栈、ISR 规范 |
| **4 系统调用** | **10.4**、**10.4.1–10.4.4** | API vs syscall、特权级、strace |
| **5 文件系统** | **10.5**、**10.5.1–10.5.3** | VFS、mount、rootfs/initramfs |
| **6 存储映射** | **10.6**、**10.6.1–10.6.3** | 总线、链接脚本、启动链 |
| **7 I/O 与寄存器** | **10.7**、**10.7.1–10.7.3**、**10.8**、**10.8.1–10.8.3** | MMIO、驱动分层、位操作 |
| **8 MMU 与进程模型** | **10.9**、**10.9.1–10.9.2**、**10.10**、**10.10.1–10.10.5** | 虚实地址、隔离、进程/线程/协程 |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01_foreground** | 前后台 super loop vs 响应延迟 | **10.1** |
| **demo02_tcb_coop** | TCB、独立栈、协作式 `os_yield` | **10.1.1**、**10.2.1–10.2.2** |
| **demo03_preempt** | 时间片、setjmp 主机版 / 可选 ARM SysTick | **10.1.3**、**10.2.1**、**10.3.1** |
| **demo04_semaphore** | 信号量同步、阻塞唤醒 | **10.2.4** |
| **demo05_queue** | 消息队列、生产者-消费者 | **10.2.4**、**10.10.3** |
| **demo06_mini_rtos** | 分层迷你 RTOS（kernel/sync/port/app） | **10.2**、**ch08/ch09** capstone |

## 考核要点

1. 区分 **并发与并行**，举例单核 RTOS vs 多核 DPDK lcore（**10.1**、**10.10.2**）
2. 画出 **协作式 vs 抢占式** 调度时序，说明 SysTick + PendSV 分工（**10.2.1**、**10.3.1**）
3. 描述 **TCB 字段** 与 **每任务独立栈** 的必要性；说明 **canary** 检测栈溢出（**10.2.2**、**ch05**）
4. 写出 ARM **上下文切换** 三步：`stmfd` 保存 → 换 SP → `ldmfd` 恢复（**ch03**、**demo03**）
5. 解释 **就绪 bitmap** 如何 O(1) 选最高优先级任务（**10.2.1**）
6. 对比 **mutex、semaphore、queue、event group** 适用场景（**10.2.4**）
7. 说明 **优先级反转** 成因与 **优先级继承 mutex**（**10.2.4**）
8. 列举 **ISR 禁止事项** 与 **FromISR** 唤醒任务流程（**10.3.3**）
9. 区分 **libc API、syscall、RTOS API**；用 **strace** 追踪一次 `open`（**10.4**）
10. 说明 **MMU 页表** 如何实现进程隔离；对比 RTOS 无 MMU 风险（**10.9**、**10.10.1**）
11. 解释 **rootfs 挂载** 与 **initramfs** 在嵌入式 Linux 启动中的作用（**10.5.3**、**10.6.3**）
12. 完成 **demo06_mini_rtos** 模块划分说明，并列举多任务 **四大陷阱** 及对策（**10.10.5**）

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **ch03** 汇编栈；**ch05** 栈；**ch08** TCB/OOP；**ch09** 模块化构建 |
| 后置 | 全书 **04-Kernel-Prep-Embedded-C-Self-Cultivation** 完结；可深入 **Linux 内核 / DPDK / 板级 BSP** |

## 小节

- [10.1 多任务的裸机实现](./10.1-bare-metal/10.1-多任务的裸机实现.md)
  - [10.1.1 多任务的模拟实现](./10.1-bare-metal/10.1.1-多任务的模拟实现.md)
  - [10.1.2 改变任务的执行频率](./10.1-bare-metal/10.1.2-改变任务的执行频率.md)
  - [10.1.3 改变任务的执行时间](./10.1-bare-metal/10.1.3-改变任务的执行时间.md)
- [10.2 操作系统基本原理](./10.2-os-basics/10.2-操作系统基本原理.md)
  - [10.2.1 调度器工作原理](./10.2-os-basics/10.2.1-调度器工作原理.md)
  - [10.2.2 函数栈与进程栈](./10.2-os-basics/10.2.2-函数栈与进程栈.md)
  - [10.2.3 可重入函数](./10.2-os-basics/10.2.3-可重入函数.md)
  - [10.2.4 临界区与临界资源](./10.2-os-basics/10.2.4-临界区与临界资源.md)
- [10.3 中断](./10.3-interrupt/10.3-中断.md)
  - [10.3.1 中断处理流程](./10.3-interrupt/10.3.1-中断处理流程.md)
  - [10.3.2 进程栈与中断栈](./10.3-interrupt/10.3.2-进程栈与中断栈.md)
  - [10.3.3 中断函数的实现](./10.3-interrupt/10.3.3-中断函数的实现.md)
- [10.4 系统调用](./10.4-syscall/10.4-系统调用.md)
  - [10.4.1 操作系统的API](./10.4-syscall/10.4.1-操作系统的API.md)
  - [10.4.2 操作系统的权限管理](./10.4-syscall/10.4.2-操作系统的权限管理.md)
  - [10.4.3 CPU的特权模式](./10.4-syscall/10.4.3-CPU的特权模式.md)
  - [10.4.4 Linux系统调用接口](./10.4-syscall/10.4.4-Linux系统调用接口.md)
- [10.5 揭开文件系统的神秘面纱](./10.5-filesystem/10.5-揭开文件系统的神秘面纱.md)
  - [10.5.1 什么是文件系统](./10.5-filesystem/10.5.1-什么是文件系统.md)
  - [10.5.2 文件系统的挂载](./10.5-filesystem/10.5.2-文件系统的挂载.md)
  - [10.5.3 根文件系统](./10.5-filesystem/10.5.3-根文件系统.md)
- [10.6 存储器接口与映射](./10.6-memory-map/10.6-存储器接口与映射.md)
  - [10.6.1 存储器与接口](./10.6-memory-map/10.6.1-存储器与接口.md)
  - [10.6.2 存储映射](./10.6-memory-map/10.6.2-存储映射.md)
  - [10.6.3 嵌入式启动方式](./10.6-memory-map/10.6.3-嵌入式启动方式.md)
- [10.7 内存与外部设备](./10.7-io/10.7-内存与外部设备.md)
  - [10.7.1 内存与外存](./10.7-io/10.7.1-内存与外存.md)
  - [10.7.2 外部设备](./10.7-io/10.7.2-外部设备.md)
  - [10.7.3 I/O端口与I/O内存](./10.7-io/10.7.3-IO端口与IO内存.md)
- [10.8 寄存器操作](./10.8-register/10.8-寄存器操作.md)
  - [10.8.1 位运算应用](./10.8-register/10.8.1-位运算应用.md)
  - [10.8.2 操作寄存器](./10.8-register/10.8.2-操作寄存器.md)
  - [10.8.3 位域](./10.8-register/10.8.3-位域.md)
- [10.9 内存管理单元MMU](./10.9-mmu/10.9-内存管理单元MMU.md)
  - [10.9.1 地址转换](./10.9-mmu/10.9.1-地址转换.md)
  - [10.9.2 权限管理](./10.9-mmu/10.9.2-权限管理.md)
- [10.10 进程、线程和协程](./10.10-process-thread/10.10-进程-线程和协程.md)
  - [10.10.1 进程](./10.10-process-thread/10.10.1-进程.md)
  - [10.10.2 线程](./10.10-process-thread/10.10.2-线程.md)
  - [10.10.3 线程池](./10.10-process-thread/10.10.3-线程池.md)
  - [10.10.4 协程](./10.10-process-thread/10.10.4-协程.md)
  - [10.10.5 小结](./10.10-process-thread/10.10.5-小结.md)


---

## 章节自测

> 多任务与 OS 是嵌入式进阶终点。看代码 → 想答案 → 点开验证。

### Q1: 上下文切换

```c
// 简化的上下文切换（RTOS）
void switch_to(TCB *next) {
    // 1. 保存当前寄存器到当前 TCB 的栈
    save_context(&current->stack_ptr);
    // 2. 切换栈指针
    current = next;
    // 3. 从 next TCB 的栈恢复寄存器
    restore_context(&current->stack_ptr);
    // 4. PC 跳转（通过 ret/bx lr）
}
```

> 上下文切换保存哪些寄存器？为什么不能切换正在 `malloc` 的线程？

<details>
<summary>答案与复习指引</summary>

**答案：** 保存通用寄存器（`R0-R12`）、`SP`、`LR`（返回地址）、`PSR`（状态寄存器）。

不能切换正在 `malloc` 的线程——`malloc` 持有**堆锁**，切走后其他线程调 `malloc` → **死锁**。

**RTOS 解决方案：**
- 关中断（`cpsid i`）后再切换 → 防止中断处理函数干扰
- 使用无锁数据结构（DPDK ring）避免锁
- 限制切换点（只在安全的地方 yield）

**复习：** → [10.3 上下文切换](./10.3-context-switch/10.3-上下文切换.md)

</details>

### Q2: mutex vs semaphore

```c
// mutex — 互斥锁
pthread_mutex_t lock;
pthread_mutex_lock(&lock);
shared_data++;
pthread_mutex_unlock(&lock);

// semaphore — 信号量
sem_t sem;
sem_wait(&sem);   // P 操作, count--
// ... 访问资源 ...
sem_post(&sem);   // V 操作, count++
```

> mutex 和 semaphore 有什么区别？

<details>
<summary>答案与复习指引</summary>

**答案：**
- **mutex** = 二值信号量（0 或 1），保护临界区。有**归属**：谁 lock 谁 unlock。可优先级继承。
- **semaphore** = 计数信号量（N 个资源），用于**同步**（如生产者-消费者）。无归属：任何人都可以 post。

**互斥**用 mutex，**同步**用 semaphore。混用容易死锁。

**内核/DPDK：** `spinlock`（忙等）、`mutex`（可睡眠）、`semaphore`（计数同步）、`completion`（一次性同步）。

**复习：** → [10.6 同步机制](./10.6-sync/10.6-同步机制.md)

</details>

### Q3: syscall 与用户/内核态

```c
// 用户态调用 write
int fd = 1;
write(fd, "hello", 5);

// 硬件层面发生了什么？
// 1. 用户态执行 syscall 指令
// 2. ???
// 3. 内核执行 sys_write
// 4. ???
// 5. 返回用户态
```

<details>
<summary>答案与复习指引</summary>

**答案：**
1. 用户态执行 `syscall`（x86-64）/ `svc`（ARM）→ 触发**异常/陷阱**
2. CPU 切到内核态（特权模式），跳到预定义的异常向量
3. 内核保存用户态上下文（寄存器），查系统调用号 → 调 `sys_write`
4. `sys_write` 执行实际 I/O（可能拷贝数据到内核缓冲 → 驱动 → 硬件）
5. 内核恢复用户态上下文，`sysret`/`eret` 返回用户态

**开销：** 一次 syscall 约 ~1-2μs（用户→内核切换 + TLB/Cache 影响）。HFT 尽量减少 syscall，用 `io_uring` 或共享内存。

**复习：** → [10.9 Linux 系统调用](./10.9-syscall/10.9-Linux系统调用.md)

</details>

### Q4: MMU 地址转换

```
// 虚拟地址 0x400000 → 物理地址 ?
// 页表怎么查？

VA = 0x0000000000400000
Page dir → Page table → Page frame → Physical address
```

> MMU 怎么把虚拟地址翻译成物理地址？TLI 是什么？

<details>
<summary>答案与复习指引</summary>

**答案：** MMU 通过**页表**（多级）翻译虚拟地址：
1. 虚拟地址拆分为页号 + 页内偏移
2. 页号 → 查页目录 → 找到页表条目
3. 页表条目包含物理页帧号 + 权限位
4. 物理页帧号 + 页内偏移 = 物理地址

**TLB**（Translation Lookaside Buffer）= 页表项的**硬件缓存**，避免每次访存都查多级页表。TLI miss → 走多级页表（慢）。

**HFT 优化：** 用 `madvise`/hugepage 减少 TLB miss。大页（2MB/1GB）让一条 TLB 条目覆盖更多内存。

**复习：** → [10.9 MMU 与地址转换](./10.9-syscall/10.9.4-Linux系统调用.md)
