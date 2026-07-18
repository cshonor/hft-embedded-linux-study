# 第 4 章 令人震惊的事实：数组和指针并不相同

**Arrays Are Not Pointers** — Peter van der Linden, *Expert C Programming*

## 本章目标

建立 **数组 ≠ 指针** 的硬结论：理解 **数组名退化（decay）** 规则及 **sizeof / &arr** 例外；掌握 **跨文件 `extern` 声明必须与定义类型一致**（`char arr[]` ≠ `extern char *arr`），能解释 **4.2 经典段错误**；熟记 **四项本质差异**、**形参 `int a[]` ≡ `int *a`**（仅限形参）、**`a[i]` 与 `*(a+i)`**；为 **ch05 链接**、**ch09 再论数组**、**Embedded ch07 指针与存储** 打底。

## 数组名退化的四种典型场景

多数表达式中，数组名 **由 `T[N]` 调整为 `T*`**（指向首元素）：

| # | 场景 | 示例 |
|---|------|------|
| **1** | **赋给指针** | `int *p = arr;` |
| **2** | **函数实参** | `foo(arr);` → 形参收到 `int*` |
| **3** | **下标 / 算术运算** | `arr[i]`、`arr + 1`（先 decay 再算） |
| **4** | **比较** | `if (arr == p)` 比较地址 |

**不退化（必记例外）**：

| 语境 | 行为 |
|------|------|
| **`sizeof(arr)`** | 整个数组字节数 |
| **`&arr`** | 类型 `T (*)[N]`，步长为数组总大小 |
| **声明/定义行** | `int arr[10]` 中 arr 仍是数组 |
| **（对比）文件作用域 `extern`** | 必须写 `extern T arr[]`，**不能** 写 `extern T *arr`（**4.4**） |

## 四项本质差异（必背）

| 维度 | 数组 `T a[N]` | 指针 `T *p` |
|------|---------------|-------------|
| **存储** | N 个 T 连续存放 | 一个地址值 |
| **`sizeof`** | `N * sizeof(T)` | `sizeof(T*)` |
| **`&`** | `T (*)[N]` | `T **` |
| **赋值** | 不可 `a = ...` | 可 `p = &x` |

详见 **4.1**、**4.5**；**demo01_sizeof** 验证 `sizeof` 与 `&arr+1` vs `arr+1`。

## 函数形参等价（仅形参）

```c
void f(int a[]);    /* 完全等价 */
void f(int a[10]);  /* 10 被忽略 */
void f(int *a);
```

- 形参处 **没有真正的数组类型**，`sizeof(a)` 是指针大小（**4.5**）。
- **切勿** 将此等价推广到 **`extern` 声明**（**4.2–4.4**）。

下标定义（**4.5**，**demo03_subscript**）：

```text
a[i]  ≡  *(a + i)  ≡  i[a]
```

## 前置依赖

| 依赖 | 说明 |
|------|------|
| **[Expert C ch03](../ch03-analyzing-c-declarations/)** | 声明读法：区分 `char *` 与 `char[]`（**3.3**、**3.8**） |
| **[Expert C ch02](../ch02-its-not-a-bug-its-a-language-feature/)** | `*`、`[]` 多重含义（**2.3**） |
| **[01-K-and-R-C](../../01-K-and-R-C/)** | 数组、指针、字符串基础 |

## 环境

- **编译器**：GCC 或 Clang，`gcc --version`
- **推荐 flags**：`-std=c11 -Wall -Wextra`
- **demo/**：见下（已存在，勿改源码）

## 快速操作 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/04-Expert-C-Programming/ch04-arrays-are-not-pointers/demo

make all
./demo01_sizeof          # sizeof 与 arr+1 / &arr+1（4.1, 4.5）
./demo03_subscript       # a[i]、*(a+i)、i[a]（4.5）
./demo02_correct         # extern char arr[] — 正常（4.2, 4.4）

# 错误 extern 演示（可能 SIGSEGV，慎用）
make demo02_wrong
./demo02_wrong

# 单独编译
gcc -std=c11 -Wall demo01_sizeof.c -o demo01_sizeof
gcc -std=c11 -Wall demo03_subscript.c -o demo03_subscript
gcc -std=c11 -Wall demo02_extern/arr_def.c demo02_extern/use_correct.c -o demo02_correct
gcc -std=c11 -Wall demo02_extern/arr_def.c demo02_extern/use_wrong.c -o demo02_wrong

make clean
```

## 知识模块

| 模块 | 小节 | 核心 |
|------|------|------|
| **1 根本结论** | **4.1** | 数组 ≠ 指针；decay；sizeof / & 例外 |
| **2 经典崩溃** | **4.2** | `char arr[]` + `extern char *arr` → SIGSEGV |
| **3 声明/定义** | **4.3** | extern 仅声明；定义分配存储 |
| **4 类型匹配** | **4.4** | 跨文件 `[]` 与 `*` 不可互换 |
| **5 其它差异** | **4.5** | 布局、指针算术、形参、下标等价 |
| **6 轻松一下** | **4.6** | 回文；复习 `[]` / `*` |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01_sizeof** | 全局/局部 sizeof；`arr+1` vs `&arr+1`；形参 sizeof | **4.1**, **4.5** |
| **demo02_correct** | `extern char arr[]` 跨文件链接 | **4.2**, **4.4** |
| **demo02_wrong** | `extern char *arr` 错误声明 | **4.2**, **4.4** |
| **demo03_subscript** | `arr[1]`、`1[arr]`、`*(arr+1)` | **4.5** |

## 高频考点 / 面试题

1. **数组和指针是一回事吗？** → **不是**。多数语境数组名 **退化为首元素指针**；对象是数组时 `sizeof`、`&` 行为不同（**4.1**, **demo01**）

2. **`extern char *arr` 与 `char arr[] = "hi"` 一起链接会怎样？** → 链接可能成功，运行把首字符当指针值解引用 → **崩溃**（**4.2**, **demo02_wrong**）

3. **`sizeof(数组名)` 与 `sizeof(指针)`？** → 前者 **元素总大小**；后者 **指针宽度**。函数形参里写 `int a[]` 仍是指针，`sizeof(a)` 是指针大小（**4.5**）

4. **`arr+1` 和 `&arr+1` 区别？** → `arr` 退化为 `T*`，加 1 跳 **一个元素**；`&arr` 是 `T(*)[N]`，加 1 跳 **整个数组**（**4.5**, **demo01**）

5. **函数参数 `int a[]` 和 `int *a` 一样吗？** → **等价**（数组形参调整为指针）。**但** 文件作用域 `extern int a[]` **不等于** `extern int *a`（**4.4**, **4.5**）

6. **`a[i]` 和 `*(a+i)`、`i[a]`？** → 三者等价；`i[a]` 合法但勿在生产代码使用（**4.5**, **demo03**）

**拓展：**

- 为何 C 链接不做类型检查？（**4.4**, **ch05 5.3**）
- `char s[] = "x"` vs `char *s = "x"` 存储与 const 区别（**4.2**, **4.5**）
- **ch09** 何时数组与指针「可交换」

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **[Expert C ch03](../ch03-analyzing-c-declarations/)** 声明分析；**ch02** 符号过载 |
| 后置 | **[ch05](../ch05-thinking-of-linking/)** 链接与符号；**[ch09](../ch09-more-about-arrays/)** 数组再论、形参设计 |
| 关联 | **[Embedded C ch07](../../05-Embedded-C-Self-Cultivation/ch07-data-storage-and-pointers/)** 数据存储、数组名、指针（**7.8**, **7.9**） |
| 全书 | **ch07** 内存布局；**ch08** 类型转换 |

## 小节

- [4.1 数组并非指针](./4.1-数组并非指针.md)
- [4.2 我的代码为什么无法运行](./4.2-我的代码为什么无法运行.md)
- [4.3 什么是声明，什么是定义](./4.3-什么是声明-什么是定义.md)
- [4.4 使声明与定义相匹配](./4.4-使声明与定义相匹配.md)
- [4.5 数组和指针的其他区别](./4.5-数组和指针的其他区别.md)
- [4.6 轻松一下——回文的乐趣](./4.6-轻松一下回文的乐趣.md)
