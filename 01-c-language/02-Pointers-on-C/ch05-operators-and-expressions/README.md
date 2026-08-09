# 第 5 章 操作符和表达式

**Operators and Expressions**

## 本章讲什么

C **全部运算符**、**优先级/结合性**、**短路求值**、**位运算/移位**、**副作用与 UB**、**隐式/算术转换**。ch04 所有 if/while/for 条件都依赖本章；DPDK mbuf、寄存器、报文解析的**刚需底层**。

## 学习重点

- 优先级表 + **强制括号**
- 位运算 **`& | ~ ^ << >>`** vs 逻辑 **`&& ||`**
- **短路**空指针判断顺序
- **`=` vs `==`**；unsigned 与负数比较
- **有符号溢出 UB**；无符号环绕
- 移位超限 UB；**同一表达式多次 `++` UB**
- **`sizeof`**、三元 `?:`、复合赋值、逗号

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| DPDK | mbuf flags、寄存器位操作 |
| HFT | 定点价格、序列号、报文字段拆分 |
| 内核 | 掩码、短路、错误码判断 |

## 实操（建议完成）

1. 32 位拆 4 字节（移位+位与）  
2. `=`/`==` 死循环  
3. `&&` 顺序与段错误  
4. 多自增 UB 对比编译器  
5. 三元择优价  
6. unsigned char vs `-1`  
7. flags set/clear/test  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch03 类型；ch04 控制流 |
| 后序 | ch06 指针算术；ch10 位域；ch11 sizeof/堆 |
| 配套 | 《C陷阱与缺陷》ch03、ch07 |

## 小节

- [5.1 操作符](./5.1-操作符.md)
- [5.2 布尔值](./5.2-布尔值.md)
- [5.3 左值和右值](./5.3-左值和右值.md)
- [5.4 表达式求值](./5.4-expression-evaluation/5.4-expression-evaluation.md)
  - [5.4.1 隐式类型转换](./5.4-expression-evaluation/5.4.1-隐式类型转换.md)
  - [5.4.2 算术转换](./5.4-expression-evaluation/5.4.2-算术转换.md)
  - [5.4.3 优先级和求值顺序](./5.4-expression-evaluation/5.4.3-优先级和求值顺序.md)


---

## 章节自测

> 看代码 → 想答案 → 点开验证。

### Q1: 短路求值

```c
int *p = NULL;
int count = 0;

if (p != NULL && *p > 0)   // (1) 安全吗？
    count++;

if (p != NULL || *p > 0)  // (2) 安全吗？
    count++;
```

> `(1)` 和 `(2)` 哪个安全？哪个会崩溃？

<details>
<summary>答案与复习指引</summary>

**答案：** `(1)` 安全——`p == NULL` 时 `&&` 短路，不执行 `*p > 0`。`(2)` 崩溃——`p == NULL` 时 `||` 前半为假，继续执行 `*p > 0` → 解引用 NULL → 段错误。

**教训：** 先判空再解引用，用 `&&`：`if (p && *p > 0)`。`||` 中判空放后面才有保护作用。

**复习：** → [5.4 Expression Evaluation](./5.4-expression-evaluation/5.4-expression-evaluation.md) — 短路求值

</details>

### Q2: 移位 UB

```c
int x = 1;
int a = x << 31;   // (1)
int b = x << 32;   // (2)
unsigned c = 1u << 31; // (3)
```

> 三个各有问题吗？

<details>
<summary>答案与复习指引</summary>

**答案：**
- `(1)` `int` 左移 31 位进入符号位 → **UB**（有符号左移进符号位是未定义的）
- `(2)` 移位 ≥ 类型位宽 → **UB**（`int` 32 位，移 32 位未定义）
- `(3)` `unsigned` 左移 31 → 合法，结果 `0x80000000`

**教训：** 位运算用 `unsigned`。移位数必须 `< sizeof(type) * 8`。

**复习：** → [5.4 Expression Evaluation](./5.4-expression-evaluation/5.4.3-优先级和求值顺序.md) — 移位 UB

</details>

### Q3: 复合赋值只求值一次

```c
struct { int arr[10]; } *s = get_struct();

// 写法 A
s->arr[s->idx++] += 5;

// 写法 B
s->arr[s->idx++] = s->arr[s->idx++] + 5;
```

> 写法 B 有什么问题？

<details>
<summary>答案与复习指引</summary>

**答案：** 写法 B 中 `s->idx++` 出现两次 → **UB**（同表达式中多次修改同一变量）。还可能导致数组越界。

写法 A（`+=`）中 `s->arr[s->idx++]` 只被求值**一次**——复合赋值保证左侧只计算一次。

**教训：** 有副作用的表达式用复合赋值 `+=`/`-=`/`*=` 而非展开写法。

**复习：** → [5.2 Arithmetic Operators](./5.2-arithmetic-operators/5.2-arithmetic-operators.md) — 复合赋值

</details>
