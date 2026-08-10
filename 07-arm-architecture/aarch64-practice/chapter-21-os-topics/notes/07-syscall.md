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
    ; 保存现场
    ; ...
    ; 读系统调用号
    ldr x8, [sp, #64]      ; x8 在栈上的位置
    ; 分发
    cmp x8, #1
    b.eq sys_malloc
    cmp x8, #2
    b.eq sys_clone
    ; ...
sys_malloc:
    bl  do_malloc          ; 调用 C 实现
    ; 恢复现场
    ; ...
    eret
```

### AArch64 syscall 约定

| 寄存器 | 用途 | 方向 |
|--------|------|------|
| X8 | 系统调用号 | 用户→内核 |
| X0-X5 | 参数（最多6个） | 用户→内核 |
| X0 | 返回值 | 内核→用户 |
| X7 | 用于间接 syscall | 保留 |

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
  │                           │ 保存现场
  │                           │ 读 X8 → 分发到 sys_malloc
  │                           │ 调用 do_malloc(size)
  │                           │ 结果写 X0
  │ ←──────────────── ERET ──│ 恢复现场
  │ X0 = 返回值               │
  │ 继续执行                   │
```

## HFT 关联

系统调用是 HFT 延迟的隐形杀手——一次 SVC 陷入约 1-5μs（保存/恢复寄存器 + TLB 切换 + 内核代码执行）。HFT 策略：(1) 交易热路径上零 syscall（数据预加载到共享内存，不读文件不 socket）；(2) 必要的 syscall 在初始化阶段一次性完成；(3) 用 io_uring 替代同步 syscall（减少陷入次数）；(4) AArch64 的 SVC 比 x86 的 SYSCALL 略慢（SVC 需要查向量表，SYSCALL 有专用 MSR 寄存器直接跳转）。

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
