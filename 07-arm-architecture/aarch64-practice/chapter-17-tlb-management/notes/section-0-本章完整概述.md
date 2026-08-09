# Ch17 完整总结 · TLB 管理

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · **选读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md) · [Pi5 适配](../../PI5-ADAPT.md)

---

## 本章定位

TLB（Translation Lookaside Buffer）缓存页表项，减少地址翻译开销。本章讲 ASID、TLB 刷新指令、内核 TLB 维护、BBM 机制。选读。

---

## 17.1 TLB 基本概念

| 概念 | 含义 |
|------|------|
| **TLB** | 页表项的 Cache，缓存 VA→PA 映射 |
| **TLB Miss** | TLB 未命中 → MMU 遍历页表（Page Walk）→ 填充 TLB |
| **TLB Hit** | TLB 命中 → 直接得到 PA，无需 walk |

```
CPU 发出 VA
  → TLB 查找
  → 命中：直接得 PA
  → 未命中：MMU walk 4 级页表（慢！4 次内存访问）
  → 填充 TLB
```

> TLB miss 代价：48 位 VA 4 级页表 = 最多 4 次内存读。  
> 大页（2MB/1GB）可减少 TLB 条目数，提高命中率。

---

## 17.2 ASID（Address Space ID）⭐

不同进程的页表不同，但 VA 相同。ASID 区分不同进程的 TLB 条目。

| 特性 | 说明 |
|------|------|
| ASID 宽度 | 通常 8 位或 16 位（TCR_EL1.AS） |
| 作用 | TLB 条目带 ASID 标签，切换进程不刷全部 TLB |
| TCR_EL1.AS | ASID 宽度选择（0=8bit, 1=16bit） |
| TTBRx_EL1 | 高位存放 ASID |

```asm
// 设置 ASID（进程切换时）
msr TTBR0_EL1, x0      // x0 高位包含 ASID
isb

// 带 ASID 的 TLB 刷新（只刷当前 ASID）
tlbi aside1, x0        // x0 = ASID
```

> **没有 ASID**：每次进程切换都要 flush 全部 TLB → 性能差。  
> **有 ASID**：切换进程时只换 ASID，旧进程的 TLB 条目仍在（下次切回来命中）。

---

## 17.3 TLB 刷新指令 ⭐

| 指令 | 作用 |
|------|------|
| `TLBI alle1` | 刷 EL1 的所有 TLB（所有 ASID） |
| `TLBI aside1, x0` | 刷指定 ASID 的 TLB |
| `TLBI vae1, x0` | 刷指定 VA 的 TLB 条目 |
| `TLBI alle1is` | 刷所有核的 EL1 TLB（Inner Shareable） |
| `TLBI vmalle1` | 刷 EL1 全部（VA+ASID） |

```asm
// 修改页表后刷新 TLB
// 方式1：全刷（简单但慢）
tlbi alle1
dsb sy
isb

// 方式2：只刷指定 VA（精确）
ldr x0, =faulty_va
tlbi vae1, x0
dsb sy
isb
```

> **修改页表后必须刷 TLB**：否则 CPU 用旧的 TLB 条目翻译 → 访问错误地址。  
> **DSB + ISB** 必须跟在 TLBI 后面：DSB 等刷完成，ISB 确保后续指令用新映射。

---

## 17.4 BBM（Break-Before-Make）⭐

修改页表项时，如果从有效→有效（改映射），必须遵循 BBM：

```
1. 将页表项设为 Invalid（break）
2. TLB 刷新（确保旧映射失效）
3. DSB（等待 TLB 刷新完成）
4. 写入新的有效映射（make）
5. TLB 刷新（可选，确保新映射可见）
6. DSB + ISB
```

> 不遵循 BBM → 在 break 和 make 之间，其他核可能用旧 TLB → 访问已释放的物理页 → 数据损坏。  
> 内核的 `set_pte()` 通常封装了 BBM 逻辑。

---

## 17.5 内核 TLB 维护场景

| 场景 | 操作 |
|------|------|
| 进程切换（无 ASID） | `TLBI alle1` 全刷 |
| 进程切换（有 ASID） | 换 ASID，不刷 |
| munmap | `TLBI vae1` 刷指定 VA |
| fork → COW | 刷新被修改的页 |
| kmap/kunmap | `TLBI vae1` 或 `TLBI alle1` |
| 模块加载 | 无需（代码在内核空间，TLB 命中） |

---

## 17.6 实验要点

本章以案例分析为主，无独立编号实验。关键案例：Linux 内核 TLB 维护、ASID 切换、BBM。

---

## 17.7 易错点清单

1. **改页表不刷 TLB** → CPU 用旧映射，访问错误物理页。
2. **TLBI 后不跟 DSB/ISB** → 刷新未完成就继续执行，行为未定义。
3. **不用 ASID 全刷 TLB** → 进程切换性能差（TLB cold miss 暴增）。
4. **不遵循 BBM** → 多核下可能同时看到新旧映射 → 数据损坏。

---

## 书中思考题（自测）

1. TLB 的作用是什么？TLB miss 的代价有多大？
2. ASID 解决什么问题？有了 ASID 后进程切换还需要刷全部 TLB 吗？
3. 修改页表后为什么要刷 TLB？TLBI 后必须跟什么指令？
4. BBM 是什么？不遵循会有什么后果？
5. `TLBI alle1` 和 `TLBI aside1` 的区别？

**参考答案：**

1. 缓存 VA→PA 映射。TLB miss → 4 级页表 walk = **最多 4 次内存读**。  
2. ASID 区分不同进程的 TLB 条目。有 ASID 后进程切换**只需换 ASID**，不需全刷。  
3. 旧 TLB 条目还在 → 用旧映射。TLBI 后必须跟 **DSB（等完成）+ ISB（重取指）**。  
4. Break-Before-Make：先失效旧映射→刷TLB→再写新映射。不遵循→**多核下新旧映射同时可见→数据损坏**。  
5. `alle1`=刷**所有 ASID**的 EL1 TLB；`aside1`=只刷**指定 ASID**。

---

上一章 [Ch16 缓存一致性](../../chapter-16-cache-coherency/) · 下一章 [Ch18 内存屏障](../../chapter-18-memory-barriers/) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)
