# 第 2 章 语法「陷阱」

**Syntactic Pitfalls** — Andrew Koenig, *C Traps and Pitfalls*

## 本章目标

词法分析（[ch01](../ch01-lexical-pitfalls/)）切完 token 后，编译器按 **语法规则** 组装表达式与语句。本章陷阱多为 **token 组合逻辑错误**——**常不报错，逻辑却完全跑偏**。

> **人脑缩进逻辑 ≠ 编译器语法匹配逻辑**

## 小节索引

| 节 | 主题 | 核心坑 |
|----|------|--------|
| [2.1](./2.1-运算符优先级.md) | 优先级 | `x & mask == 2` 分组错误 |
| [2.2](./2.2-else就近匹配.md) | 悬垂 else | else 绑最近 if |
| [2.3](./2.3-函数声明与调用.md) | 调用语法 | `handler` vs `handler()` |
| [2.4](./2.4-括号缺失与语句体.md) | 缺 `{}` | 仅下一语句受控 |
| [2.5](./2.5-逗号运算符与分隔符.md) | 逗号 | 运算符 vs 实参分隔 |
| [2.6](./2.6-数组与结构体语法.md) | `[]` `.` | `i[a]`、`&st.x` |

## 工程规范（内核 / HFT / 嵌入式）

1. **位运算 + 比较**：一律加括号 `(x & m) == v`
2. **所有分支/循环**：强制 `{}`
3. **函数**：头文件原型，禁隐式 int（`-Werror=implicit-function-declaration`）
4. **复杂表达式**：不用逗号运算符拼逻辑

## 前后章节

| | 章节 |
|---|------|
| **前置** | [ch01 词法](../ch01-lexical-pitfalls/) |
| **后置** | [ch03 语义](../ch03-semantic-pitfalls/) — 类型、指针、UB |
| **交叉** | [Expert C ch08 优先级表](../03-Advanced-Expert-C-Programming/ch08-halloween-vs-christmas/operator-precedence-cheatsheet.md) |

## Demo

```bash
cd demo
make all
./demo01_bitwise/main
./demo02_dangling_else/main
./demo03_func_ptr/main
./demo04_missing_braces/main
./demo05_comma/main
```

## 面试题

1. `if (x & 0x02 == 2)` 实际判断什么？如何写对？
2. 悬垂 else 绑定规则？如何修复？
3. `signal(sig, handler())` 错在哪？
4. `if (f) a=1; b=2;` 执行语义？
5. 逗号运算符与函数实参逗号有何区别？

## 章节自测

> 语法陷阱：token 组装成语句时的逻辑错误。看代码 → 想答案 → 点开验证。

### Q1: 位运算优先级

```c
int val = 0x06;  // 0b110
int mask = 0x02; // 0b010

if (val & mask == 2)
    printf("bit set\n");
else
    printf("bit not set\n");
```

> 输出什么？如何修正？

<details>
<summary>答案与复习指引</summary>

**答案：** 输出 `bit not set`。`==` 优先级**高于** `&`，所以表达式被解析为 `val & (mask == 2)`，即 `val & 1` = `0x06 & 1` = `0`（假）。

**修正：** `if ((val & mask) == 2)` — 位运算和比较一律加括号。

**复习：** → [2.1 运算符优先级](./2.1-运算符优先级.md)

</details>

### Q2: 悬垂 else

```c
int x = 1, y = 0;
if (x > 0)
    if (y > 0)
        printf("both positive\n");
else
    printf("x not positive\n");
```

> 输出什么？缩进是否反映实际逻辑？

<details>
<summary>答案与复习指引</summary>

**答案：** **没有输出**。`else` 绑定**最近的 `if`**（`if (y > 0)`），而非缩进暗示的外层 `if (x > 0)`。实际逻辑等价于：

```c
if (x > 0) {
    if (y > 0)
        printf("both positive\n");
    else
        printf("x not positive\n");  // y <= 0 时执行
}
```

`x=1, y=0` → `y > 0` 为假 → 输出 `x not positive`。

等等，重新分析：`x=1 > 0` 为真，进入内层。`y=0 > 0` 为假，执行 else → 输出 `x not positive`。

**修正：** 外层 if 加 `{}` 明确意图。

**复习：** → [2.2 else 就近匹配](./2.2-else就近匹配.md)

</details>

### Q3: 函数声明还是调用？

```c
void handler(int sig) {
    printf("signal %d\n", sig);
}

int main() {
    signal(SIGINT, handler);    // 正确：传函数指针
    signal(SIGINT, handler());  // 错误：调用 handler 后传返回值
    return 0;
}
```

> 第二行 `handler()` 有什么问题？

<details>
<summary>答案与复习指引</summary>

**答案：** `handler()` 是**调用** handler 函数（无参，但 handler 需要 int 参数——编译警告或 UB），然后把返回值 `void` 当参数传给 `signal`。正确写法是 `handler`（不带括号 = 函数指针）。

**规则：** 函数名即地址；加 `()` 就是调用。

**复习：** → [2.3 函数声明与调用](./2.3-函数声明与调用.md)

</details>

### Q4: 缺少大括号

```c
int debug = 0;

if (debug)
    log_message("starting");
    log_message("phase 1");
    log_message("phase 2");

printf("done\n");
```

> `debug=0` 时输出什么？`debug=1` 时呢？

<details>
<summary>答案与复习指引</summary>

**答案：** `debug=0` 时输出 `phase 1`、`phase 2`、`done`——只有 `log_message("starting")` 受 if 控制，后两行**总是执行**（缩进欺骗人眼）。`debug=1` 时输出全部四行。

**规则：** 所有 if/else/for/while 体一律加 `{}`，即使只有一行。

**复习：** → [2.4 括号缺失与语句体](./2.4-括号缺失与语句体.md)

</details>

### Q5: 逗号运算符 vs 实参分隔

```c
int f(int a, int b) { return a + b; }

int x = 1, y = 2;

int r1 = f(x, y);        // A
int r2 = f((x, y), 3);   // B
```

> `r1` 和 `r2` 分别是多少？

<details>
<summary>答案与复习指引</summary>

**答案：** `r1 = 3`（`f(1, 2)` = 1+2）。`r2 = 5`（`(x, y)` 是逗号运算符——求值 x 丢弃，求值 y=2 作为第一个实参，`f(2, 3)` = 2+3）。

**关键区别：** 函数调用 `f(a, b)` 的逗号是**分隔符**；`(a, b)` 中的逗号是**运算符**（左求值丢弃，右求值为结果）。

**复习：** → [2.5 逗号运算符与分隔符](./2.5-逗号运算符与分隔符.md)

</details>
