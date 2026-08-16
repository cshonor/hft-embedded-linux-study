# 3 · 替代的数据表示 (Alternative representations)

← [本章目录](./README.md) · 上一节：[02-exotic-types.md](./02-exotic-types.md)

---

| `repr` | 作用 | 注意 |
|--------|------|------|
| **`repr(C)`** | 按 C/C++ 规则排序、算 size/align | FFI、内存重释、固定二进制格式 **必用** |
| **`repr(transparent)`** | 仅含一个非 ZST 字段时，布局/ABI 与内层字段完全一致 | 新 type 包装 |
| **`repr(u*)` / `repr(i*)`** | 无字段枚举的底层整数大小与符号 | C 枚举互操作 |
| **`repr(packed)`** | 剥离 padding，整体对齐到 1 字节 | **危险**：未对齐访问，ARM 等可能硬件异常 |
| **`repr(align(n))`** | 最小对齐至少为 `n`（2 的幂） | 缓存行对齐、避免 false sharing |

→ 源码：[src/repr_alt.rs](./src/repr_alt.rs)

→ 下一章：[03 Lifetime](../03_Lifetime_Variance/README.md)
