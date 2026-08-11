# 6.2 SVC 系统调用

> 来源：§6.2 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

SVC（Supervisor Call）指令触发同步异常实现系统调用，从 EL0 切换到 EL1。这是用户态进入内核的唯一正门。

## 核心要点

### SVC 指令

```asm
SVC #0    ; 触发同步异常，从 EL0 切换到 EL1
; SVC #imm 中的 imm 是立即数，Linux 固定用 #0
; 内核通过 ESR_EL1 的 EC 字段（EC=0x15）识别这是 SVC
```

### Linux AArch64 系统调用约定

```asm
; 完整的系统调用流程（以 write 为例）
MOV x8, #64          ; syscall number：write=64
MOV x0, #1           ; arg0: fd=stdout
ADR x1, msg          ; arg1: buffer 地址
MOV x2, #12          ; arg2: length
SVC #0               ; 触发系统调用
; 返回值在 x0
```

| 寄存器 | 用途 | 说明 |
|--------|------|------|
| x8 | 系统调用号 | Linux syscall number |
| x0-x5 | 参数 | 最多 6 个参数 |
| x0 | 返回值 | 系统调用返回值 |
| x6-x7, x16-x17 | 临时 | 内核可能修改 |
| x8 之上 | 保留 | 被调用者保存 |

### SVC 执行后的硬件流程

```
用户态 EL0                          内核态 EL1
──────────                          ──────────
SVC #0 执行
    │
    ├─→ 1. 硬件保存 PC → ELR_EL1（异常返回地址）
    ├─→ 2. 硬件保存 PSTATE → SPSR_EL1（处理器状态）
    ├─→ 3. 切换到 EL1 的 SP（SP_EL1）
    ├─→ 4. PSTATE 更新（EL→1, 屏蔽中断等）
    ├─→ 5. PC ← VBAR_EL1 + 0x200（同步异常向量）
    │         （低EL用AArch64→当前EL AArch64 同步异常）
    │
    │                               ─────────────────────
    │                               vector_table + 0x200:
    │                                 ← 内核异常处理入口
    │                                 1. 保存 x0-x30 到栈
    │                                 2. 读 ESR_EL1 → EC=0x15(SVC)
    │                                 3. 读 x8 → syscall number
    │                                 4. 查 sys_call_table[x8]
    │                                 5. 调用 sys_write(...)
    │                                 6. 返回值 → x0
    │                                 7. 恢复 x0-x30
    │                                 8. ERET → 恢复 PC/EL
    │                               ─────────────────────
    │
    ←─ 返回用户态 EL0（x0 = 返回值）
```

### 异常向量表偏移

| 偏移 | 异常类型 | 来源 |
|------|----------|------|
| 0x000 | 同步异常 | 当前EL → 当前EL |
| 0x080 | IRQ | 当前EL → 当前EL |
| 0x100 | FIQ | 当前EL → 当前EL |
| 0x180 | SError | 当前EL → 当前EL |
| **0x200** | **同步异常** | **低EL → 当前EL（SVC 走这里）** |
| 0x280 | IRQ | 低EL → 当前EL |
| 0x300 | FIQ | 低EL → 当前EL |
| 0x380 | SError | 低EL → 当前EL |

> SVC 从 EL0 触发到 EL1 处理，属于"低EL → 当前EL"的同步异常，向量偏移 0x200。

### ESR_EL1 寄存器

```
ESR_EL1 (Exception Syndrome Register) 格式：
  [31:26] EC  - Exception Class（异常类别）
  [25]    IL  - Instruction Length（32位=1）
  [24:0]  ISS - Instruction Specific Syndrome（具体信息）

EC=0x15 → SVC 指令（AArch64）
EC=0x21 → 数据中止（对齐/权限）
EC=0x22 → 对齐异常
EC=0x24 → 非法指令
```

### 常见系统调用号（AArch64 Linux）

| syscall # | 名称 | 用途 |
|-----------|------|------|
| 63 | read | 读文件 |
| 64 | write | 写文件 |
| 56 | openat | 打开文件 |
| 57 | close | 关闭文件 |
| 169 | gettimeofday | 获取时间 |
| 172 | getpid | 获取PID |
| 221 | execve | 执行程序 |

### ARM32 SWI → AArch64 SVC 设计演进

```
ARM32 系统调用（SWI / SVC）：
  SWI #0x900000 + syscall_num
  ; 24 位立即数空间，Linux 用高 8 位 0x90 标记
  ; 内核从指令编码中提取 syscall number

  问题：
  1. 立即数只有 24 位，系统调用号受限于编码空间
  2. 内核必须解码指令才能获取调用号 → 额外开销
  3. 不同 OS 约定不同（Linux vs 其他）→ 不可移植

AArch64 系统调用（SVC）：
  MOV x8, #syscall_num
  SVC #0
  ; 调用号在 x8 寄存器中，不在指令里

  改进：
  1. x8 是 64 位寄存器 → 调用号空间无限
  2. 内核直接读 x8 → 不需要解码指令
  3. SVC #0 的立即数留给 OS 自由使用（Linux 不用，其他 OS 可标记服务类型）
  4. 统一约定 → 内核只需查 sys_call_table[x8]
```

> 这体现了 AArch64 的设计理念：**让寄存器承担数据传递，指令只做控制**。寄存器比指令立即数灵活得多——可以动态计算、可以间接传递、不受编码位数限制。

### SVC 异常处理时序

```
SVC #0 执行后的 CPU 硬件时序（不可中断的原子操作）：

  Cycle 1: 硬件保存
    ELR_EL1  ← PC（SVC 指令的下一条地址）
    SPSR_EL1 ← PSTATE（当前 EL、标志位、中断屏蔽状态）
    
  Cycle 2: 状态切换
    SP      ← SP_EL1（切换到 EL1 的栈指针）
    PSTATE.EL ← 1（进入 EL1）
    PSTATE.D ← 1（屏蔽 Debug 异常）
    PSTATE.I ← 1（屏蔽 IRQ，防止内核入口被打断）
    PSTATE.A ← 1（屏蔽 SError）
    
  Cycle 3: 跳转
    PC ← VBAR_EL1 + 0x200（同步异常向量）
    → 开始执行内核的 vector table 入口代码

  软件阶段（内核异常处理）：
    1. 保存 x0-x30 到内核栈（STP 批量存储）
    2. MRS x0, ESR_EL1 → 读异常信息，EC=0x15 确认是 SVC
    3. 读 x8 → syscall number
    4. 查 sys_call_table[x8] → 获取内核函数指针
    5. 调用内核函数（参数从 x0-x5 传递）
    6. 返回值 → x0
    7. 恢复 x0-x30（LDP 批量恢复）
    8. ERET → 恢复 PC←ELR_EL1, PSTATE←SPSR_EL1（回到 EL0）
```

> **为什么 PSTATE.I 要在硬件阶段就置 1**：SVC 入口处内核还没保存寄存器，如果此时 IRQ 打断，中断处理程序会覆盖寄存器 → SVC 调用数据丢失。硬件自动屏蔽 IRQ 确保内核入口代码安全执行。

## 与 C 的对照

```c
// C 库函数 write() 的底层
ssize_t write(int fd, const void *buf, size_t count) {
    ssize_t ret;
    // 内联汇编或编译器内置
    register long x8 asm("x8") = 64;  // syscall number
    asm volatile("svc #0"
                 : "=r"(ret)           // x0 = 返回值
                 : "r"(fd), "r"(buf), "r"(count), "r"(x8)
                 : "memory");
    return ret;
}
```

## 常见错误

1. **EL0 执行 SVC 后忘记 x8 设系统调用号**：内核读 x8 查表，如果 x8 是随机值会导致 `sys_call_table` 越界。
2. **混淆 ARM32 的 SWI 和 AArch64 的 SVC**：ARM32 用 `SWI #num`（编号在指令中），AArch64 用 `SVC #0`（编号在 x8 中）。
3. **以为 SVC 的立即数是系统调用号**：SVC #0 的 #0 只是立即数标签，Linux 内核不读这个值，而是读 x8。

## HFT 关联

系统调用是 HFT 低延迟的大敌：
- 每次 SVC 产生异常切换开销 ~1-5μs（EL0→EL1→EL0）
- HFT 关键路径尽量避免 syscall → 预分配内存、避免 I/O
- io_uring（5.1+）把多次 I/O 系统调用批量提交 → 减少 SVC 次数
- vDSO 把 gettimeofday/clock_gettime 映射到用户态 → 零 syscall 获取时间

```c
// HFT 反模式：热路径中调用系统调用
void process_order() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);  // 如果不走 vDSO → SVC → ~1μs
    write(socket_fd, data, len);          // SVC → ~2-5μs
    // 总计 ~3-6μs 的内核切换开销！
}

// HFT 正模式：预分配 + vDSO + io_uring
void init() {
    // 启动时预分配内存、预打开文件
    // 设置 io_uring 批量提交
}

void process_order() {
    // 用户态完成所有操作
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);  // vDSO → ~20ns（无 SVC）
    // 数据写入 ring buffer，稍后批量 io_uring 提交
}
```

## 自测题

1. SVC #0 执行后 CPU 发生了什么？
<details><summary>答案</summary>
1. 硬件保存 PC→ELR_EL1, PSTATE→SPSR_EL1
2. 切换到 EL1 的 SP
3. 跳转到 VBAR_EL1 中"低EL→当前EL AArch64, 同步异常"表项（偏移 0x200）
4. 软件保存 X0-X30，读 ESR_EL1 确认 EC=0x15(SVC)，读 x8 获取调用号
</details>

2. 为什么 HFT 要尽量减少系统调用？
<details><summary>答案</summary>
每次 SVC 产生异常切换：保存/恢复寄存器、EL 切换、内核调度可能介入。开销 ~1-5μs，而用户态操作只需纳秒级。HFT 热路径中一次意外的 syscall 可能导致错过交易窗口。
</details>

3. vDSO 如何避免系统调用获取时间？
<details><summary>答案</summary>
vDSO 把内核的时间数据（vvar 页）映射到用户空间只读。gettimeofday/clock_gettime 直接在用户态读取 vvar 页的时间值，不需要 SVC。只有时间精度要求极高时才回退到真正的 syscall。
</details>

4. SVC #0 中的 #0 有什么用？内核如何知道是哪个系统调用？
<details><summary>答案</summary>
SVC #0 中的立即数 #0 在 AArch64 Linux 中不被使用。内核通过读取 x8 寄存器的值来确定系统调用号。#0 只是惯例（ARM32 的 SWI 曾把调用号编码在指令中，但 AArch64 改为用 x8 传递，因为 x8 可以容纳更多调用号且更灵活）。
</details>

5. 如果在 EL1（内核态）执行 SVC #0 会发生什么？
<details><summary>答案</summary>
SVC 在 EL1 执行仍然会触发同步异常，但走的是"当前EL→当前EL 同步异常"向量（偏移 0x000 而非 0x200）。内核通常不会在 EL1 执行 SVC（没有意义），但如果发生，异常处理代码需要区分异常来源。实际上 Linux 内核不会在 EL1 使用 SVC，它直接调用内核函数。
</details>

## 参考与延伸

- 原书 §6.2
- [Ch11 异常处理](../../chapter-11-exception-handling/notes/section-0-本章完整概述.md)
- [Ch21 自定义系统调用](../../chapter-21-os-topics/notes/section-0-本章完整概述.md)
