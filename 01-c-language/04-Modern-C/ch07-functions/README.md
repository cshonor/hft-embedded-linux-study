# Ch7 · Functions（函数）

> **Level 1 · 相识** · 策略：**🟡 略读**（聚焦 inline 现代规则 + C23 属性）
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

函数声明/定义、`main` 的特殊性、递归、**inline 的现代规则**、no-return 属性。
K&R Ch1/4 已覆盖函数基础；本章重点在 **C99/C23 inline 语义**和 **C23 属性语法**。

## 一、函数基础（速过）

```c
/* 声明（原型） */
int add(int a, int b);

/* 定义 */
int add(int a, int b) {
    return a + b;
}

/* C23：参数类型可以省略吗？不行。
   C23 删除了 K&R 风格参数声明，必须用 prototype 风格。 */
```

### C23 变化：删除 K&R 风格函数定义

```c
/* ❌ C23 已删除：K&R 风格（参数类型在括号外） */
int old_style(a, b)
    int a;
    int b;
{
    return a + b;
}

/* ✅ C23 唯一合法写法：prototype 风格 */
int modern_style(int a, int b) {
    return a + b;
}
```

> C99 起 K&R 风格已废弃（gcc 警告），C23 正式删除。五本书提到 K&R 风格时只需识别、不模仿。

## 二、`main` 的特殊性

已在 [Ch2](../ch02-principal-structure-of-program/README.md) 详细讨论。要点：
- `int main(void)` 或 `int main(int argc, char *argv[])`
- `return 0;` 可省略（C99+）
- C23 允许省略返回类型（隐式 `int`），但不推荐

## 三、递归

```c
/* 经典：阶乘 */
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
```

| 要点 | 说明 |
|------|------|
| 栈消耗 | 每层递归压栈，深度过大 → 栈溢出 |
| 尾递归优化 | 如果递归调用是函数最后一步，编译器可能优化为循环（不保证） |
| HFT 场景 | 热路径避免递归（不可预测的栈深度）；解析嵌套结构时可用 |

> **HFT 建议**：热路径用迭代替代递归；递归只在初始化/配置解析阶段使用。

## 四、inline 函数的现代规则（重点）

### C89 vs C99 vs GNU inline

| 模式 | 语法 | 语义 | 头文件中可用 |
|------|------|------|-------------|
| C89 | 无 `inline` | 不存在 | — |
| **C99 inline** | `inline` | 内联是建议，链接时需要一处外部定义 | ❌（单独需要 `.c` 中提供定义） |
| **C99 `static inline`** | `static inline` | 文件内可见，每个翻译单元独立一份 | ✅ |
| **C99 `extern inline`** | `inline` + `.c` 中 `extern` | 头文件提供内联版，`.c` 提供外部版 | ✅（配合 `extern`） |
| **GNU inline** | `extern inline` | 反转 C99 语义（GNU 扩展） | ✅ |

### 实际推荐

```c
/* ✅ 头文件中：static inline（内核和 DPDK 标配） */
static inline uint32_t ring_next(uint32_t idx, uint32_t mask) {
    return (idx + 1) & mask;
}

/* ✅ 源文件中：普通函数 */
int ring_enqueue(struct ring *r, void *item) {
    // ...
}
```

| 写法 | 放头文件 | 放源文件 | 适用场景 |
|------|----------|----------|----------|
| `static inline` | ✅ | ⚠️（可以但意义不大） | 热路径小函数（内核/DPDK 标配） |
| `inline`（C99） | ❌（需要外部定义） | ✅ | 编译器可忽略 inline 建议退回普通调用 |
| `[[gnu::always_inline]] static inline` | ✅ | — | 强制内联（HFT 热路径） |

### C23 属性语法

```c
/* C23：标准化属性语法 [[...]] */
[[gnu::always_inline]] static inline void hot_func(void) { }

/* 等价于 GNU 扩展 */
__attribute__((always_inline)) static inline void hot_func(void) { }
```

| C23 属性 | 说明 | HFT 用途 |
|----------|------|----------|
| `[[noreturn]]` | 函数不会返回 | 错误终止函数 `die()` / `abort_handler()` |
| `[[gnu::always_inline]]` | 强制内联 | 热路径函数 |
| `[[gnu::noinline]]` | 禁止内联 | 调试时隔离函数 |
| `[[deprecated]]` | 标记弃用 | API 迁移 |
| `[[nodiscard]]` | 返回值不能忽略 | 校验函数 |
| `[[maybe_unused]]` | 抑制未使用警告 | 条件编译保留的参数 |

```c
/* C23 noreturn 示例 */
[[noreturn]] void fatal(const char *msg) {
    fprintf(stderr, "FATAL: %s\n", msg);
    abort();
}

void init(void) {
    if (!setup_hardware()) {
        fatal("hardware init failed");
        // 编译器知道这里不会返回，不需要 return
    }
    // ...
}
```

## 五、函数指针（速过，详见 Ch11）

```c
/* 函数指针类型 */
typedef int (*compare_fn)(const void *, const void *);

/* qsort 回调 */
int cmp_int(const void *a, const void *b) {
    return *(const int *)a - *(const int *)b;
}

int arr[] = {3, 1, 4, 1, 5, 9};
qsort(arr, 6, sizeof(int), cmp_int);
```

> DPDK 大量使用函数指针做回调：`rte_eth_dev_cb_fn`（收包回调）、`rte_timer_cb`（定时器回调）。

## 六、参数传递

C 只有**值传递**。指针模拟传引用：

```c
/* 值传递：改不了原变量 */
void set_to_42(int x) { x = 42; }       // 无效

/* 指针传递：通过地址间接修改 */
void set_to_42(int *x) { *x = 42; }     // 有效

int v = 0;
set_to_42(&v);   // v 变成 42
```

### 数组参数退化

```c
/* 这三种写法完全等价：数组退化为指针 */
void process(int arr[10]);
void process(int arr[]);
void process(int *arr);     // 编译器看到的真正签名
```

> 数组大小信息丢失——需要额外传长度参数。`sizeof(arr)` 在函数内得到的是指针大小（8），不是数组大小。

## HFT / DPDK 关联

| 特性 | HFT 用途 |
|------|----------|
| `static inline` | 头文件热路径函数（零调用开销） |
| `[[noreturn]]` | 错误终止函数（编译器可优化控制流） |
| `[[gnu::always_inline]]` | 强制内联关键路径函数 |
| 函数指针 | DPDK 回调机制（收包、定时器、中断处理） |
| 值传递 | 小结构体（≤16 bytes）直接传值可能比指针更快（避免间接寻址） |

## 自测题

<details><summary>1. <code>static inline</code> 放头文件里，多个 .c 文件包含会链接冲突吗？</summary>

不会。`static` 使符号只在当前翻译单元可见，每个包含该头文件的 `.c` 文件各自有一份副本。
编译器内联展开后函数消失；即使不内联，多份 `static` 副本也不会冲突。这就是为什么内核和 DPDK
头文件里到处是 `static inline`。
</details>

<details><summary>2. C99 的 <code>inline</code> 和 <code>static inline</code> 有什么区别？</summary>

`inline`（不带 static）：头文件中提供内联版本，但还需要在某个 `.c` 文件中提供一份外部定义
（用 `extern inline` 声明），否则链接报错。语义复杂，实际很少用。
`static inline`：文件内可见，每个翻译单元独立一份，不需要外部定义。简单直接，是工业标准做法。
</details>

<details><summary>3. <code>[[noreturn]]</code> 对编译器优化有什么帮助？</summary>

编译器知道 `[[noreturn]]` 函数不会返回，可以优化调用点之后的代码：
① 不需要在调用后插入"unreachable"检查；② 死代码消除更激进；
③ 警告调用后的不可达代码。内核的 `BUG()` / `panic()` 就是 noreturn 函数。
</details>

<details><summary>4. 为什么数组作为函数参数时 <code>sizeof</code> 不对？</summary>

数组参数退化为指针——`void f(int arr[10])` 实际上是 `void f(int *arr)`。
在函数内 `sizeof(arr)` 得到的是指针大小（64 位系统为 8），不是数组大小。
必须额外传长度参数：`void f(int *arr, size_t n)`。这是 K&R Ch5 讲过的经典陷阱。
</details>
