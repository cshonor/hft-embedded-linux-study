# 第 16 章 标准函数库

**Standard Library**

## 本章讲什么

C 标准库工具集总览：**stdlib**、**string**、**ctype**、**math**、**time**、**stdarg**、信号与执行环境。区分标准库封装 vs 系统调用；规避隐藏 UB 与 HFT 性能损耗。

## 学习重点

### 按模块（与全书衔接）

| 模块 | 头文件 | 要点 |
|------|--------|------|
| 内存/串 | string.h | **mem\*** 二进制；**str\*** 文本（ch09） |
| 堆 | stdlib.h | malloc/posix_memalign（ch11） |
| 转换 | stdlib.h | **strtoll** 非 atoi |
| 随机 | stdlib.h | rand 线程不安全 → rand_r/rte_rand |
| 排序 | stdlib.h | qsort/bsearch + 回调（ch13） |
| 控制 | stdlib.h | exit/atexit；**禁 system** |
| 字符 | ctype.h | unsigned char（ch09） |
| 数学 | math.h | 价用整型，浮点仅统计 |
| 时间 | time.h | clock_gettime 纳秒延迟 |
| 可变参 | stdarg.h | vfprintf 日志（ch15） |

- **二进制 → mem\***；**文本 → str\***
- DPDK：**rte_memcpy**、**rte_rand**、自研排序对标本章优化点

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| DPDK | 标准库设计参照、规避锁/碎片 |
| 网关 | strtoll 解析、qsort 合约表、atexit 落盘 |
| HFT | 热路径禁 malloc/system/qsort 通用路径 |

## 线上陷阱（汇总）

1. atoi 静默失败  
2. rand 多线程竞争  
3. ctype 负 char UB  
4. qsort 回调强转错  
5. system 子进程抖动  
6. memcpy 重叠  
7. 热路径 malloc 锁  

## 实操（建议完成）

1. atoi vs strtoll  
2. qsort + bsearch 订单 ID  
3. memcpy vs memmove 重叠  
4. 多线程 rand vs rand_r  
5. clock 测解析耗时  
6. atexit 快照落盘  
7. ctype 负 char UB  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch09/ch11/ch13/ch15 |
| 后序 | ch17 ADT；ch18 syscall |
| 配套 | 《C陷阱与缺陷》ch05 |

## 小节

- [16.1 整型函数](./16.1-整型函数.md)
- [16.2 浮点型函数](./16.2-浮点型函数.md)
- [16.3 日期和时间函数](./16.3-日期和时间函数.md)
- [16.4 非本地跳转](./16.4-非本地跳转.md)
- [16.5 信号](./16.5-信号.md)
- [16.6 打印可变参数列表](./16.6-打印可变参数列表.md)
- [16.7 执行环境](./16.7-执行环境.md)
- [16.8 locale](./16.8-locale.md)


---

## 章节自测

> 看代码 → 想答案 → 点开验证。

### Q1: qsort + 回调

```c
int cmp(const void *a, const void *b) {
    return *(const int*)a - *(const int*)b;
}

int arr[] = {5, 2, 8, 1, 9, 3};
qsort(arr, 6, sizeof(int), cmp);

// arr 现在是什么？
```

> `qsort` 的四个参数各是什么？`cmp` 返回值有什么要求？

<details>
<summary>答案与复习指引</summary>

**答案：** `arr` 变为 `{1, 2, 3, 5, 8, 9}`。

**参数：** `qsort(基地址, 元素个数, 单元素大小, 比较函数指针)`

**cmp 要求：** 返回 `< 0`（a 排前）、`0`（相等）、`> 0`（a 排后）。**不能直接 `return a - b`**——如果差值溢出（如 `INT_MIN - INT_MAX`）会导致错误比较结果。安全写法：
```c
int va = *(const int*)a, vb = *(const int*)b;
return (va > vb) - (va < vb);
```

**复习：** → [16.1 stdlib](./16.1-stdlib.md)

</details>


### Q3: atoi vs strtol 安全转换

```c
const char *s1 = "42abc";
const char *s2 = "hello";
const char *s3 = "999999999999999999999";  // 溢出

int a = atoi(s1);     // A
int b = atoi(s2);     // B
int c = atoi(s3);     // C

char *end;
long d = strtol(s1, &end, 10);  // D
```

> A、B、C 分别返回什么？atoi 有什么缺陷？strtol 如何解决？

<details>
<summary>答案与复习指引</summary>

**答案：**
- A = **42**（atoi 解析数字直到非数字字符）
- B = **0**（atoi 解析失败返回 0——无法区分 "0" 和 "hello"）
- C = **UB**（atoi 溢出是未定义行为）

**atoi 缺陷：** 无法检测错误（0 既可能是合法值也可能是错误），溢出是 UB。

**strtol 优势：**
- 返回 `long`（范围更大）
- `end` 指针指向未解析部分——可以检查是否全部解析
- `errno = ERANGE` 检测溢出
- 支持任意进制（base 参数）

```c
errno = 0;
long val = strtol(s1, &end, 10);
if (errno == ERANGE) /* 溢出 */;
if (*end != '\0')    /* 有未解析字符 */;
```

**复习：** → [16.1 stdlib](./16.1-stdlib.md)

</details>

### Q4: 可变参数函数 va_list

```c
#include <stdarg.h>

int sum_ints(int count, ...) {
    va_list ap;
    va_start(ap, count);
    int total = 0;
    for (int i = 0; i < count; i++)
        total += va_arg(ap, int);
    va_end(ap);
    return total;
}

int r = sum_ints(3, 10, 20, 30);     // A: 60
int r2 = sum_ints(3, 10, 20);         // B: 只传了 2 个
int r3 = sum_ints(3, 10.0, 20, 30);   // C: 传了 double
```

> B 和 C 会发生什么？可变参数函数有什么安全隐患？

<details>
<summary>答案与复习指引</summary>

**答案：**
- B：**UB**——`va_arg` 读取第 3 个参数时访问栈上未初始化的内存，返回垃圾值
- C：**UB**——`10.0` 是 `double`（8 字节），`va_arg(ap, int)` 按 `int`（4 字节）读取——栈布局错位，后续参数全部错误

**安全隐患：**
- 编译器**不检查参数类型和个数**——`printf` 格式符检查是编译器扩展，不是标准
- 栈布局依赖 ABI——不同平台参数传递方式不同（寄存器 vs 栈）
- 攻击者可通过格式串攻击读取栈数据

**规则：** 可变参数函数必须约定参数类型和个数（如 `count` 参数）；C++ 用可变参数模板更安全。

**复习：** → [16.6 打印可变参数列表](./16.6-打印可变参数列表.md)

</details>


### Q2: exit vs _exit

```c
#include <stdlib.h>
#include <unistd.h>

void cleanup(void) { printf("flushing...\n"); }

int main(void) {
    atexit(cleanup);
    // _exit(1);   // (1) cleanup 会执行吗？
    exit(1);        // (2) cleanup 会执行吗？
}
```

> `exit(1)` 和 `_exit(1)` 哪个会执行 `atexit` 注册的函数？

<details>
<summary>答案与复习指引</summary>

**答案：** `exit(1)` 会执行 `atexit` 注册的函数 + 刷 stdio 缓冲。`_exit(1)` 是系统调用，**直接终止进程**，不执行任何清理。

**用途：** `fork` 后子进程如果 `exec` 失败应调 `_exit`（不是 `exit`），避免触发父进程的 `atexit` 清理函数（可能重复 flush 父的 stdio 缓冲）。

**复习：** → [16.7 执行环境](./16.7-执行环境.md)

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
