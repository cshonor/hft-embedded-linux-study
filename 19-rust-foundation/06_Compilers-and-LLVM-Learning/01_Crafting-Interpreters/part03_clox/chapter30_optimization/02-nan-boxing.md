# 第 30 章 · Optimization（优化） · §30.3 NaN 装箱（NaN Boxing）

← [本章目录](./README.md) · 上一节：[01-hash-table-probe-optimization.md](./01-hash-table-probe-optimization.md) · 下一节：[03-a-map-of-the-territory.md](./03-a-map-of-the-territory.md)

---

**问题**：ch18 **`Value` struct**（tag + union + **对齐**）常占 **16 字节** / 栈槽 · 缓存行浪费。

**思路**：把 **所有 Value 塞进单个 `uint64_t` / `double` 位型** —— **NaN boxing**（LuaJIT、JavaScript 引擎常用）。

### IEEE 754 Double 位布局（概念）

```text
64 bit: [ sign | exponent | fraction/mantissa ... ]
```

- **Quiet NaN（qNaN）**：指数全 1 · 尾数非 0 → **大量 NaN  payload 位** 可 **存数据**。
- **合法 double 数** · **NaN 编码值** · **非数指针** 可 **共存于 64 位**。

### clox 编码策略（书中）

| 值种类 | 编码方式 |
|--------|----------|
| **number** | 普通 **IEEE double** 位型 |
| **`nil` / `true` / `false`** | 特定 **NaN 立即数** 模式 |
| **Obj 指针** | 利用 **48 位有效地址**（x64 常见）· 塞进 **NaN payload** |
| **Sign bit 标记** | 区分 **指针 boxed 值** vs **纯 double**（书中用符号位作 tag 手段之一） |

**结果**：

```text
Value: 8 字节（一个 uint64_t）
  栈更紧凑 · 常量池更小 · cache line 装更多 slot
  → 解释器整体再提速（书中给出显著 delta）
```

| 代价 | 说明 |
|------|------|
| **调试难度** | 位 hack · **`printValue` 需解码** |
| **可移植性** | 假设指针宽度 / NaN 语义 |
| **IS_NUMBER 等宏** | 改为 **位测试** 而非 **`type` 枚举** |

**对照 ch18 [tagged union](../chapter18_types-of-values/README.md)**：教学清晰 **→** 生产性能 **NaN tag**；同一语义两种表示。

---
