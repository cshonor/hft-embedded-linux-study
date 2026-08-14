# 第 20 章 · Hash Tables（哈希表） · 哈希函数与负载因子（Hash Functions and Load Factor）

← [本章目录](./README.md) · 上一节：[01-open-addressing-and-linear-probing.md](./01-open-addressing-and-linear-probing.md) · 下一节：[03-string-interning.md](./03-string-interning.md)

---

| 项 | 选择 |
|----|------|
| **哈希** | **FNV-1a**（字符串 → **uint32_t**） |
| **负载因子** | **entries / capacity > 0.75** → **grow** |
| **扩容** | 更大数组 · **所有 Entry 重新插入（rehash）** |

```text
count / capacity > 0.75
  → capacity *= 2
  → 新表 rehash 所有 live entries
```

**均摊 O(1)**：单次扩容贵，插入均摊便宜。

**键类型**：本章以 **`ObjString*`** 为键（已含 **预计算 hash**）。

---
