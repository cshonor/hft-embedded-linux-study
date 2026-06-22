## 2.4.5–2.4.6 浮点运算与 C 浮点

### 2.4.5 浮点运算

- **非结合、非分配：** `(a+b)+c ≠ a+(b+c)` — 大数 swallow 小数
- **NaN 传播：** 任何 op 含 NaN → NaN；比较无序
- **Inf 算术：** 有定义规则，仍易出 bug

```c
// 经典坑
float a = 1e30f, b = -1e30f, c = 1.0f;
(a + b) + c;  // 0 + 1 = 1
a + (b + c);  // 1e30 + (-1e30+1) 可能仍 0
```

**HFT：** 组合多个 venue 的 partial fill 算 VWAP — 用 **整数分子/分母** 或 Kahan summation；别裸 `float` 连加。

### 2.4.6 C 语言中的浮点数

- 字面量：`3.14f`（float）、`3.14`（double）、`3.14L`（long double）
- **隐式转换：** float 运算常 **提升为 double**（默认 argument promotion）
- **`printf`：** `%f` double、`%Lf` long double；**精确打印** 用 hex float `%a`
- **`-ffast-math`** — 打破 IEEE 严格语义换速度；**回测与生产是否一致** 要验证

### 2.5 小结（原书）

- **信息 = 位 + 解释上下文**
- **整数：** 补码、溢出、有/无符号转换 — 安全 bug 温床
- **浮点：** 近似值 — 金融与延迟敏感系统 **优先整数定点**

→ 下一章看 CPU 如何用这些格式做运算：[Ch 3 机器级表示](../../chapter-03-程序的机器级表示.md)

---

← [本章导读](../README.md)
