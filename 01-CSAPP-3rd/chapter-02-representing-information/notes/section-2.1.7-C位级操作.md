## 2.1.7 C 语言中的位级操作

> **Ch2 §2.1** · [章导读](../README.md) · 上节 [§2.1.6 布尔](./section-2.1.6-布尔代数简介.md) · 下节 [§2.1.8 逻辑](./section-2.1.8-C逻辑操作.md)

---

| 运算符 | 含义 | 典型用途 |
|--------|------|----------|
| `&` | 按位与 | **掩码** 取字段 |
| `\|` | 按位或 | **合并** 标志位 |
| `^` | 按位异或 | 切换位、简单 checksum |
| `~` | 按位取反 | 掩码求补 |
| `<<` `>>` | 左/右移 | 乘除 2 的幂、字段抽取（详 → [§2.1.9](./section-2.1.9-C移位操作.md)） |

```c
/* 取第 k 位（从 0 起） */
int get_bit(unsigned x, int k) { return (x >> k) & 1; }

/* 第 k 位置 1 */
unsigned set_bit(unsigned x, int k) { return x | (1u << k); }

/* 协议常见：从 word 中抠 bit 域 */
uint32_t field = (word >> shift) & mask;
```

**HFT：** SBE / 自定义 header 解码 — **shift + mask** 是热路径基本动作；保证 `mask` 与 `shift` 来自 schema，别手写魔数散落各处。

---

### 口述巩固 · 自测

1. 解码 16 位 header 里的 5-bit 字段，典型步骤是什么？（shift + mask）

---

← [本章导读](../README.md) · [§2.1.6 ←](./section-2.1.6-布尔代数简介.md) · [§2.1.8 →](./section-2.1.8-C逻辑操作.md)
