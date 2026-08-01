## §12.4 二分查找 (Binary Searches)

> **Ch 12 · 表** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 问题：在表里找 key

表项不仅是数字，常是 **(key → payload)**，例如：

| key（排序） | payload |
|-------------|---------|
| 10 | "Mushroom" |
| 25 | "Pepperoni" |
| 40 | "Veggie" |

目标：给定 **target key**，找到对应字符串或确认 **不存在**。

---

### 线性 vs 二分

| | **线性搜索** | **二分搜索** |
|---|--------------|--------------|
| **前提** | 无 | **key 必须有序** |
| **比较次数** | 平均 N/2 | 最多 ⌈log₂ N⌉ |
| **表翻倍** | 时间 ×2 | 时间 **+1 次循环** |
| **汇编** | 索引 0→N-1，逐项 CMP | **lo/hi/mid** 减半区间 |

**口述：** N=1024 时，线性最坏 1024 次；二分约 **10 次**。

---

### 算法（与 Ch11 二分**求根**对照）

**离散 key（本章）：**

```
lo = 0, hi = N-1
while lo <= hi:
    mid = (lo + hi) / 2
    if table[mid].key == target:  found
    if target < table[mid].key:   hi = mid - 1
    else:                         lo = mid + 1
not found
```

**连续函数（Ch11）：** 区间 **[a,b]** 上 **f(mid)** 符号 — 同名 **减半**，对象不同（**离散 key** vs **连续 f(x)**）。

---

### 汇编技巧：mid = (lo + hi) / 2

```asm
    ADD   r2, r0, r1      ; r0=lo, r1=hi
    MOV   r2, r2, ASR #1  ; 算术右移 1 位 = 除以 2（有符号安全）
```

- **不要用** `LSR` 若 lo/hi 为有符号且可能为负（表中索引通常非负，**ASR** 仍与 C 语义一致）。  
- 取 **table[mid]**：`LDR r3, [base, r2, LSL #2]`（key 为字）。

**循环骨架：** [Ch8](../chapter-08-branches-loops/notes/section-8-3-loops.md) — `CMP` + `BLT/BGT` + `B` 回环，或 `SUBS` 计数上限。

---

### 完整流程口述

```
1. 读 target 到寄存器
2. lo=0, hi=N-1
3. 循环：mid=(lo+hi)>>1
4. LDR key[mid]，CMP target
5. 相等 → 取 payload 指针，退出
6. target 小 → hi=mid-1；大 → lo=mid+1
7. lo>hi → 未找到
```

书中 **披萨配料** 示例：key 与 **字符串指针** 同表或相邻结构 — 找到 key 后 **LDR 字符串地址** 再打印（Ch13 可 `BL printf`）。

---

### 何时不用二分

- 表 **很小**（<8）— 线性更简单  
- 表 **频繁插入** — 保持有序成本高 → 哈希或平衡树（超出本章）  
- **Flash 只读静态表** — 二分非常合适  

---

### 可复述要点

1. 二分 **前提：有序 key**。  
2. **mid = (lo+hi) ASR #1** — 汇编里除以 2 的标准写法。  
3. 表大小翻倍 → 线性慢一倍，二分只多 **一轮** — **O(log N)**。
