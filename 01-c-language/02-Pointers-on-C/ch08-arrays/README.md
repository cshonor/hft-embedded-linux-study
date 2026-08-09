# 第 8 章 数组

**Arrays**

## 本章讲什么

**一维回顾、二维行优先、传参退化、指针数组 vs 数组指针、字符数组、VLA、初始化与陷阱**。DPDK `pkts[]`、内核二维表、HFT 批量缓冲的核心容器。

## 学习重点

- `arr[i] ≡ *(arr+i)`；传参 **`+ len`**
- 二维：**`*(*(buf+i)+j)`**；传参 **`int (*)[COLS]`**
- **`int *a[5]`** vs **`int (*b)[5]`**
- 字符串指针数组 vs **`char buf[N][M]`**
- **VLA** 栈风险；**`{0}`** 清零
- 栈 mega 数组 → **malloc**

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| DPDK | `pkts[BURST]`、port×queue 二维 |
| 内核 | 向量表、设备二维映射 |
| HFT | 连续行情池、常量 opcode 表 |

## 实操（建议完成）

1. 模拟 `pkts[32]`  
2. `int (*row)[3]` 遍历  
3. 字符串指针数组 vs 二维 char  
4. 大 VLA 栈溢出  
5. 二维传参仅首维省略  
6. sizeof 对比三种类型  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch06 指针；ch07 栈 |
| 后序 | ch09 字符串；ch11 堆数组；ch13 |
| 配套 | 《C陷阱与缺陷》ch03、ch07 |

## 小节

- [8.1 一维数组](./8.1-one-dimensional-arrays/8.1-one-dimensional-arrays.md)（8.1.1–8.1.11）
- [8.2 多维数组](./8.2-multidimensional-arrays/8.2-multidimensional-arrays.md)（8.2.1–8.2.7）
- [8.3 指针数组](./8.3-指针数组.md)


---

## 章节自测

> 看代码 → 想答案 → 点开验证。

### Q1: 二维数组传参

```c
// 哪种声明可以作为接收 int[3][4] 的函数参数？
void func_a(int (*arr)[4]);     // (1)
void func_b(int arr[][4]);      // (2)
void func_c(int arr[3][4]);     // (3)
void func_d(int **arr);          // (4)
```

> 哪些能正确接收 `int matrix[3][4]`？哪个不行？

<details>
<summary>答案与复习指引</summary>

**答案：** `(1)` `(2)` `(3)` 都行，`(4)` **不行**。

**解析：** 二维数组 `int[3][4]` 退化为 `int(*)[4]`（指向含 4 个 int 的数组的指针）。`int**` 是指向指针的指针，内存布局完全不同。传 `int**` 给 `int(*)[4]` 的函数 → 类型不匹配 → UB。

**复习：** → [8.2 Multidimensional Arrays](./8.2-multidimensional-arrays/8.2-multidimensional-arrays.md)

</details>

### Q2: 指针数组 vs 数组指针

```c
int *pa[5];       // (1) 这是什么？
int (*ap)[5];     // (2) 这是什么？
```

<details>
<summary>答案与复习指引</summary>

**答案：**
- `(1)` `int *pa[5]` — **指针数组**：5 个 `int*` 元素的数组
- `(2)` `int (*ap)[5]` — **数组指针**：一个指针，指向含 5 个 `int` 的数组

**区分口诀：** `[]` 优先级高于 `*`。`pa[5]` 先是数组 → 指针数组。`(*ap)` 括号强制先是指针 → 数组指针。

**复习：** → [8.3 指针数组](./8.3-指针数组.md)

</details>

### Q3: VLA 栈风险

```c
int n = get_size();
int arr[n];  // VLA, C99
```

> VLA 有什么风险？HFT/内核为什么禁用？

<details>
<summary>答案与复习指引</summary>

**答案：** VLA 在栈上分配 `n` 个元素。如果 `n` 很大 → **栈溢出**。`n` 运行时确定 → 编译器无法检查上限。

**HFT/内核禁用 VLA 的原因：**
1. 栈空间有限（内核栈仅 4-16KB）
2. `n` 可能来自外部输入 → 安全漏洞
3. 编译器不易优化
4. Linux 内核编译时 `-Wvla` 禁止 VLA

**替代方案：** 用 `malloc`（堆）或固定大小数组 + 长度参数。

**复习：** → [8.1 One-Dimensional Arrays](./8.1-one-dimensional-arrays/8.1-one-dimensional-arrays.md) — VLA

</details>

---

## 代码自测

**题目 1：** 以下代码在 C 语言中能编译吗？说明了 C 的什么特性？
```c
#include <stdio.h>
int main() {
    printf("hello\n");
}
```

<details>
<summary>参考答案</summary>

能编译。C 是编译型语言——源代码经过预处理、编译、汇编、链接四个阶段生成可执行文件。这个程序体现了 C 的基本结构：包含头文件、main 函数入口、标准库函数调用。C 的设计哲学是"信任程序员"——它不会像 Java 那样强制检查很多东西。

</details>
