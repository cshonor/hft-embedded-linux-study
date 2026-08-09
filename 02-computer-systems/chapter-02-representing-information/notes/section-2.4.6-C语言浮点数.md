## 2.4.6 C 语言中的浮点数

> **Ch2 §2.4** · [章导读](../README.md) · 上节 [§2.4.5 浮点运算](./section-2.4.5-浮点运算.md) · 下节 [§2.5 小结](./section-2.5-本章小结.md)

---

### float vs double（字段回顾）

| | float（32） | double（64） |
|--|-------------|--------------|
| 布局 | 1 + 8 EXP + 23 frac | 1 + 11 EXP + 52 frac |
| 约十进制精度 | **6–7** 位 | **15–16** 位 |

字面量：`3.14f` → float；**`3.14` 默认 double**；`3.14L` → long double。

---

### 混合运算：类型提升

表达式里 **float 与 double 相遇**（`+ * - /` 等）：

1. **float 先提升为 double**  
2. 运算在 **64 位精度** 下做完  
3. 表达式结果类型是 **double**

原因：double 范围/精度更高；且 C 默认浮点字面量就是 double。

#### 场景

```c
float a = 1.5f;
double b = 2.5;
/* a 先变 double，再加；结果 double */
double res = a + b;

float x = 1.0f, y = 2.0f;
float z = x * y;   /* 全程 float，精度较低 */

double d = 1.2f;   /* float → double：通常安全（加宽） */
float f = 3.1415926; /* 右边是 double 字面量 → 截断进 float：丢精度 */
```

| 写法 | 风险 |
|------|------|
| `float = float + double` | 先按 double 算完，再 **截断写回 float** → 小数精度丢失 |
| 热路径里反复 float↔double | 多占 cache/寄存器；嵌入式/极致延迟场景有成本 |
| 混算自动升 double | **优点：** 中间过程精度更高 |

**避坑：** 需要 float 结果就全程 `f` 后缀；需要精度就变量/字面量都用 double，别算完再塞进 float。

---

### 其它 C 细节

- **默认 argument promotion：** `float` 传可变参等场合常 **提升为 double**  
- **`printf`：** `%f` / `%a`（hex float，看精确 bit）  
- **`-ffast-math`** — 打破严格 IEEE 换速度；**回测 vs 生产** 是否一致要验证  

**HFT：** 价格/PnL 用整数 tick；统计可用 double；别靠 `float` 当账本。

---

### 口述巩固 · 自测

1. `float + double` 结果类型？运算在几位精度下做？  
2. 为什么 `3.14` 不是 float？怎样才是？  
3. `float f = 3.1415926;` 有什么问题？  
4. `-ffast-math` 对 HFT 回测有什么风险？

### 自测题

<details>
<summary>1. float 和 double 的精度分别是多少位十进制有效数字？</summary>

`float`（32 位）：约 **7 位**十进制有效数字（23 位尾数 + 1 隐含位 = 24 位二进制 → log10(2^24) ≈ 7.22）。`double`（64 位）：约 **15-16 位**（52 位尾数 + 1 = 53 位 → log10(2^53) ≈ 15.95）。HFT 行情价格如果需要 8+ 位精度，float 不够用，需要 double 或定点数。

</details>

<details>
<summary>2. `float f = 0.1; if (f == 0.1)` 会怎样？为什么？</summary>

可能为**假**。`0.1` 是 `double` 字面量，`f = 0.1` 先取 double 的 0.1 再截断为 float——精度损失。`f == 0.1` 中 f 被提升为 double 再比较，但 f 已经是截断后的值，和原始 double 0.1 不同。正确写法：`if (f == 0.1f)` 或用 epsilon 比较 `fabs(f - 0.1f) < 1e-6`。

</details>


---

← [本章导读](../README.md) · [§2.4.5 ←](./section-2.4.5-浮点运算.md) · [§2.5 →](./section-2.5-本章小结.md)
