# 6.5 DMB / DSB / ISB 内存屏障（预览）

> 来源：§6.5 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

DMB/DSB/ISB 三条内存屏障指令的预览——控制内存访问和指令执行的顺序。详细内容在 Ch18-19。

## 核心要点

### 三条屏障指令

| 指令 | 全称 | 作用 | 强度 | 性能代价 |
|------|------|------|------|----------|
| DMB | Data Memory Barrier | 排序 DMB 前后的访存，不等待完成 | 中 | ~数cycle |
| DSB | Data Synchronization Barrier | 排序 + 等待所有访存完成 | 强 | ~数十cycle |
| ISB | Instruction Synchronization Barrier | 冲刷流水线，重新取指 | 最强 | ~数十cycle |

### 屏障的作用域

```
DMB/DSB 的共享域（domain）参数：

  SY  (Full System)     — 所有核、所有设备都可见
  ISH (Inner Shareable) — 内部共享域（通常同一集群的核）
  OSH (Outer Shareable) — 外部共享域
  NSH (Non-shareable)   — 不可共享（仅当前核）
  ST  (Store)           — 仅存储操作

示例：
  DMB ISH   — 内部共享域屏障（最常用）
  DMB SY    — 全系统屏障（最强，最慢）
  DSB ISHST — 内部共享域存储同步屏障
```

### DMB 详解

```asm
; DMB：保证前后的访存操作有序，但不等待
; CPU 可以继续执行后续非访存指令

STR X0, [X1]           ; 写数据
DMB ISH                ; 屏障：保证上面的写在下面的写之前完成
STR X2, [X3]           ; 写标志

; DMB 不等待 → 下一条指令可以立即执行（如果它不访存）
; 但任何访存操作不会被重排到 DMB 之前
```

### DSB 详解

```asm
; DSB：排序 + 等待所有之前的访存完成
; CPU 必须等待 DSB 之前的所有访存操作完成后才执行 DSB 之后的指令

; 典型场景：修改页表后
STR X0, [page_table]   ; 写页表项
DSB ISH                 ; 等待页表写入完成
TLBI VAAE1IS, X1        ; 刷新 TLB（必须确保页表已写入）
DSB ISH                 ; 等待 TLB 刷新完成

; DMA 场景：确保数据写入内存后再启动 DMA
STR X0, [buffer]        ; 写数据到 DMA 缓冲区
DSB SY                  ; 等待数据写入完成（对设备可见）
; 启动 DMA
```

### ISB 详解

```asm
; ISB：冲刷流水线，重新取指
; 用于：修改代码后、修改系统寄存器后、切换执行状态后

; 1. 修改系统寄存器后
MSR SCTLR_EL1, X0      ; 开启/关闭 MMU
ISB                     ; 冲刷流水线，后续指令用新配置取指

; 2. 修改代码后（自修改代码）
STR X0, [code_addr]    ; 写入新指令
DC CVAU, code_addr     ; 清 D-cache
DSB ISH                 ; 等待 D-cache 清除完成
IC IVAU, code_addr     ; 清 I-cache
DSB ISH                 ; 等待 I-cache 清除完成
ISB                     ; 冲刷流水线，重新取新指令

; 3. 修改异常向量表后
MSR VBAR_EL1, X0       ; 设置新向量表地址
ISB                     ; 确保后续异常用新向量表
```

### DMB vs DSB vs ISB 对比

```
时间线 →

DMB ISH:
  Store A ──┐
            ├─ DMB ──┐
  Store B ──┘        ├─ Store C ──┐
                     │             │
  (CPU 不等待，可以执行非访存指令)
  (但 Store C 不会排到 Store A/B 前面)

DSB ISH:
  Store A ──┐
            ├─ DSB ────── 等待 ──────┐
  Store B ──┘                        ├─ Store C
  (CPU 必须等待 A/B 完成后才继续)

ISB:
  指令A ──┐
          ├─ ISB ── [冲刷流水线] ──┐
  指令B ──┘                        ├─ 指令C（重新取指）
  (流水线中所有指令被丢弃，C 重新从 I-cache/内存取)
```

### LDAR / STLR（Acquire/Release）

```asm
; ARMv8 提供了隐含屏障语义的加载/存储指令
; 比显式 DMB 更精确，性能更好

LDAR X0, [X1]          ; Load-Acquire：后续访存不能重排到 LDAR 之前
STLR X0, [X1]          ; Store-Release：前面的访存不能重排到 STLR 之后

; 对比 DMB 方式
; 旧方式（DMB）
STR X0, [data]
DMB ISH
STR X1, [flag]

; 新方式（STLR）
STR X0, [data]
STLR X1, [flag]         ; STLR 自动保证 data 的写在 flag 之前
```

## 与 C 的对照

```c
// C11 <stdatomic.h> 映射到 ARM 屏障指令
#include <stdatomic.h>

// atomic_store_release → STLR
atomic_store_explicit(&flag, 1, memory_order_release);
// 编译为: stlr w0, [flag]

// atomic_load_acquire → LDAR
int val = atomic_load_explicit(&data, memory_order_acquire);
// 编译为: ldar w0, [data]

// atomic_thread_fence(seq_cst) → DMB ISH
atomic_thread_fence(memory_order_seq_cst);
// 编译为: dmb ish
```

## 常见错误

1. **DMB 放错位置**：放在所有写之后 → 没有排序效果。应放在需要排序的两次访存之间。
2. **该用 DSB 时用了 DMB**：DMB 不等待完成，DMA/页表场景可能数据未写入。
3. **修改系统寄存器后不加 ISB**：流水线中残留旧指令，行为不可预测。

## HFT 关联

内存屏障是弱序内存模型下的正确性保障，但有性能代价：
- DMB/DSB 阻止 CPU 乱序优化 → 减少并行度，增加延迟
- HFT 无锁队列需要在正确位置放屏障 → 太少导致数据竞争，太多损害性能
- LDAR/STLR（acquire/release）比显式 DMB 更精确 → 只排序相关访问
- HFT 优先用单线程模型 → 完全不需要屏障

```c
// HFT：无锁 SPSC 队列的屏障使用
void push(ring_buffer_t *rb, void *data) {
    rb->buffer[rb->producer_idx] = data;
    __asm__ __volatile__("dmb ish" ::: "memory");  // 确保数据先于索引可见
    rb->producer_idx++;
}

void *pop(ring_buffer_t *rb) {
    __asm__ __volatile__("dmb ish" ::: "memory");  // 确保读到最新索引
    void *data = rb->buffer[rb->consumer_idx];
    if (data == NULL) return NULL;
    rb->buffer[rb->consumer_idx] = NULL;
    rb->consumer_idx++;
    return data;
}
```

## 自测题

1. DMB 和 DSB 的区别？
<details><summary>答案</summary>
DMB 保证其前后的访存操作有序，但不等待操作完成——CPU 可以继续执行后续指令。DSB 不仅有 DMB 的排序功能，还等待所有之前的访存操作完成才继续——更强的同步点。
</details>

2. 什么时候必须用 ISB？
<details><summary>答案</summary>
1. 修改系统寄存器后（如开 MMU 后冲刷流水线中的旧指令）
2. 修改代码后（自修改代码，确保取到新指令）
3. 刷新 I-cache 后
4. 切换执行状态后（如 AArch32↔AArch64）
ISB 冲刷流水线，保证后续指令重新取指。
</details>

3. 为什么 HFT 要尽量减少屏障指令？
<details><summary>答案</summary>
屏障指令阻止 CPU 乱序执行和访存合并优化，降低 IPC 和并行度。每条 DMB/DSB 可能增加数个周期的延迟。HFT 热路径中多余的屏障会累积延迟。用 LDAR/STLR 精确替代、用单线程避免共享、用 per-CPU 数据避免屏障。
</details>

4. STLR（Store-Release）相比 `STR + DMB` 有什么优势？
<details><summary>答案</summary>
1. 单条指令实现 release 语义，不需要额外的 DMB
2. 屏障范围更精确——只排序相关地址的访问，不阻塞无关访存
3. 硬件可以优化 STLR 的实现（如只在该地址上排序，而非全局）
4. 代码更简洁，编译器和 CPU 都更容易优化
</details>

5. 以下屏障使用有什么问题？
```asm
str x0, [data]
str x1, [flag]
dmb ish
```
<details><summary>答案</summary>
屏障放错位置。在弱序内存模型下，CPU 可能把 flag 的写重排到 data 之前。屏障放在最后无法阻止这种重排。正确做法是放在两次写之间：
```asm
str x0, [data]
dmb ish
str x1, [flag]
```
或者用 STLR 替代 str [flag]：
```asm
str x0, [data]
stlr x1, [flag]   ; 自动带 release 语义
```
</details>

## 参考与延伸

- 原书 §6.5
- [Ch18 内存屏障详解](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md)
- [Ch19 屏障使用案例](../../chapter-19-barrier-usage/notes/section-0-本章完整概述.md)
