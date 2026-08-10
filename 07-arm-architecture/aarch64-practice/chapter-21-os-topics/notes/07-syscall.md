# §21.7 自定义系统调用 ⭐

> **来源：** [Ch21 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

在 BenOS 中定义自己的 SVC 系统调用：用户态通过 SVC 指令陷入内核，异常处理读取系统调用号分发到对应处理函数，返回值通过 X0 传回用户态。

## 核心要点

### 用户态调用

```c
int my_malloc(size_t size) {
    // x8 = syscall number, x0 = 参数
    register long x8 asm("x8") = 1;  // SYS_malloc
    register long x0 asm("x0") = size;
    asm volatile("svc #0" : "+r"(x0) : "r"(x8) : "memory");
    return x0;  // 返回值
}
```

### SVC 异常处理

```asm
svc_handler:
    ; 保存现场（保存所有通用寄存器到栈）
    sub sp, sp, #256
    stp x0, x1, [sp, #0]
    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]      ; X8 = syscall number
    ; ... 保存 X10-X30 ...

    ; 读系统调用号（从栈上保存区读 X8）
    ldr x8, [sp, #64]

    ; 分发
    cmp x8, #1
    b.eq sys_malloc
    cmp x8, #2
    b.eq sys_clone
    b unknown_syscall

sys_malloc:
    ldr x0, [sp, #0]           ; 恢复参数（size）
    bl  do_malloc              ; 调用 C 实现
    str x0, [sp, #0]           ; 返回值写回栈上 X0 位置

    ; 恢复现场
    ldp x0, x1, [sp, #0]       ; X0 现在是返回值
    ; ... 恢复 X2-X30 ...
    add sp, sp, #256
    eret                       ; 返回用户态
```

### AArch64 syscall 约定

| 寄存器 | 用途 | 方向 | 备注 |
|--------|------|------|------|
| X8 | 系统调用号 | 用户→内核 | 不同 x86 的 RAX |
| X0-X5 | 参数（最多6个） | 用户→内核 | 同 C 调用约定 |
| X0 | 返回值 | 内核→用户 | 负值=错误码 |
| X6-X7 | 保留/临时 | — | 可能被覆盖 |
| X16-X17 | 被内核使用 | — | 调用约定允许 |

### AArch64 vs x86-64 syscall 对比

| 维度 | AArch64 | x86-64 |
|------|---------|--------|
| 陷入指令 | SVC #0 | SYSCALL |
| 系统调用号 | X8 | RAX |
| 参数 | X0-X5 | RDI/RSI/RDX/R10/R8/R9 |
| 返回值 | X0 | RAX |
| 入口查找 | VBAR_EL1 + 向量表偏移 | MSR_LSTAR 直接跳转 |
| 状态保存 | SPSR_EL1 + ELR_EL1 | R11/RFLAGS (SWAPGS) |
| 开销 | ~1-5μs | ~0.5-2μs |

### malloc 和 clone 系统调用

```c
long sys_malloc(size_t size) {
    void *p = simple_alloc(size);  // 简单 bump allocator
    return (long)p;
}

long sys_clone(void (*fn)(void)) {
    return do_fork(fn);  // 创建新进程
}
```

### 完整流程

```
用户态                      内核态
  │                           │
  │ X8=1, X0=size             │
  │ SVC #0 ──────────────────→│ SVC 异常入口
  │                           │ 硬件自动保存 SPSR/ELR
  │                           │ 软件保存 X0-X30 到栈
  │                           │ 读 X8 → 分发到 sys_malloc
  │                           │ 调用 do_malloc(size)
  │                           │ 结果写回栈上 X0 位置
  │ ←──────────────── ERET ──│ 恢复 X0-X30 从栈
  │ X0 = 返回值               │ ERET 恢复 SPSR/ELR
  │ 继续执行                   │
```

### SVC 指令的硬件行为

```
SVC #0 执行后，CPU 自动完成：
┌─────────────────────────────────────────┐
│ 1. PSTATE → SPSR_EL1                    │  保存当前状态
│ 2. SVC 下一条指令 → ELR_EL1             │  保存返回地址
│ 3. PSTATE.M = EL1                       │  切换到 EL1
│ 4. PSTATE.D = 1, PSTATE.I = 1           │  屏蔽中断
│ 5. SP → SP_EL1                          │  切换到内核栈
│ 6. PC → VBAR_EL1 + 0x200               │  跳到异常向量
└─────────────────────────────────────────┘

异常向量表布局（VBAR_EL1 基址）：
+0x000  Current EL with SP_EL0 (同步)
+0x080  Current EL with SP_EL0 (IRQ)
+0x100  Current EL with SP_EL0 (FIQ)
+0x180  Current EL with SP_EL0 (SError)
+0x200  Current EL with SP_ELx (同步) ← SVC 走这里
+0x280  Current EL with SP_ELx (IRQ)
...
+0x400  Lower EL (AArch64) 同步
...
```

## HFT 关联

系统调用是 HFT 延迟的隐形杀手——一次 SVC 陷入约 1-5μs（保存/恢复寄存器 + TLB 切换 + 内核代码执行）。

```c
// HFT 减少 syscall 的策略

// 策略1: 零 syscall 热路径
// 所有数据预加载到共享内存，不读文件不 socket
void hft_hot_path() {
    // 初始化阶段（一次性 syscall）
    int fd = shm_open("/hft_data", O_RDWR, 0666);
    void *data = mmap(NULL, SHM_SIZE, PROT_READ|PROT_WRITE,
                       MAP_SHARED, fd, 0);

    // 热路径：零 syscall
    while (running) {
        struct market_data *md = (struct market_data *)data;
        if (md->sequence != last_seq) {
            process(md);  // 纯用户态计算
            last_seq = md->sequence;
        }
    }
}

// 策略2: vDSO 替代时钟 syscall
// 坏：clock_gettime 每次陷入内核
// 好：vDSO 在用户态直接读内核映射的时钟页面
struct timespec ts;
clock_gettime(CLOCK_MONOTONIC, &ts);  // vDSO 不陷入！

// 策略3: io_uring 替代同步 IO
// 坏：每次 read/write 都陷入
// 好：提交一批请求，内核异步处理，减少陷入次数

// 策略4: 批量 syscall
// 坏：每个操作单独 syscall
for (int i = 0; i < N; i++)
    send(sock, &pkt[i], sizeof(pkt[i]), 0);  // N 次 syscall

// 好：用 sendmmsg 一次提交
struct mmsghdr msgs[N];
sendmmsg(sock, msgs, N, 0);  // 1 次 syscall
```

| HFT syscall 优化 | 原始开销 | 优化后 | 方法 |
|-----------------|---------|--------|------|
| clock_gettime | ~1μs | ~20ns | vDSO |
| send/recv | ~2μs/次 | ~0.2μs/包 | io_uring |
| sendmmsg | N×2μs | 2μs | 批量 |
| mmap 共享内存 | 0（热路径） | 0 | 预映射 |
| epoll_wait | ~1μs | ~50ns | busy poll |

## 自测题

1. **AArch64 的 syscall 调用约定中，系统调用号放在哪个寄存器？和 x86 有什么区别？**

<details>
<summary>答案</summary>

AArch64 的系统调用号放在 **X8**（不是 X0），参数放 X0-X5，返回值放 X0。x86-64 Linux 的系统调用号放在 **RAX**，参数放 RDI/RSI/RDX/R10/R8/R9，返回值放 RAX。另一个区别：AArch64 用 `SVC #0` 指令陷入，x86-64 用 `SYSCALL` 指令。SVC 通过异常向量表跳转（查表开销），SYSCALL 直接从 MSR 寄存器读取入口地址（更快）。
</details>

2. **SVC 异常处理中如何获取系统调用号？为什么不能直接读 X8？**

<details>
<summary>答案</summary>

SVC 异常入口会保存所有通用寄存器到栈上（用于后续恢复用户态）。保存后，原始 X8 的值在栈上的某个位置（偏移取决于栈帧布局），不能直接读 X8 寄存器（因为 X8 可能已被异常处理代码覆盖）。代码 `LDR X8, [SP, #64]` 从栈上保存区读取原始 X8 的值。这是裸机/内核开发中常见的模式——异常入口保存全部寄存器后，从栈上读取需要的值。
</details>

3. **用户态调用 svc 后，CPU 自动做了哪些事情？**

<details>
<summary>答案</summary>

SVC 指令触发同步异常，CPU 自动完成：(1) **PSTATE 保存到 SPSR_EL1**（当前 DAIF/EL 等状态）；(2) **返回地址保存到 ELR_EL1**（SVC 指令的下一条）；(3) **PSTATE 切换**：EL 切换到 EL1，DAIF 屏蔽（中断关闭），SP 切换到 SP_EL1（内核栈）；(4) **PC 跳转到 VBAR_EL1 + 偏移**（异常向量表入口）。这些全由硬件自动完成，软件在异常处理入口处再保存通用寄存器。
</details>

## 参考与延伸

- [§21.5 上下文切换](05-context-switch.md) — clone syscall 调用 do_fork
- [Ch11 异常处理](../../chapter-11-exception-handling/notes/section-0-本章完整概述.md) — SVC 异常的完整处理流程
- [Ch10 GCC 内联汇编](../../chapter-10-gcc-inline-asm/notes/section-0-本章完整概述.md) — register asm 变量绑定
