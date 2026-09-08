# Modern C 补充：C89 → C99 → C11 → C17 → C23 差异速查

> **为什么有这份笔记：** 其余五本书（K&R、C 和指针、C 专家编程、嵌入式自我修养、C 陷阱与缺陷）
> 都以 C89/C90 为基线，有人说"过时了"。本笔记回答两个问题：
> ① 哪些内容真的过时了（读书时跳过）② C99–C23 新增了什么（读书时补上）。
> 结论：**其余五本书不换**，它们教的是标准无关的思维（指针、内存模型、声明解析、陷阱），
> 新标准只是增量，用本笔记补齐即可。

## 0. 标准时间线与现状

| 标准 | 年份 | 关键词 | 你目标环境用哪个 |
|------|------|--------|------------------|
| C89/C90 | 1989/1990 | ANSI C / ISO C，其余五本书基线 | 内核 ≤5.17 基底（`-std=gnu89`），非 C99 |
| C99 | 1999 | `//` 注释、`stdint.h`、VLA、`inline` | 内核日常写法大量沿用，但**从来不是内核基准** |
| C11 | 2011 | `_Atomic`、`threads.h`、`_Generic` | DPDK（`-std=c11`）、新项目默认；**内核 ≥5.18（2022）升 `-std=gnu11`**（详见 [LKD Ch2 §2.4](../../05-linux-kernel/chapter-02-getting-started/notes/section-2.4-内核开发的特点.md)） |
| C17/C18 | 2018 | 纯缺陷修复，零新特性 | 现代编译器默认（gcc 13 默认 gnu17） |
| C23 | 2024 | `nullptr`、`constexpr`、`typeof` | gcc 14+ 支持，逐渐铺开 |

**gcc 默认标准：** gcc 5 起 gnu11，gcc 11 起 gnu17，gcc 15 起 gnu23。
查自己环境：`gcc -dM -E -x c /dev/null | grep -i std`

## 1. 其余五本书逐本"过时清单"

### K&R（C89 基线）

| 书中写法 | 现代写法 | 严重程度 |
|----------|----------|----------|
| `int main() { }` 空参数列表 | `int main(void)` | 低（C23 起空括号又合法了，但 void 更明确） |
| old-style 声明 `int f(a, b) int a; int b; { }` | `int f(int a, int b)` | **高：C23 已删除，读时跳过** |
| 隐式 int：`static x;` | 必须写 `static int x;` | **高：C99 起非法，不要学** |
| `gets()` | `fgets()` | **高：C11 已从标准删除** |
| 函数调用前不声明（隐式返回 int） | 必须先声明 | **高：C99 起非法** |
| K&R 函数定义风格（参数括号外声明类型） | prototype 风格 | 高：只做识别，不做模仿 |

**K&R 不变的核心（占全书 90%+）：** 指针与数组关系、字符串处理、内存布局、
结构体传值/传指针、函数指针、位操作、`printf`/文件 IO、qsort 例子、Unix 系统调用入门。

### C 和指针（C89 基线）

- 几乎无过时内容；书里已预告了 C++ 引用对比，思路标准无关
- 唯一注意：部分示例用 `malloc` 不强转返回值之后又强转——跟随现代实践：**C 里不要强转 `malloc`**（C++ 才需要）

### C 专家编程（C89 基线）

- 声明解析（`char *const *(*next)()`）、链接器、`setjmp`/`longjmp`、可移植性——全部标准无关，仍是最好的资料
- 仅第 4 章个别 ANSI 细节可对照 C11 更新阅读

### 嵌入式自我修养（GNU C 扩展）

- 本来就覆盖 `__attribute__`、`inline asm`、内核列表等扩展，最贴近内核现实，无过时问题

### C 陷阱与缺陷（C89 基线）

- 全是语言陷阱（= vs ==、词法歧义、求值顺序），任何标准下都成立

## 2. C99 新增（内核已大量用，必须会）

| 特性 | 例子 | 内核中的使用 |
|------|------|--------------|
| `//` 注释 | `// TODO` | 处处 |
| `stdint.h` 定宽类型 | `uint32_t`、`int64_t` | 处处（`u32`/`s64` 是其 typedef） |
| 声明任意位置 | `for (int i = 0; ...)` | 处处 |
| `inline` 关键字 | `static inline void f(void)` | 处处（配 `static`） |
| 变长数组 VLA | `int a[n];` | **内核禁用**；C11 转可选，别用 |
| 柔性数组成员 | `struct s { int n; int d[]; };` | 常见（消息/变长协议结构） |
| `snprintf` | 安全字符串 | 处处（HFT 消息组装必用） |
| 指定初始化器 | `.field = 1` | 内核 `struct initcall`、`file_operations` 必用 |
| 复合字面量 | `(struct pt){1, 2}` | 内核常用 |
| `restrict` 指针 | `void *restrict p` | libc/DPDK 热路径（见 §4） |
| `long long` | 64 位整型 | 处处 |

## 3. C11/C17 新增（DPDK/新项目用）

| 特性 | 例子 | HFT 相关度 |
|------|------|-----------|
| `_Atomic` 原子 | `_Atomic int x; atomic_fetch_add(&x,1);` | ⭐⭐⭐ 用户态无锁数据结构（对标内核 `atomic_t`） |
| `_Static_assert` | `_Static_assert(sizeof(hdr)==8, "hdr");` | ⭐⭐⭐ 消息结构布局编译期校验，协议开发必备 |
| `threads.h` | `thrd_create` | ⭐⭐ 一般还是用 pthread |
| `_Generic` 泛型选择 | `_Generic(x, int: f, float: g)(x)` | ⭐ 少用，可读性差 |
| `_Alignas`/`alignof` | `_Alignas(64) char buf[128];` | ⭐⭐⭐ **缓存行对齐防伪共享，HFT 高频技巧** |
| `_Thread_local` | `_Thread_local int per_core;` | ⭐⭐⭐ 每核独立计数器/热数据 |
| 匿名结构/联合 | `struct { union { int a; float b; }; }` | ⭐⭐ 消息头复用 |
| `gets()` 删除 | — | 说明旧书此函数已死 |

**C17：零新特性**，纯缺陷修复，读完 C11 就等于读了 C17。

### C11 的"可选特性"机制（容易被忽略的设计变化）

C99 里 VLA、`_Complex`、`__func__` 是**强制**特性；C11 把它们全改成**可选**——编译器可以不实现，
改用一组约定好的宏告诉你"没有"：

| 探测宏 | 没定义 = 支持 | 没实现时 |
|--------|--------------|----------|
| `__STDC_NO_VLA__` | VLA 可用 | 内核 `-Wvla` 直接禁；C11 起别用 |
| `__STDC_NO_THREADS__` | `<threads.h>` 存在 | glibc 2.28 起才有；老系统链接不到 |
| `__STDC_NO_ATOMICS__` | `<stdatomic.h>` 存在 | 无锁代码全废，只能上 pthread/汇编 |

```c
/* 可移植库的标准写法：先探测再启用 */
#ifndef __STDC_NO_ATOMICS__
#include <stdatomic.h>
/* 无锁快路径 */
#else
/* pthread 互斥慢路径 */
#endif
```

推论：**"符合 C11"不代表"这些特性都有"**——这正是内核（gnu11 却禁 VLA、不用 stdatomic）和
跨平台库（DPDK 自带 rte_atomic/rte_thread 抽象层）各自做法的标准依据。

## 4. HFT 视角：旧标准环境里怎么活

**内核态（C89/C99 + GNU 扩展）：**
- 原子：内核自带 `atomic_t`/`atomic64_t` API，不用 `_Atomic`
- 对齐：`____cacheline_aligned_in_smp` 属性，不用 `_Alignas`
- per-CPU：`DEFINE_PER_CPU`，不用 `_Thread_local`
- 泛型：宏 + `typeof`（GNU 扩展），不用 `_Generic`

**用户态热路径（C11，DPDK 风格）：**
```c
// 缓存行对齐 + 每 lcore 独立 + 原子序号：三个 C11 特性一次用齐
struct rte_ring_aligned {
    _Alignas(64) _Atomic uint32_t head;   // 生产者独占行
    _Alignas(64) _Atomic uint32_t tail;   // 消费者独占行
} __rte_cache_aligned;

static _Thread_local unsigned lcore_id;   // 每核线程局部

_Static_assert(sizeof(struct msg_hdr) == 16, "wire format");
```

**编译建议：** 新项目 `-std=c11 -Wall -Wextra -Wpedantic`；写可移植库用 `__STDC_VERSION__`
判断（`199901L`=C99，`201112L`=C11，`201710L`=C17，`202311L`=C23）。

## 5. 自测题

<details><summary>1. K&R 里哪种写法在 C99 之后直接编译不过？</summary>

隐式 int（`static x;`）和调用未声明函数。old-style 函数定义到 C23 才删除，
但 C99 起 gcc 默认就会警告/报错。
</details>

<details><summary>2. 为什么 C 里 malloc 不需要强制转换返回值？</summary>

`void *` 到任意对象指针的隐式转换是 C 的规则（C++ 才要求显式转换）。
强转反而会掩盖 `malloc` 忘记 `#include <stdlib.h>` 时的隐式声明错误。
</details>

<details><summary>3. HFT 用户态无锁结构防伪共享，用 C11 哪两个特性？</summary>

`_Alignas(64)`（把生产者/消费者索引放到不同缓存行）+ `_Atomic`（免锁更新）。
内核态对应 `____cacheline_aligned_in_smp` + `atomic_t`。
</details>

<details><summary>4. restrict 关键字对优化有什么用？为什么热路径库函数到处是它？</summary>

`restrict` 承诺指针不与其它指针别名，编译器可放心做向量化和重排。
memcpy/memset 签名都带 restrict——HFT 热路径函数加 restrict 是低成本优化手段。
</details>

<details><summary>5. C11 的 threads.h 和 pthread 怎么选？</summary>

生产环境选 pthread（Linux 事实标准，功能全：亲和性、优先级、futex）。
threads.h 是可移植薄层，C11 标准库实现质量参差，HFT 场景绑核/实时调度都要 pthread。
</details>

## 6. 延伸阅读

- **Modern C**（Jens Gustedt）—— 免费在线，C17/C23 视角重讲 C，本路线第 4 本书
- **Modern C Library** —— C23 新库概览
- gcc 手册：C Dialect Options（`-std=` 全家）
