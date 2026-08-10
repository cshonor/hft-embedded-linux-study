# 3.6 特殊访存指令：LDXR/STXR、LDAR/STLR、LDTR/STTR

> 来源：§3.6 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

ARMv8 三组特殊 Load/Store 指令——独占访存（原子操作基础）、Acquire/Release 访存（内存序）、非特权访存（内核安全访问用户空间）。Linux 内核锁、`copy_from_user` 底层大量使用。

## 快速对照表

| 指令组 | 名称 | 核心作用 | 典型内核场景 |
|--------|------|----------|-------------|
| LDXR / STXR | 独占加载存储 | LL-SC 原子操作，失败返回状态码 | 自旋锁、CAS、原子变量 |
| LDAR / STLR | Acquire-Release 访存 | 硬件自带内存序，替代 DMB 屏障 | 内核同步、`std::atomic` |
| LDTR / STTR | 非特权加载存储 | EL1 内核以 EL0 用户权限访问地址 | `copy_from_user`、`copy_to_user` |

### 后缀 R 含义

| 后缀 | 全称 | 含义 |
|------|------|------|
| XR | eXclusive Register | 独占（LL-SC 原子） |
| AR | AcquiRe / Release | 获取/释放内存序 |
| TR | Translate-unpRivileged | 非特权翻译（强制 EL0 权限） |

---

## 一、LDXR / STXR —— 独占加载-存储（LL-SC 无锁原子）

| 指令 | 语法 | 含义 |
|------|------|------|
| LDXR | `LDXR Rd, [Xn]` | 独占方式读内存，同时硬件标记监视地址 Xn |
| STXR | `STXR Rs, Rd, [Xn]` | 尝试把 Rd 写入被监视的内存地址 |

**STXR 状态码：**
- `Rs = 0`：写入成功（独占监视仍有效）
- `Rs ≠ 0`：写入失败（期间内存被别的 CPU 改动过，独占监视被打破）

**用途：** 实现 CAS（比较交换）、自旋锁、信号量，ARM64 硬件原子原语。

### LL-SC 伪代码（CAS 实现）

```asm
; CAS: 比较x1地址的值是否等于w2，等于则写入w4
retry:
    ldxr  w0, [x1]       ; 独占加载，开启硬件监视
    cmp   w0, w2         ; 比较当前值与期望值
    b.ne  exit           ; 不相等，直接退出
    stxr  w3, w4, [x1]   ; 尝试独占写
    cmp   w3, #0         ; 检查写入状态
    b.ne  retry          ; 写失败（监视被打破），重新来
exit:
```

### 关键规则

- LDXR 和 STXR **必须成对**使用
- LDXR 和 STXR 中间**不能随便访问别的内存**（可能打破独占监视器）
- 多核之间靠**硬件独占监视器**（Exclusive Monitor）协调
- STXR 的第一个寄存器 Rs 是**状态码输出**（不是数据），0=成功
- ARMv8.1 LSE 原子指令（CAS/LDADD 等）可替代 LL-SC，低争用时更快

> 详见 [Ch20 原子操作](../../chapter-20-atomic-operations/notes/section-0-本章完整概述.md)。

---

## 二、LDAR / STLR —— Acquire / Release 内存序指令

| 指令 | 全称 | 语义 |
|------|------|------|
| LDAR | Load-Acquire | 加载，自带 acquire 语义：**这条加载之后的读写，不能跑到这条指令前面** |
| STLR | Store-Release | 存储，自带 release 语义：**这条存储之前的读写，不能跑到这条指令后面** |

### 内存序可视化

```
// acquire (LDAR) — 后面的不能往前跑
[其他读写]  ──→  不能跑到 LDAR 前面
                  LDAR x0, [x1]   ← 屏障
[后续读写]  ──→  正常执行

// release (STLR) — 前面的不能往后跑
[之前的读写] ──→ 正常执行
                  STLR x0, [x1]   ← 屏障
[其他读写]  ──→  不能跑到 STLR 前面
```

### 用途

- ARMv8 弱内存模型的同步指令，实现 C++ `std::memory_order_acquire` / `release`
- 内核读写锁、RCU 大量使用
- 对比 DMB 内存屏障：LDAR/STLR 把内存屏障和 load/store **合并成单条指令**，性能更好——DMB 会过度排序不相关的访问，LDAR/STLR 只排序相关访问

### LDR vs LDAR 区别

| 特性 | LDR（普通加载） | LDAR（Acquire 加载） |
|------|----------------|---------------------|
| 内存序保证 | 无（可被重排） | acquire 语义（后续读写不能前移） |
| 等价语义 | 普通读 | 读 + 内存屏障 |
| 性能 | 最快 | 略慢（但有屏障效果） |
| 适用场景 | 无并发要求 | 需要同步的并发场景 |

> 详见 [Ch18 内存屏障](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) 和 [§18.4 acquire/release](../../chapter-18-memory-barriers/notes/04-acquire-release.md)。

---

## 三、LDTR / STTR —— 非特权访问指令（内核访问用户空间）

| 指令 | 语法 | 含义 |
|------|------|------|
| LDTR | `LDTR w0, [x1]` | EL1 执行，访问 x1 地址，**使用 EL0 权限检查** |
| STTR | `STTR w0, [x1]` | EL1 执行，写内存，**使用 EL0 权限检查** |

### 核心场景：copy_from_user / copy_to_user

内核 EL1 要读写用户态地址。普通 LDR 在 EL1 会直接绕过用户权限检查；用 LDTR/STTR，即使在内核，硬件依然检查：**这个地址用户态是否有权读写**。如果用户地址非法、不可访问，直接触发异常。

### LDR vs LDTR 对比

| 特性 | LDR（EL1 下） | LDTR（EL1 下） |
|------|--------------|----------------|
| 权限检查级别 | EL1 内核权限 | **EL0 用户权限**（强制降级） |
| 用户空间 PXN/PAN | 跳过检查 | 检查（违规则触发异常） |
| 典型用途 | 内核数据访问 | `copy_from_user` / `copy_to_user` |
| 安全性 | 可能绕过用户权限限制 | 确保用户权限约束 |

```asm
; 内核安全读取用户空间数据
; 普通 LDR: 内核权限，跳过用户权限检查
ldr  w0, [x1]     ; ❌ 危险：可能绕过用户页表权限

; LDTR: 强制使用 EL0 用户权限校验
ldtr w0, [x1]     ; ✅ 安全：硬件检查用户权限
```

---

## HFT 关联

这些指令对无锁编程和内核-用户态数据传递至关重要：

| 场景 | 推荐指令 | 原因 |
|------|---------|------|
| 无锁 SPSC 队列 | LDXR/STXR | 避免 mutex 上下文切换，但 SPSC 模式争用为 0 |
| 无锁 MPSC 队列 | LDXR/STXR + CAS | 多生产者需要 CAS 争用，重试成本需评估 |
| 内存序同步 | LDAR/STLR | 比显式 DMB 精确，只排序相关访问，减少不必要屏障 |
| 交易指令传入 | LDTR/STTR | 内核安全复制用户态交易数据，防止绕过权限 |
| 低争用原子 | LSE（ARMv8.1） | 比 LDXR/STXR 更快，无重试循环 |
| 高争用原子 | 每核独立数据 | 完全避免原子操作，HFT 最优方案 |

- LDXR/STXR 在争用激烈时重试成本高 → HFT 中用单生产者-单消费者模式避免争用
- LDAR/STLR 比 DMB 屏障更精确 → 减少不必要的屏障开销
- LDTR 用于内核安全地复制用户数据（如交易指令从用户态传入），防止绕过权限检查

## 自测题

1. LDXR/STXR 如何实现原子操作？
<details><summary>答案</summary>
LDXR 设置独占监视器标记该地址。STXR 尝试写回：如果监视器仍持有独占标记则写入成功（返回 0）；如果中间有其他核写了该地址则失败（返回非 0）。通过循环重试实现原子 read-modify-write。
</details>

2. LDAR 和 LDR 的区别是什么？
<details><summary>答案</summary>
LDAR = Load-Acquire，保证 LDAR 之后的读写不会重排到 LDAR 之前。LDR 是普通加载，没有内存序保证。LDAR 用于需要 acquire 语义的场景（如读取标志后访问对应数据）。
</details>

3. 为什么内核用 LDTR 而不是 LDR 来访问用户空间地址？
<details><summary>答案</summary>
LDTR 以 EL0（用户）权限执行访问，即使用户空间的页表设置了 PXN（特权不可执行）等限制，内核也能正确检测到权限违规。用普通 LDR 则以内核权限访问，可能绕过用户空间的访问限制。
</details>

4. STXR 的第一个寄存器 Rs 输出什么？0 和非 0 分别代表什么？
<details><summary>答案</summary>
Rs 是**状态码**（不是数据）：Rs = 0 表示独占写入成功（监视器仍有效）；Rs ≠ 0 表示失败（LDXR 和 STXR 之间有其他核写了该地址，监视器被打破）。需要重新 LDXR → 修改 → STXR 重试。
</details>

5. LDAR 相比普通 LDR 多了什么？
<details><summary>答案</summary>
**Acquire 内存序语义**——LDAR 之后的读写不能重排到 LDAR 前面。等价于"加载 + 内存屏障"。普通 LDR 没有内存序保证，可被 CPU 乱序执行重排。
</details>

6. EL1 内核想访问用户地址并做用户权限检查，用哪条指令？
<details><summary>答案</summary>
**LDTR / STTR**。普通 LDR 在 EL1 下以内核权限访问，跳过 EL0 用户权限检查。LDTR 强制使用 EL0 权限校验——如果用户地址非法或不可访问，直接触发异常。这是 `copy_from_user` / `copy_to_user` 的底层机制。
</details>

7. LDXR 和 STXR 之间能随便访问其他内存地址吗？为什么？
<details><summary>答案</summary>
**不能**。LDXR 和 STXR 之间对其他地址的内存访问可能打破独占监视器（Exclusive Monitor）。ARMv8 的独占监视器是粒度性的——对同一监视区域内的任何访问都可能清除独占标记，导致 STXR 无谓失败。LL-SC 临界区应尽量短，只做寄存器运算。
</details>

8. LDAR/STLR 相比 DMB 屏障有什么优势？
<details><summary>答案</summary>
LDAR/STLR 把内存屏障和 load/store **合并成单条指令**，只排序与该指令相关的访问。DMB 是全局屏障，会过度排序不相关的访问，性能开销更大。LDAR/STLR 更精确，在现代乱序执行 CPU 上性能更好。
</details>

## 参考与延伸

- 原书 §3.6
- [§3.1 LDR/STR 核心规则](01-load-store-rules.md) — 普通 Load/Store 基础
- [§3.2 寄存器宽度与扩展](02-register-width.md) — LDRB/LDRH/LDRSB 等家族
- [§3.4 STP/LDP 栈操作](04-stp-ldp.md) — 成对加载存储
- [Ch18 内存屏障](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — DMB/DSB/ISB 详解
- [§18.4 acquire/release](../../chapter-18-memory-barriers/notes/04-acquire-release.md) — LDAR/STLR 深入
- [Ch20 原子操作](../../chapter-20-atomic-operations/notes/section-0-本章完整概述.md) — LDXR/STXR/LSE 详解
- [§6.4 LDXR/STXR 预览](../../chapter-06-a64-other-instructions/notes/04-ldxr-stxr-preview.md) — 第六章中的预览
