# 第 3 章 语义「陷阱」

**Semantic Pitfalls** — Andrew Koenig, *C Traps and Pitfalls*

## 本章目标

词法（[ch01](../ch01-lexical-pitfalls/)）解决 **怎么切 token**，语法（[ch02](../ch02-syntactic-pitfalls/)）解决 **怎么组语句**；**语义** 决定 **类型、内存、指针、运算的实际含义**。

> 全书 **最难、底层踩坑最多** 的一章：多为 **UB、隐式转换、指针/数组混淆**，编译器 **不报错**，运行 **随机出错**。

## 小节索引

| 节 | 主题 |
|----|------|
| [3.1](./3.1-指针与数组混淆.md) | 数组 decay、`sizeof(ptr)` 陷阱 |
| [3.2](./3.2-空指针解引用.md) | `NULL` 解引用 UB |
| [3.3](./3.3-有符号无符号转换.md) | `int` vs `unsigned` 比较、死循环 |
| [3.4](./3.4-整型溢出.md) | 有符号 UB vs 无符号回绕 |
| [3.5](./3.5-指针运算规则.md) | 步长、`void*` |
| [3.6](./3.6-实参求值顺序.md) | `f(i++, i)` UB |
| [3.7](./3.7-数组下标越界.md) | 无边界检查 |
| [3.8](./3.8-结构体与位域.md) | padding、位域符号 |
| [3.9](./3.9-字符串字面量只读.md) | `"..."` 不可写 |

## 工程强制规范（内核 / HFT / 嵌入式）

1. 传数组 **必带长度**；禁止 `sizeof(ptr)` 当缓冲区大小
2. **不混用** 有符号/无符号比较；循环计数类型统一
3. 解引用前 **判空**
4. 实参 **无副作用**（无 `++`/`--` 混用）
5. 字符串字面量 **不修改**；用 `char[]` 或 `const char *`
6. 有符号累加考虑 **溢出**；长度/索引用 `size_t`

## 前后章节

| | 章节 |
|---|------|
| **前置** | [ch02 语法](../ch02-syntactic-pitfalls/) |
| **后置** | [ch04 连接](../ch04-linking/) — extern、静态符号 |
| **交叉** | [Expert C ch04–ch07](../../03-Advanced-Expert-C-Programming/) |

## Demo

```bash
cd demo && make all
./demo01_array_ptr/main
./demo02_signed_unsigned/main
./demo03_unsigned_loop/main
./demo04_arg_order/main
./demo05_bounds/main
./demo06_ro_string/main
```

## 面试题

1. `sizeof(arr)` vs `sizeof(p)`，`p=arr`？
2. `int i=-1; unsigned u=10; i<u` 结果？
3. `for (unsigned i=n; i>=0; i--)` 为何死循环？
4. 有符号 vs 无符号溢出标准差异？
5. `char *s="x"; s[0]='y'` 为何错？

## 章节自测

> 语义陷阱：类型、指针、内存的实际含义出错。全书最难一章。看代码 → 想答案 → 点开验证。

### Q1: sizeof 数组 vs 指针

```c
void process(int arr[], int n) {
    printf("sizeof(arr) = %zu\n", sizeof(arr));
}

int main() {
    int data[10] = {0};
    printf("sizeof(data) = %zu\n", sizeof(data));
    process(data, 10);
    return 0;
}
```

> 两次 sizeof 分别输出多少？（假设 64 位系统，int=4）

<details>
<summary>答案与复习指引</summary>

**答案：** `sizeof(data) = 40`（10×4=40 字节，真数组）；`sizeof(arr) = 8`（形参 `arr` 已退化为 `int*`，64 位指针 8 字节）。

**教训：** 函数内无法用 `sizeof` 获取数组长度——必须靠参数 `n` 传递。`sizeof(ptr)` 是最经典的数组/指针混淆。

**复习：** → [3.1 指针与数组混淆](./3.1-指针与数组混淆.md)

</details>

### Q2: 有符号无符号比较

```c
unsigned int len = 5;
int idx = -1;

if (idx < len)
    printf("valid index\n");
else
    printf("invalid\n");

for (unsigned int i = len - 1; i >= 0; i--) {
    printf("%u ", i);
    if (i == 0) break;
}
```

> 输出什么？循环正常结束吗？

<details>
<summary>答案与复习指引</summary>

**答案：** 输出 `invalid`。`idx=-1` 与 `unsigned` 比较时，`idx` 被隐式转换为 `unsigned`，变成 `4294967295`（UINT_MAX），远大于 5。

循环：`i >= 0` 对 `unsigned` **永远为真**——当 `i=0` 再 `i--` 变成 `4294967295`，死循环。这里靠 `if (i==0) break` 逃出。

**规则：** 循环计数器不要用 `unsigned` 做 `>= 0` 判断；不混用 signed/unsigned 比较。

**复习：** → [3.3 有符号无符号转换](./3.3-有符号无符号转换.md)

</details>

### Q3: 整型溢出差异

```c
int si = INT_MAX;      // 2147483647
unsigned ui = UINT_MAX; // 4294967295

si = si + 1;
ui = ui + 1;

printf("si = %d\nui = %u\n", si, ui);
```

> 两个结果分别是什么？哪个是 UB？

<details>
<summary>答案与复习指引</summary>

**答案：**
- `si + 1`：**UB**（有符号溢出）。实际常回绕为 `-2147483648`，但编译器可假设不溢出做优化。
- `ui + 1`：**明确定义**——回绕为 `0`（mod 2^32）。

**HFT 关联：** 累加成交量用 `int` 可能溢出 UB → 用 `uint64_t`（回绕确定）或做前置溢出检查。

**复习：** → [3.4 整型溢出](./3.4-整型溢出.md)

</details>

### Q4: 实参求值顺序

```c
int i = 1;
printf("%d %d\n", i, i++);
```

> 输出什么？这是 UB 吗？

<details>
<summary>答案与复习指引</summary>

**答案：** **未定义行为（UB）**。函数实参的求值顺序在 C 标准中是 **unspecified**（编译器可选择任意顺序），而 `i++` 修改了 `i` 且另一个实参也读 `i`——同一对象的无序读写是 UB。

可能的输出：`1 1`、`2 1`、或任何值。

**规则：** 函数实参不要有副作用（`++`/`--`）——拆成两行。

**复习：** → [3.6 实参求值顺序](./3.6-实参求值顺序.md)

</details>

### Q5: 字符串字面量只读

```c
char *s = "hello";
s[0] = 'H';
printf("%s\n", s);
```

> 这段代码会发生什么？

<details>
<summary>答案与复习指引</summary>

**答案：** **UB**——字符串字面量 `"hello"` 存储在只读段（`.rodata`），修改它可能触发 **SIGSEGV**（段错误）。某些旧系统可能不崩溃，但行为未定义。

**正确写法：** `char s[] = "hello";`（栈上副本，可修改）或 `const char *s = "hello";`（编译器阻止修改）。

**复习：** → [3.9 字符串字面量只读](./3.9-字符串字面量只读.md)

</details>
