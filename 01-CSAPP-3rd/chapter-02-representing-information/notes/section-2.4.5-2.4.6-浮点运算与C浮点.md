## 2.4.5–2.4.6 浮点运算与 C 浮点

> **本节一条线：** 浮点运算是 **近似算术** → 非结合 → HFT 账本/热路径优先整数。

---

### 2.4.5 浮点运算

- **非结合、非分配：** `(a+b)+c` 可能 ≠ `a+(b+c)` — **大数 swallow 小数**
- **NaN 传播：** 任一操作数为 NaN → 结果为 NaN；比较为 **无序**
- **Inf** 有定义规则，仍易出 bug

```c
float a = 1e30f, b = -1e30f, c = 1.0f;
(a + b) + c;   /* 0 + 1 = 1 */
a + (b + c);   /* 可能仍 ≈ 0 — 非结合 */
```

**HFT：** 多 venue partial fill 算 VWAP — 用 **整数分子/分母** 或 Kahan summation；别裸 `float` 连加 PnL。

---

### 2.4.6 C 语言中的浮点数

- 字面量：`3.14f`（float）、`3.14`（double）、`3.14L`（long double）
- **默认 argument promotion：** `float` 传参常 **提升为 double**
- **`printf`：** `%f` / `%a`（hex float，调试精确 bit 时用）
- **`-ffast-math`** — 打破严格 IEEE 换速度；**回测 vs 生产** 是否一致要验证

---

### 2.5 小结（原书）

| 主题 | 要点 |
|------|------|
| 信息 | **位 + 解释上下文**（同 bit 可以是 int/float/指令） |
| 整数 | 补码、有/无符号转换、溢出 — 安全 bug 温床 |
| 浮点 | **近似值** — 金融与低延迟系统 **优先整数定点** |
| 字节序 | 多字节 **排列** — wire 必须显式约定（§2.1.3） |
| 类型宽度 | `sizeof`/ABI — wire 用 `int32_t` 等（§2.1.2） |

→ 下一章 CPU 如何用这些格式运算：[Ch 3 机器级表示](../../chapter-03-machine-level-programs/)

---

### 口述巩固 · 自测

1. 为什么 `(a+b)+c` 和 `a+(b+c)` 对 float 可能不同？
2. `-ffast-math` 对 HFT 回测有什么风险？
3. 本章学完后，解析一条 binary 行情消息至少要考虑哪三件事？（宽度、endian、字段类型）

---

← [本章导读](../README.md) · [§2.1.3 寻址与字节序](../chapter-02-representing-information/notes/section-2.1.3-寻址与字节序.md)
