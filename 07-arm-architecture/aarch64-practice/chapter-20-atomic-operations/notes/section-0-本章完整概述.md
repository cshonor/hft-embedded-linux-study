# Ch20 完整总结 · 原子操作

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md) · [Pi5 适配](../../PI5-ADAPT.md)

---

## 本章定位

原子操作是多核同步的基石。本章讲 LDXR/STXR 独占监视器、CAS（Compare-And-Swap）、WFE 低功耗自旋锁。

> **HFT 关联**：无锁数据结构（ring buffer、订单簿）核心原语。

---

## 20.1 独占监视器（Exclusive Monitor）⭐

ARM 用 **LDXR + STXR** 配对实现原子操作：

| 指令 | 行为 |
|------|------|
| `LDXR Wd/Xd, [Xn]` | 独占加载：标记"我正在独占访问这个地址" |
| `STXR Ws, Wd/Xd, [Xn]` | 独占存储：如果监视器仍有效→写入成功(Ws=0)；否则失败(Ws=1) |
| `CLREX` | 清除本地独占监视器（放弃独占） |

```
CPU0                          CPU1
ldxr x0, [addr]               
                              ldxr x1, [addr]  ← 也标记独占
                              stxr w2, x1, [addr]  ← 成功(w2=0)，CPU0的监视器被清除
stxr w2, x0, [addr]           
← 失败(w2=1)，必须重试
```

> 独占监视器是**缓存行级别**的：监视一个 Cache Line，如果其他核写了该行→监视器被清除。

---

## 20.2 原子操作实现模式 ⭐

### 原子加法

```asm
; atomic_add(addr, val)
; x0 = addr, x1 = val
atomic_add:
1:  ldxr x2, [x0]       ; 独占读
    add  x2, x2, x1     ; 修改
    stxr w3, x2, [x0]   ; 独占写
    cbnz w3, 1b         ; 失败则重试
    ret
```

### CAS（Compare-And-Swap）

```asm
; x0 = addr, x1 = expected, x2 = desired
; 返回 x0: 旧值（=expected 表示成功）
cas:
1:  ldxr x3, [x0]       ; 独占读当前值
    cmp  x3, x1         ; 和期望值比较
    b.ne 2f             ; 不等 → 返回旧值
    stxr w4, x2, [x0]   ; 相等 → 写入新值
    cbnz w4, 1b         ; 写入失败 → 重试
2:  mov  x0, x3         ; 返回旧值
    ret
```

### C++11 atomic 对应

| C++ 操作 | ARM 实现 |
|----------|----------|
| `atomic.load()` | `LDR`（配屏障）或 `LDAR` |
| `atomic.store()` | `STR`（配屏障）或 `STLR` |
| `atomic.fetch_add()` | `LDXR + ADD + STXR` 循环 |
| `atomic.compare_exchange()` | `LDXR + CMP + STXR` 循环 |
| `atomic.exchange()` | `LDXR + STXR` 循环 |

---

## 20.3 ARMv8.1 LSE（Large System Extensions）⭐

ARMv8.1 引入了**单指令原子操作**（LSE），不再需要 LDXR/STXR 循环：

| 指令 | 行为 |
|------|------|
| `LDADD Ws, Wd, [Xn]` | 原子加法：`Wd = [Xn]; [Xn] += Ws` |
| `CAS Ws, Wt, [Xn]` | 原子 CAS：如果 `[Xn]==Ws` 则 `[Xn]=Wt` |
| `SWP Ws, Wd, [Xn]` | 原子交换：`Wd = [Xn]; [Xn] = Ws` |
| `LDSET Ws, Wd, [Xn]` | 原子置位：`Wd = [Xn]; [Xn] \|= Ws` |
| `LDCLR Ws, Wd, [Xn]` | 原子清位：`Wd = [Xn]; [Xn] &= ~Ws` |

> LSE 比 LDXR/STXR 循环**更快**（无重试开销），在多核高竞争场景尤其明显。  
> Linux 通过 `__LSE_ATOMIC` 宏在编译期选择。Pi5 Cortex-A76 支持 LSE。

---

## 20.4 WFE / SEV —— 低功耗自旋锁 ⭐

| 指令 | 行为 |
|------|------|
| `WFE` | Wait For Event：进入低功耗，等事件唤醒 |
| `SEV` | Send Event：唤醒所有等 WFE 的 CPU |
| `SEVL` | Send Event Local：只唤醒本核（用于 WFE 前预发） |

### WFE 自旋锁

```asm
; 低功耗自旋锁
spin_lock_wfe:
1:  ldxr w1, [x0]       ; 读锁
    cbnz w1, 2f          ; 锁被占 → 等待
    stxr w2, w1, [x0]    ; 尝试获取（w1=1）
    cbnz w2, 1b          ; STXR 失败 → 重试
    ret                   ; 获取成功

2:  wfe                   ; 低功耗等待
    b 1b                  ; 被唤醒后重试

spin_unlock_wfe:
    stlr w1, [x0]        ; 释放锁（STLR 自带 release 屏障）
    sev                   ; 唤醒等待的 CPU
    ret
```

> **WFE 优势**：自旋时不用 100% 占 CPU，降低功耗。  
> **WFE 陷阱**：WFE 可能被其他事件（中断、调试）误唤醒，醒来后要重新检查条件。

---

## 20.5 Linux 原子操作 API

| API | 实现 | 说明 |
|-----|------|------|
| `atomic_read(v)` | `READ_ONCE` | 原子读 |
| `atomic_set(v, i)` | `WRITE_ONCE` | 原子写 |
| `atomic_add(i, v)` | `stxr` 循环 / `LDADD` | 原子加 |
| `atomic_add_return(i, v)` | `LDAXR/STLXR` 循环 | 原子加并返回新值 |
| `atomic_cmpxchg(v, old, new)` | `LDXR/CMP/STXR` | CAS |
| `atomic_inc(v)` | `atomic_add(1, v)` | 原子自增 |
| `smp_mb__after_atomic()` | `dmb ish` | 原子操作后加屏障 |

---

## 20.6 实验要点

本章以案例分析为主。关键案例：独占监视器工作原理、CAS 实现、WFE 自旋锁。

---

## 20.7 易错点清单

1. **LDXR 后不做 STXR** → 监视器残留 → 后续 LDXR 异常（Ch7 案例）。
2. **CAS 循环太长** → 高竞争时活锁（livelock），考虑退避策略。
3. **WFE 被误唤醒** → 醒来后必须重新检查条件，不能假设锁已释放。
4. **忘加屏障** → 原子操作只保证读-改-写原子，不保证和其他访存的顺序。
5. **用 `atomic_read` 就以为安全** → `atomic_read` 只保证编译器不优化，不保证多核可见性，需要屏障。

---

## 书中思考题（自测）

1. LDXR/STXR 如何实现原子操作？STXR 返回 1 代表什么？
2. 写一个原子加法的汇编代码。
3. ARMv8.1 LSE 有什么优势？为什么比 LDXR/STXR 循环快？
4. WFE 自旋锁比普通自旋锁有什么优势？
5. 原子操作是否自带内存屏障？需要额外加吗？

**参考答案：**

1. LDXR 标记独占监视器；STXR 检查是否被干扰。返回 **1 = 失败**（需重试）。  
2. 见 20.2 节代码：`ldxr` → `add` → `stxr` → `cbnz` 重试。  
3. LSE 单指令完成（如 `LDADD`），**无循环重试开销**；多核高竞争时优势明显。  
4. WFE 自旋时**低功耗等待**，不 100% 占 CPU，降低功耗和总线竞争。  
5. **不带屏障**（LDXR/STXR 不保证顺序）。需要额外加 DMB 或用 LDAXR/STLXR（带 acquire/release）。

---

上一章 [Ch19 屏障使用](../../chapter-19-barrier-usage/) · 下一章 [Ch21 OS话题](../../chapter-21-os-topics/) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)
