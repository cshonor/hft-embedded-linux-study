## 9.9.5-9.9.6 实现问题与隐式空闲链表

> **Ch9 §9.9.5-9.9.6** · [章导读](../README.md) · 上节 [§9.9.4 ←](./section-9.9.4-碎片.md) · 下节 [§9.9.7-9.9.9 →](./section-9.9.7-9.9.9-放置、分割、扩展堆.md)

---

← [本章导读](../README.md)

---

### 隐式空闲链表

- **块格式：** `[header(4-8B)] [payload] [padding] [footer(4-8B)]`
- **header/footer 存：** 块大小 + 分配位（1=已分配，0=空闲）
- **隐式：** 所有块（已分配+空闲）通过 size 隐含链接，遍历时用 `当前地址 + size` 跳到下一块
- **操作：**
  - `find_fit` — 从头遍历找够大的空闲块
  - `split` — 空闲块大于需求时切分
  - `coalesce` — free 时合并相邻空闲块

```
堆布局：[prologue][block1][block2]...[epilogue]
        ← header ← payload → ← footer →
```

**问题：** 遍历所有块（含已分配）→ O(n)，n = 总块数。

### 常见陷阱
1. **隐式链表遍历所有块（含已分配），慢** — 查找空闲块要跳过所有已分配块，O(n)
2. **header/footer 占额外空间（8-16B/块）** — 小块时开销比例大（16B 块 → 50%+ 开销）
3. **对齐要求决定最小块大小** — 16B 对齐 → header(4)+payload(1)+pad+footer(4) → 最小 16B

### 自测题

<details>
<summary>Q1: 隐式空闲链表的「隐式」是什么意思？</summary>

不需要显式指针链接空闲块。所有块（已分配+空闲）按地址连续排列，通过 header 中的 size 字段隐含「下一块」的位置。

</details>

<details>
<summary>Q2: header 和 footer 各存什么？为什么需要 footer？</summary>

都存块大小+分配位。footer 用于从当前块地址反向定位前一个块（coalesce 时判断前块是否空闲）。

</details>

<details>
<summary>Q3: 隐式链表查找空闲块的时间复杂度？为什么慢？</summary>

O(n)，n=总块数（含已分配）。因为要遍历所有块跳过已分配的，即使大部分已分配也要逐个检查。

</details>

<details>
<summary>Q4: 如果 16 字节对齐，header 4B + footer 4B，最小块多大？</summary>

至少 16B：header(4) + payload(至少 1B) + padding(7B) + footer(4B) → 向上对齐到 16B。这意味着分配 1B 实际占 16B。

</details>

---

← [§9.9.4 ←](./section-9.9.4-碎片.md) · [本章导读](../README.md) · [§9.9.7-9.9.9 →](./section-9.9.7-9.9.9-放置、分割、扩展堆.md)
