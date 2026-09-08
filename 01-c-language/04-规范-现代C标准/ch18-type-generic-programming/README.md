# Ch18 · Type-generic programming（类型泛型编程）

> **Level 3 · 深入** · 策略：**🟡 略读**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

`_Generic` 选择表达式、C23 `typeof`/`typeof_unqual`、C23 `auto` 类型推断、复合表达式与 lambda 讨论。
内核宏早已用 GNU `typeof`，C23 正式标准化后可移植性更好。

## 一、`_Generic`（C11）

### 基本语法

```c
/* _Generic: 根据表达式类型选择不同的分支 */
#define cbrt(X) _Generic((X), \
    long double: cbrtl, \
    default:     cbrt,   \
    float:       cbrtf   \
)(X)

/* 根据参数类型调用不同函数 */
cbrt(2.0f);      // → cbrtf(2.0f)    （float 版本）
cbrt(2.0);       // → cbrt(2.0)       （double 版本）
cbrt(2.0L);      // → cbrtl(2.0L)     （long double 版本）
```

### 类型安全的 min/max

```c
/* C11: 用 _Generic 实现类型安全的 min */
static inline int    min_int(int a, int b)    { return a < b ? a : b; }
static inline long   min_long(long a, long b)  { return a < b ? a : b; }
static inline double min_double(double a, double b) { return a < b ? a : b; }

#define min(x, y) _Generic((x), \
    int:    min_int, \
    long:   min_long, \
    double: min_double, \
    default: min_int \
)((x), (y))

int    a = min(3, 5);       // → min_int(3, 5)
long   b = min(3L, 5L);     // → min_long(3L, 5L)
double c = min(3.0, 5.0);   // → min_double(3.0, 5.0)
```

> 对比 GNU `typeof` 宏：`_Generic` 是标准 C，可移植；`typeof` 是 GNU 扩展，但内核已大量使用。

### `_Generic` 限制

| 限制 | 说明 |
|------|------|
| 只匹配类型，不匹配值 | 不能做 `if (x == 0)` 的条件分发 |
| `default` 兜底 | 没有匹配的分支时走 default |
| 不做隐式转换 | `int` 和 `long` 是不同分支 |
| 限定符不区分 | `const int` 和 `int` 匹配同一分支 |

## 二、C23 `typeof` / `typeof_unqual`

### 从 GNU 扩展到 C23 标准

```c
/* GNU 扩展（内核已用多年） */
typeof(x) y = x;        // y 的类型与 x 相同

/* C23：typeof 成为正式关键字 */
typeof(x) y = x;        // 保留限定符（const/volatile）
typeof_unqual(x) z = x; // 去除限定符

/* C23：typeof 可以用在声明中 */
int *const p = &x;
typeof(p) q = p;         // q 类型: int *const
typeof_unqual(p) r = p;  // r 类型: int *（去掉 const）
```

### 用 typeof 实现类型安全的宏

```c
/* C23: typeof 让宏更安全 */
#define container_of(ptr, type, member) ({  \
    const typeof(((type *)0)->member) *__mptr = (ptr);  \
    (type *)((char *)__mptr - offsetof(type, member)); })

/* DPDK/内核经典宏：从成员指针反推父结构体指针 */
struct order {
    int id;
    struct rte_ring_node node;   // 链表节点
    double price;
};

struct rte_ring_node *node = ...;
struct order *ord = container_of(node, struct order, node);
// ord 指向包含 node 的 order 结构体
```

> `container_of` 是内核和 DPDK 最核心的宏之一——`typeof` 让它类型安全。
> C23 之前依赖 GNU 扩展 `typeof`，C23 后可移植。

## 三、C23 `auto` 类型推断

```c
/* C23: auto 从初始化器推断类型 */
auto x = 42;              // x 类型: int
auto pi = 3.14;           // pi 类型: double
auto p = &x;              // p 类型: int *

/* 在复杂类型中特别有用 */
auto ring = ring_create(1024);   // ring 类型: struct ring *
auto val = ring_dequeue(ring);   // val 类型: void *
```

| C 的 `auto` vs C++ 的 `auto` | 说明 |
|------------------------------|------|
| C89 `auto` | 存储类说明符（自动存储期），几乎没人用 |
| C23 `auto` | 类型推断（从初始化器推断类型） |
| C++ `auto` | 类型推断（与 C23 类似） |

> **注意**：C23 的 `auto` 只在声明带初始化器时做类型推断。`auto x;`（无初始化器）仍是存储类说明符。

### `auto` 的适用场景

```c
/* ✅ 复杂类型简化 */
auto handler = get_handler(msg->type);   // 不需要写完整的函数指针类型
auto stats = &per_core_stats[lcore_id];  // 简化长链式访问

/* ⚠️ 不要过度使用 */
auto x = compute();   // x 是什么类型？需要看 compute 的返回值
int x = compute();    // 更清晰（除非返回类型复杂）
```

## 四、C23 `nullptr_t` 与泛型

```c
/* C23: nullptr 有自己的类型 nullptr_t */
_Generic(nullptr,
    nullptr_t: printf("nullptr\n"),
    default:    printf("not nullptr\n")
);
```

## 五、类型泛型的实际应用

### 通用打印宏

```c
/* C11: 类型安全的打印 */
#define PRINT_VAL(x) _Generic((x), \
    int:        printf("%d\n", (int)(x)), \
    long:       printf("%ld\n", (long)(x)), \
    long long:  printf("%lld\n", (long long)(x)), \
    double:     printf("%f\n", (double)(x)), \
    float:      printf("%f\n", (float)(x)), \
    char *:     printf("%s\n", (char *)(x)), \
    default:    printf("(unknown type)\n") \
)

PRINT_VAL(42);        // → printf("%d\n", 42)
PRINT_VAL(3.14);      // → printf("%f\n", 3.14)
PRINT_VAL("hello");   // → printf("%s\n", "hello")
```

### 通用比较宏

```c
#define LESS_THAN(a, b) _Generic((a), \
    int:    ((int)(a) < (int)(b)), \
    long:   ((long)(a) < (long)(b)), \
    double: ((double)(a) < (double)(b)), \
    default: ((int)(a) < (int)(b)) \
)

bool result = LESS_THAN(3, 5);   // → true
```

## HFT / DPDK 关联

| 特性 | HFT 用途 |
|------|----------|
| `typeof` + `container_of` | DPDK/内核链表操作（从节点指针获取父结构体） |
| `_Generic` | 类型安全的通用接口（打印、比较） |
| `auto` | 简化复杂类型声明（函数指针、模板化容器） |
| `typeof_unqual` | 去除 const/volatile 限定（谨慎使用） |

## 自测题

<details><summary>1. <code>_Generic</code> 和函数重载有什么区别？</summary>

C 没有函数重载（那是 C++ 的特性）。`_Generic` 是编译期类型选择——预处理器根据表达式的类型
在编译期选择不同的函数/表达式，运行时没有额外开销。函数重载是编译器名称修饰（name mangling），
运行时也是直接调用对应版本。两者效果类似但机制不同：`_Generic` 是宏展开，重载是语言特性。
</details>

<details><summary>2. <code>container_of</code> 宏做了什么？为什么需要 <code>typeof</code>？</summary>

`container_of(ptr, type, member)` 从成员指针 `ptr` 反推包含它的父结构体指针。
原理：用 `offsetof(type, member)` 计算成员在结构体中的偏移，从成员地址减去偏移得到父结构体地址。
`typeof` 用于类型检查——确保 `ptr` 的类型与 `type::member` 的类型一致，防止传入错误类型的指针。
</details>

<details><summary>3. C23 的 <code>auto</code> 和 C89 的 <code>auto</code> 有什么区别？</summary>

C89 的 `auto` 是存储类说明符（表示自动存储期），局部变量默认就是 auto，几乎没人写。
C23 的 `auto` 是类型推断——从初始化器推断变量类型。`auto x = 42;` 中 `auto` 让编译器推断 `x` 是 `int`。
C23 保留了 C89 `auto` 的存储类语义（向后兼容），但带初始化器时优先做类型推断。
</details>
