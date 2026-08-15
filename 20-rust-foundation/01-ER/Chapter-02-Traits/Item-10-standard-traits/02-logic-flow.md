# Item 10 · 逻辑脉络

← [Item 10 目录](./README.md)

```text
Clone ← Copy（Copy 必须 Clone）
PartialEq ← Eq（Eq 是标记）
PartialEq + PartialOrd ← Ord
Eq + Hash → 必须 hash(x)==hash(y) when x==y
```

### 浮点数的鸿沟

- IEEE 754：**`NaN != NaNaN`** → 无反射性。
- 含 **`f32` / `f64`** 的类型 **不能** derive **`Eq` / `Ord`**（只能 `PartialEq` / `PartialOrd`）。

---
