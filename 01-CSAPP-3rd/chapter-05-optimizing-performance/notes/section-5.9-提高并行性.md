## 5.9 提高并行性

> **Ch5 §5.9** · [章导读](../README.md) · 上节 [§5.8 ←](./section-5.8-循环展开.md) · 下节 [§5.10 →](./section-5.10-优化合并代码的结果小结.md)

---

#### 5.9.1 多个累积变量

```c
// 2 路展开 + 双累加器，打破 loop-carried dep
x0 = ident; x1 = ident;
for (i = 0; i < n-1; i += 2) {
    x0 = x0 + data[i];
    x1 = x1 + data[i+1];
}
acc = (x0 + x1) + ...;
```

`x0` 与 `x1` **无依赖** — CPU 可并行执行。

#### 5.9.2 重新结合变换 (Reassociation)

```c
// 结合律对浮点不严格成立！整数 + 在数学上 OK
// (a + data[i]) + data[i+1]  vs  a + (data[i] + data[i+1])
```

- 编译器在 `-ffast-math` 下对浮点重结合 — **HFT 慎用** 于价格累加
- 整数 tick 求和、checksum 可重结合

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§5.8 ←](./section-5.8-循环展开.md) · [本章导读](../README.md) · [§5.10 →](./section-5.10-优化合并代码的结果小结.md)
