# 第 9 章 再论数组

**More About Arrays** — Peter van der Linden, *Expert C Programming*

## 本章目标

在 **[ch04 数组并非指针](../ch04-arrays-are-not-pointers/)** 基础上 **深化**：系统掌握 **数组名退化（decay）** 的适用与不适用场合；解释 **`sizeof` / `&arr` / `extern`** 混淆与 **4.2 段错误**；牢记 **一维形参 `int a[]` ≡ `int *a`（仅形参）** 与 **二维必须保留内维 `int a[][4]` / `int (*a)[4]`**；熟练 **`a[i]` 对数组片段**、**`char str[N][M]` vs `char *str[N]`**、**`argv`**；掌握 **C99 柔性数组成员** 与 **`char *buf` 双 `malloc`** 差异；理解 **`int[3][4]` 行优先布局**、**`arr` vs `arr[0]`**、**`p+1` 步长 = 4×sizeof(int)**。衔接 **[ch03 声明螺旋](../ch03-analyzing-c-declarations/)**、**[ch10 指针再论](../ch10-more-about-pointers/)**。

## 数组 vs 指针：本章立场

| 命题 | 结论 |
|------|------|
| **类型是否相同** | **否** — 数组是 `T[N]`，指针是 `T*`（**ch04**） |
| **用法是否常可互换** | **是** — 下标、一维传参、遍历（**9.1, 9.5**） |
| **何时「看见真数组」** | **`sizeof(数组名)`、`&数组名`、定义/声明行**（**9.2**） |
| **形参为何像指针** | 历史 ABI：**按地址传参**，边界丢失（**9.3**） |

## 退化与不退化速查

| 语境 | `int a[10]` 行为 |
|------|------------------|
| **`a[i]`、`a+1`、赋给 `int*`、实参** | 退化为 `int*`，步长 4 B |
| **`sizeof a`** | `40` |
| **`&a`** | 类型 `int (*)[10]`，`&a+1` 步长 40 B |
| **形参 `void f(int a[])`** | `sizeof a` 为指针宽；**≠** 40 |

## 二维核心（9.6）

```c
int arr[3][4];
```

| 表达式 | 退化类型 | `+1` 步长 |
|--------|----------|-----------|
| **`arr`** | `int (*)[4]` | **16 B**（4×sizeof(int)） |
| **`arr[0]`** | `int *` | **4 B** |
| **`&arr`** | `int (*)[3][4]` | **48 B**（整块） |

**形参**：`void f(int arr[][4])` 或 `void f(int (*arr)[4])` — **禁止** `int **arr` 接收栈上 `int[m][4]`。

## `char str[10][20]` vs `char *str[10]`

| 维度 | **`char str[10][20]`** | **`char *str[10]`** |
|------|------------------------|---------------------|
| **`sizeof`** | `200` | `10×指针宽` |
| **数据位置** | 内嵌 10×20 字节块 | 指针指 rodata/堆 |
| **可写** | ✅（栈数组） | 字面量时 **❌ SIGSEGV** |
| **行宽** | 固定 20 | 各行可不等长 |
| **传参** | `char (*)[20]` | `char **` |
| **典型** | 固定列宽表 | **`argv`** |

见 **9.5**、**demo02_string_arrays**。

## C99 柔性数组成员 vs 指针成员

```c
/* 一次 malloc — 连续 */
struct MsgFlex {
    int len;
    char buf[];  /* 不计入 sizeof */
};
struct MsgFlex *m = malloc(sizeof(struct MsgFlex) + cap);
memcpy(m->buf, data, cap);
free(m);

/* 两次 malloc — 可能不连续 */
struct MsgPtr {
    int len;
    char *buf;
};
struct MsgPtr *p = malloc(sizeof *p);
p->buf = malloc(cap);
free(p->buf);
free(p);
```

| | **柔性 `buf[]`** | **`char *buf`** |
|--|------------------|-----------------|
| **分配** | 1 次 | 2 次 |
| **局部性** | 头+数据连续 | 可能分离 |
| **栈上** | ❌ 不可用 | 指针可指栈（慎生命周期） |

见 **9.5**、**demo03_flex_vs_ptr**。

## 前置依赖

| 依赖 | 说明 |
|------|------|
| **[Expert C ch04](../ch04-arrays-are-not-pointers/)** | **数组≠指针**、**extern 陷阱**、**`a[i]` 等价**（**4.1–4.5**） |
| **[Expert C ch03](../ch03-analyzing-c-declarations/)** | **`*[N]` vs `(*)[N]`** 声明螺旋（**3.3, 3.8**） |
| **[Expert C ch08](../ch08-halloween-vs-christmas/)** | **`&` 取址 vs 按位与**；软件规则之难（**8.8**） |
| **[Expert C ch07](../ch07-adventures-in-memory/)** | 内存布局、对齐、堆 |

## 知识模块

| 模块 | 小节 | 核心 |
|------|------|------|
| **1 退化回顾** | **9.1** | decay 四场景；与 ch04 衔接 |
| **2 混淆源** | **9.2** | **sizeof / &arr / extern** |
| **3 形参设计** | **9.3** | 一维等价；二维保内维 |
| **4 片段下标** | **9.4** | **`a[i]` ≡ `*(a+i)`** 对子数组 |
| **5 总结与应用** | **9.5** | 指阵 vs 阵指；**argv**；柔性数组 |
| **6 多维** | **9.6** | 行优先；**步长**；初始化 |
| **7 轶事** | **9.7** | 软/硬件权衡 |

## Demo 清单（规划引用）

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01_2d_pass** | 二维传参 `int[][4]`；`sizeof`；`p+1` 步长 16 B | **9.3, 9.6** |
| **demo02_string_arrays** | `char[2][8]` 可写 vs `char*[2]` 字面量只读 | **9.5** |
| **demo03_flex_vs_ptr** | 柔性 `buf[]` 一次 `malloc` vs `char *buf` 两次 | **9.5** |
| **demo04_ptr_arith_2d** | `arr` vs `arr[0]`；`row_ptr+1` vs `elem_ptr+1`；`&arr` | **9.2, 9.6** |

```bash
cd 00-Linux-Kernel-DPDK-Network-C/04-Expert-C-Programming/ch09-more-about-arrays/demo
cd demo01_2d_pass && make && ./demo01 && cd ..
cd demo02_string_arrays && make && ./demo02 && cd ..
cd demo03_flex_vs_ptr && make && ./demo03 && cd ..
cd demo04_ptr_arith_2d && make && ./demo04 && cd ..
```

## 高频考点 / 面试题

1. **数组名在什么情况下退化为指针？`sizeof(arr)` 何时等于整个数组？** → 赋值、实参、下标、算术、比较时退化；**`sizeof`/`&arr`/声明行不退化**；形参处 `sizeof` 是指针宽（**9.1, 9.2**, **ch04**）

2. **`extern char *arr` 与 `char arr[]` 定义为何导致段错误？** → 类型不匹配：定义是 **6 字节连续数组**，错误声明当 **指针变量**，把首字节 `'h'` 当地址（**9.2**, **ch04 4.2**）

3. **一维 `int a[]` 与二维 `int m[][4]` 作形参有何不同？能否用 `int **` 传 `int[2][4]`？** → 一维等价 `int*`；二维 **必须内维 4**，类型 `int (*)[4]`；**`int**` 布局不同，不可**（**9.3, 9.6**, **demo01**）

4. **`char str[10][20]` 与 `char *str[10]` 区别？`argv` 是哪种？** → 前者 200 字节内嵌、固定行宽；后者 10 个指针、可指只读字面量；**`argv` 是 `char *argv[]`（指针数组）**（**9.5**, **demo02**）

5. **`int arr[3][4]` 中 `arr`、`arr[0]`、`&arr` 类型与 `+1` 步长？** → `arr`→`int(*)[4]` +16B；`arr[0]`→`int*` +4B；`&arr`→`int(*)[3][4]`，`&arr+1` +48B（**9.6**, **demo04**）

**拓展：**

- **柔性数组为何不能放栈上？**（**9.5**）
- **`a[i]` 与 `i[a]` 为何等价？**（**9.4**, **ch04 4.5**）
- **动态 `int **` 二维与 `int[][COLS]` 栈二维传参为何不能混用？**（**9.6**）
- **读声明 `int (*p)[4]` 与 `int *p[4]`？**（**ch03**, **9.5**）

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **[ch04](../ch04-arrays-are-not-pointers/)** 数组≠指针；**[ch03](../ch03-analyzing-c-declarations/)** 声明；**[ch08](../ch08-halloween-vs-christmas/)** `&` 与优先级 |
| 后置 | **[ch10](../ch10-more-about-pointers/)** 指针再论、`const`、函数指针 |
| 关联 | **[ch05](../ch05-thinking-of-linking/)** 链接与符号；**[ch07](../ch07-adventures-in-memory/)** 堆与布局 |
| 全书 | **ch04 打基础，ch09 加深，ch10 收束指针**

## 小节

- [9.1 什么时候数组与指针相同](./9.1-什么时候数组与指针相同.md)
- [9.2 为什么会发生混淆](./9.2-为什么会发生混淆.md)
- [9.3 为什么C语言把数组形参当作指针](./9.3-为什么C语言把数组形参当作指针.md)
- [9.4 数组片段的下标](./9.4-数组片段的下标.md)
- [9.5 数组和指针可交换性的总结](./9.5-数组和指针可交换性的总结.md)
- [9.6 C语言的多维数组](./9.6-C语言的多维数组.md)
- [9.7 轻松一下——软件/硬件平衡](./9.7-轻松一下软件硬件平衡.md)
