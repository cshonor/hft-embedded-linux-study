# §21.8 实验要点

> **来源：** [Ch21 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Ch21 的实验通过在 BenOS 上逐步实现进程管理，将前面学到的 A64 指令、异常、中断知识综合运用到操作系统实践中。

## 核心要点

### 实验列表

| 实验 | 内容 | 平台 | 核心知识点 |
|------|------|------|-----------|
| 21-1 | 观察栈布局（GDB 看栈帧） | QEMU | AAPCS64 栈帧、FP/LR |
| 21-2 | 进程创建（do_fork） | QEMU | PCB、栈设计 |
| 21-3 | 进程调度（轮转 + 定时器） | QEMU | schedule、switch_to |
| 21-4 | 新增 malloc 系统调用 | QEMU | SVC、syscall 分发 |
| 21-5 | 新增 clone 系统调用 | QEMU | fork + syscall |

### 实验 21-1：观察栈布局

用 GDB 在函数入口设断点，观察栈帧布局：

```bash
# QEMU + GDB 调试
qemu-system-aarch64 -M virt -cpu cortex-a57 -m 128M \
    -nographic -kernel benos.bin -S -gdb tcp::1234

# 另一个终端
aarch64-linux-gnu-gdb benos.elf
(gdb) target remote :1234
(gdb) break main
(gdb) continue
(gdb) break my_function
(gdb) continue

# 查看栈帧
(gdb) info registers sp fp x30       # SP/FP/LR
(gdb) x/16gx $sp                     # 栈内容
(gdb) info frame                     # 当前栈帧信息
(gdb) backtrace                      # 调用栈
(gdb) x/gx $fp                       # 上一个 FP
(gdb) x/gx $fp+8                     # 调用者返回地址
```

| GDB 命令 | 作用 | 查看内容 |
|----------|------|---------|
| `info registers sp` | 查看栈指针 | 当前 SP 值 |
| `info registers fp` | 查看帧指针 | X29 值 |
| `info registers x30` | 查看返回地址 | LR 值 |
| `x/16gx $sp` | 查看栈内容 | 16 个 64 位值 |
| `info frame` | 栈帧详情 | 保存的寄存器 |
| `backtrace` | 调用栈 | 沿 FP 链回溯 |

### 实验 21-2：进程创建

```
实验步骤：
1. 定义 task_struct 结构
2. 实现 alloc_task() 分配 PCB
3. 实现 alloc_stack() 分配栈
4. 实现 do_fork(fn) 创建进程
5. 创建 task1（打印 PID 的函数）
6. 验证 task1 的栈和 PCB 初始化正确

验证方法：
- GDB 检查 task1->sp 指向栈顶
- GDB 检查 *(task1->sp) == fn 地址
- GDB 检查 task1->pid == 1
- GDB 检查 task1->next == task0（循环链表）
```

### 实验 21-3：进程调度

```
定时器中断 → timer_irq_handler → schedule → switch_to → 新进程运行
                                                ↓
                                          新进程打印 PID
                                                ↓
                                          下一次定时器中断 → 再调度

预期输出：
[task0] running
[task1] running, pid=1
[task2] running, pid=2
[task1] running, pid=1
[task2] running, pid=2
...

调试技巧：
- 在 switch_to 设断点，观察 prev/next 的 PCB
- 单步执行 STP/LDP 序列，验证寄存器保存/恢复
- 在 timer_irq_handler 设断点，观察中断触发频率
```

### 实验 21-4：malloc 系统调用

```c
// 用户态测试
void user_main(void) {
    char *p = (char *)my_malloc(128);
    // 打印 p 地址验证
    printf("malloc returned: %p\n", p);
}

// 验证步骤：
// 1. GDB 在 svc_handler 设断点
// 2. 检查 X8 == 1 (SYS_malloc)
// 3. 检查 X0 == 128 (size 参数)
// 4. 单步到 do_malloc 返回
// 5. 检查 X0 == 分配的地址
// 6. 继续，用户态拿到正确的返回值
```

### 实验 21-5：clone 系统调用

```
clone 系统调用链：
用户态: my_clone(fn)
  → SVC #0 (X8=2, X0=fn)
  → svc_handler
  → sys_clone → do_fork(fn)
  → 新进程加入调度队列
  → 下次调度时新进程从 fn 开始执行

验证：
- 创建后 next_pid 递增
- 新进程 PCB 中 LR = fn
- 调度后新进程打印自己的 PID
```

## HFT 关联

这些实验是理解操作系统调度的基础。HFT 开发者需要理解：(1) 实验 21-1 的栈帧分析技巧在调试 crash 时直接使用（`gdb` 的 `bt` 或 `crash` 工具）；(2) 实验 21-3 的调度机制帮助理解为什么交易线程会被抢占以及如何避免；(3) 实验 21-4 的 syscall 开销测量是 HFT 性能优化的基本技能。

```c
// HFT 上下文切换开销测量实验
#include <sched.h>
#include <time.h>
#include <stdio.h>

#define N 10000

int pipefd[2];

void measure_ctx_switch() {
    struct timespec start, end;

    pipe(pipefd);

    if (fork() == 0) {
        // 子进程：读管道
        for (int i = 0; i < N; i++) {
            char c;
            read(pipefd[0], &c, 1);
        }
        _exit(0);
    }

    // 父进程：写管道 + 测量
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < N; i++) {
        write(pipefd[1], "x", 1);  // 唤醒子进程
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double total_ns = (end.tv_sec - start.tv_sec) * 1e9
                    + (end.tv_nsec - start.tv_nsec);
    double per_switch = total_ns / N;
    printf("Context switch: %.0f ns/switch (%d switches)\n",
           per_switch, N);
    // 典型结果：AArch64 ~2-5μs/switch
}

// HFT syscall 开销测量
void measure_syscall_overhead() {
    struct timespec start, end;
    int dummy_fd = open("/dev/null", O_RDONLY);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < N; i++) {
        // 每次触发一次 read syscall
        char c;
        read(dummy_fd, &c, 1);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double total_ns = (end.tv_sec - start.tv_sec) * 1e9
                    + (end.tv_nsec - start.tv_nsec);
    printf("Syscall (read): %.0f ns/call\n", total_ns / N);
    // 典型结果：AArch64 ~1-2μs/call

    close(dummy_fd);
}
```

| 实验 | HFT 对应技能 | 工具 |
|------|-------------|------|
| 21-1 栈帧 | crash 调试 | gdb, crash |
| 21-3 调度 | 延迟抖动诊断 | perf sched, /proc/schedstat |
| 21-4 syscall | syscall 开销测量 | perf trace, strace |
| 21-5 clone | 线程创建 | pthread_create |

## 自测题

1. **在 GDB 中如何查看当前函数的栈帧？FP 指向什么？**

<details>
<summary>答案</summary>

用 `info frame` 查看当前栈帧，`info registers fp sp x30` 查看 FP/SP/LR。FP(X29) 指向栈帧中保存的**上一个 FP 的地址**——即调用者的 FP。通过 `x/gx $fp` 可以读到上一个 FP，`x/gx $fp+8` 可以读到调用者的返回地址(LR)。`backtrace` 命令自动沿 FP 链表遍历打印调用栈。
</details>

2. **实验 21-3 中，如果定时器中断频率设得太低会怎样？太高呢？**

<details>
<summary>答案</summary>

**太低**：每个进程获得很长的执行时间，看起来像顺序执行而非并发——响应延迟高。**太高**：频繁的上下文切换开销占比过大，实际有效工作时间减少（thrashing）。BenOS 课堂实验中一般设 100-1000Hz。Linux 默认 250Hz（CONFIG_HZ=250），实时内核可选 1000Hz。HFT 系统反而用更低频率或 NOHZ_FULL 关闭调度时钟——因为交易线程不需要被抢占，频繁的定时器中断反而是干扰。
</details>

## 参考与延伸

- [§21.9 易错点清单](09-pitfalls.md) — 实验中常见错误
- [Ch12 中断处理](../../chapter-12-interrupt-handling/notes/section-0-本章完整概述.md) — 定时器中断配置
