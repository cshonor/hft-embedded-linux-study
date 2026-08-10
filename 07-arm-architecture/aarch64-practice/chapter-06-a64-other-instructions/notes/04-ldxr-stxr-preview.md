# 6.4 LDXR / STXR 独占访问（预览）

> 来源：§6.4 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

LDXR/STXR 独占加载/存储指令的预览——通过独占监视器实现无锁原子操作（read-modify-write）的基础。详细内容在 Ch20 原子操作。

## 核心要点

### 指令格式

```asm
; LDXR：独占加载（Load Exclusive Register）
LDXR Wd, [Xn]          ; 32位独占加载
LDXR Xd, [Xn]          ; 64位独占加载

; STXR：独占存储（Store Exclusive Register）
STXR Ws, Wd, [Xn]      ; 32位独占存储，Ws=状态(0=成功)
STXR Ws, Xd, [Xn]      ; 64位独占存储，Ws=状态
```

### 原子 read-modify-write 模式

```asm
; 原子自增 [x1] 处的值
retry:
    LDXR W0, [X1]       ; 1. 独占加载：W0 = *X1，设置监视器
    ADD  W0, W0, #1     ; 2. 修改：W0++
    STXR W2, W0, [X1]   ; 3. 独占存储：尝试写回，W2=0成功
    CBNZ W2, retry      ; 4. 失败则重试
```

### 独占监视器原理

```
核 A                                核 B
────                                ────
LDXR W0, [X1]    ← 设置监视器
                  标记地址 [X1]
                                    STR W5, [X1]  ← 写同一地址
                                                  → 监视器被清除！
ADD W0, W0, #1
STXR W2, W0, [X1]
→ 监视器已失效
→ W2 = 1（失败）
→ 需要重试

重试流程：
LDXR W0, [X1]    ← 重新加载（读到核B写的新值）
ADD W0, W0, #1
STXR W2, W0, [X1]
→ 监视器有效（没有其他核干扰）
→ W2 = 0（成功）✓
```

### STXR 返回值

| Ws 值 | 含义 | 处理 |
|-------|------|------|
| 0 | 独占写入成功 | 继续后续操作 |
| 非0 | 独占写入失败 | CBNZ 重试 |

### 独占访问的大小

```asm
; 不同的访问大小
LDXRB Wd, [Xn]        ; 1 字节独占加载
LDXRH Wd, [Xn]        ; 2 字节独占加载
LDXR  Wd, [Xn]        ; 4 字节独占加载
LDXR  Xd, [Xn]        ; 8 字节独占加载

; 对应的存储
STXRB Ws, Wd, [Xn]    ; 1 字节独占存储
STXRH Ws, Wd, [Xn]    ; 2 字节独占存储
STXR  Ws, Wd, [Xn]    ; 4 字节独占存储
STXR  Ws, Xd, [Xn]    ; 8 字节独占存储
```

### 自旋锁实现

```asm
; 经典自旋锁（spinlock）
; x0 = lock 变量地址
spin_lock:
    MOV W1, #1           ; 锁值=1（已锁）
    MOV W2, #0           ; 期望值=0（未锁）
retry:
    LDXR W3, [X0]        ; 独占读锁值
    CMP  W3, W2          ; 锁是否为0（未锁）？
    BNE  retry           ; 非0（已锁）→ 重试
    STXR W4, W1, [X0]    ; 尝试写入1（加锁）
    CBNZ W4, retry       ; 写入失败 → 重试
    RET                   ; 成功获得锁

spin_unlock:
    STLR W2, [X0]        ; 写0（释放锁），STLR 带 release 语义
    RET
```

### 原子 CAS（Compare-And-Swap）

```asm
; CAS: 如果 *addr == expected，则 *addr = new_val
; x0=addr, x1=expected, x2=new_val → x0=旧值
cas:
    LDXR X3, [X0]        ; 独占读
    CMP  X3, X1          ; 比较
    BNE  cas_fail        ; 不等 → 返回旧值
    STXR W4, X2, [X0]    ; 尝试写新值
    CBNZ W4, cas         ; 失败 → 重试
    MOV  X0, X3          ; 返回旧值
    RET
cas_fail:
    MOV  X0, X3          ; 返回旧值（比较失败）
    RET
```

### ARMv8.1 LSE 原子指令

```
ARMv8.1 引入了单条原子指令（LSE），不需要 LDXR/STXR 循环：

  LDADD  Xd, Xn, [Xs]    ; 原子加：Xn = *Xs + Xd（原子）
  LDCLR  Xd, Xn, [Xs]    ; 原子清零
  CAS    Xd, Xn, [Xs]    ; 原子比较交换
  SWP    Xd, Xn, [Xs]    ; 原子交换

优势：无重试循环，低争用时更快
劣势：高争用时仍需等待（但不会活锁）
```

## LDXR/STXR vs LSE 对比

| 特性 | LDXR/STXR | LSE (LDADD等) |
|------|-----------|---------------|
| 指令数 | 3-4 条（循环） | 1 条 |
| 低争用延迟 | 较高（3+周期） | 较低（1-2周期） |
| 高争用行为 | 可能活锁 | 公平排队 |
| 硬件要求 | ARMv8.0+ | ARMv8.1+ |
| 适用场景 | 兼容性好 | 性能更好 |

## 常见错误

1. **不检查 STXR 返回值**：以为写入一定成功，实际可能失败。
2. **LDXR 和 STXR 之间做太多操作**：增加了被其他核干扰的窗口，提高失败率。
3. **忘记循环重试**：单次 LDXR+STXR 不保证原子性，必须循环直到成功。

## HFT 关联

LDXR/STXR 是无锁编程的基础：
- 实现无锁 ring buffer → 单生产者-单消费者模式避免争用
- 自旋锁用 LDXR/STXR 实现 → 但争用激烈时重试成本高
- HFT 中更推荐单线程模式（每个核独占数据）→ 完全避免原子操作
- ARMv8.1 LSE（LSE 原子指令）比 LDXR/STXR 在低争用时更快

```asm
; HFT：无锁 SPSC ring buffer（单生产者单消费者）
; x0 = ring buffer 结构体地址
; 生产者入队：
push:
    ADRP x1, producer_idx
    LDR  x2, [x1, :lo12:producer_idx]   ; 读生产者索引
    ; ... 计算槽位地址 ...
    STR  x3, [x0, x2, LSL #3]            ; 写数据
    ADD  x2, x2, #1
    STR  x2, [x1, :lo12:producer_idx]    ; 更新索引
    ; SPSC 不需要原子操作（单写者），只需屏障保证可见性
    DMB ISH
```

## 自测题

1. LDXR/STXR 如何保证原子性？
<details><summary>答案</summary>
LDXR 在地址上设置独占监视器。STXR 检查监视器是否仍有效（没有其他核写该地址）。有效则写入成功(返回0)，无效则失败(返回非0)。通过循环重试直到成功，保证 read-modify-write 的原子性。
</details>

2. STXR 返回非 0 意味着什么？应该怎么处理？
<details><summary>答案</summary>
返回非 0 表示独占写入失败（有其他核在 LDXR 和 STXR 之间写了该地址，监视器被清除）。需要重新 LDXR → 修改 → STXR 重试。通常用 CBNZ 判断并跳回重试。
</details>

3. LDXR/STXR 在高争用场景下有什么问题？
<details><summary>答案</summary>
高争用下 STXR 频繁失败，导致大量重试循环 → 活锁风险（live-lock）、CPU 浪费、延迟不可预测。HFT 中应避免高争用原子操作，改用每核独立数据结构或单生产者-单消费者模式。
</details>

4. 为什么 LDXR 和 STXR 之间的操作应尽量少？
<details><summary>答案</summary>
LDXR 和 STXR 之间的窗口越长，其他核写入同一地址的概率越高 → STXR 失败率上升 → 重试次数增加 → 延迟不可预测。最佳实践是在 LDXR 后立即做最简单的修改，然后 STXR。复杂逻辑应移到循环外。
</details>

5. ARMv8.1 的 LDADD 相比 LDXR+ADD+STXR 有什么优势？
<details><summary>答案</summary>
1. 单条指令完成原子加，不需要循环重试 → 低争用时延迟更低
2. 不会活锁（硬件保证最终成功）
3. 代码更紧凑（1条 vs 4条）
4. 但需要 ARMv8.1+ 硬件支持，旧 CPU 不兼容。Linux 内核会检测 LSE 支持并选择使用哪种实现。
</details>

## 参考与延伸

- 原书 §6.4
- [3.6 特殊访存](../../chapter-03-a64-load-store/notes/section-0-本章完整概述.md)
- [Ch20 原子操作详解](../../chapter-20-atomic-operations/notes/section-0-本章完整概述.md)
