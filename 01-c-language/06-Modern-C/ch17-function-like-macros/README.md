# Ch17 · Function-like macros（类函数宏）

> **Level 3 · 深入** · 策略：**🟡 略读**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

宏展开原理、参数检查与副作用陷阱、访问调用上下文、变长参数宏 `__VA_OPT__`。
内核/DPDK 宏很重（`rte_ring_enqueue_bulk` 等），但现代 C 倾向用 `static inline` 和 `_Generic` 替代宏。

## 一、宏展开原理

```c
#define SQUARE(x) ((x) * (x))

/* 预处理器做纯文本替换 */
int a = SQUARE(3);     // → ((3) * (3)) → 9
int b = SQUARE(2+3);   // → ((2+3) * (2+3)) → 25（括号保护了优先级）
```

### 宏的括号规则

```c
/* ❌ 不加括号 → 优先级陷阱 */
#define SQUARE_BAD(x) x * x
SQUARE_BAD(2+3);   // → 2+3 * 2+3 = 2+6+3 = 11（不是25！）

/* ✅ 每个参数和整体都加括号 */
#define SQUARE(x) ((x) * (x))
```

| 规则 | 说明 |
|------|------|
| 每个参数用 `( )` 包裹 | 防止参数含运算符时优先级错乱 |
| 整个宏体用 `( )` 包裹 | 防止宏体在表达式中优先级错乱 |
| 宏体多条语句用 `do { ... } while(0)` | 保证在 `if`/`else` 中安全使用 |

### `do { ... } while(0)` 技巧

```c
/* 多语句宏 */
#define LOG(level, fmt, ...) \
    do { \
        fprintf(stderr, "[%s] " fmt "\n", level, ##__VA_ARGS__); \
    } while (0)

/* 在 if-else 中安全使用 */
if (debug)
    LOG("DBG", "value=%d", x);
else
    do_something();
```

> 不用 `do { } while(0)` 的话，`if` 只会执行宏的第一条语句，`else` 匹配出错。

## 二、副作用陷阱

```c
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int x = 1, y = 2;
int z = MAX(x++, y++);

/* 展开后： */
/* ((x++) > (y++) ? (x++) : (y++)) */
/* x++ 被求值两次！z 可能是 3，x 变成 2，y 变成 3 */
```

| 问题 | 说明 |
|------|------|
| 参数被多次求值 | `x++` 在展开中出现多次，自增多次 |
| 副作用不可预测 | 条件表达式的短路行为导致不同参数被求值不同次数 |
| 解决方案 | 用 `static inline` 函数替代 |

```c
/* ✅ 用函数替代宏 */
static inline int max_int(int a, int b) {
    return a > b ? a : b;
}

int z = max_int(x++, y++);   // x++ 和 y++ 各求值一次，行为可预测
```

> **HFT 建议**：能用 `static inline` 函数就不用宏。宏只在以下场景使用：条件编译、字符串操作（`#`/`##`）、类型泛型（`_Generic`）、编译期拼接。

## 三、变长参数宏 `__VA_OPT__`（C23）

### C99: `__VA_ARGS__`

```c
#define LOG(fmt, ...) printf(fmt, __VA_ARGS__)

LOG("hello\n");          // ❌ C99: __VA_ARGS__ 为空 → printf("hello\n",) 逗号多余
LOG("val=%d\n", x);      // ✅ printf("val=%d\n", x)
```

### GNU 扩展: `##__VA_ARGS__`

```c
#define LOG(fmt, ...) printf(fmt, ##__VA_ARGS__)

LOG("hello\n");          // ✅ GNU: 逗号被吃掉 → printf("hello\n")
LOG("val=%d\n", x);      // ✅ printf("val=%d\n", x)
```

### C23: `__VA_OPT__`

```c
/* C23 标准化方案 */
#define LOG(fmt, ...) printf(fmt __VA_OPT__(,) __VA_ARGS__)

LOG("hello\n");          // ✅ __VA_OPT__ 为空 → printf("hello\n")
LOG("val=%d\n", x);      // ✅ __VA_OPT__ 展开为逗号 → printf("val=%d\n", x)
```

| 写法 | 标准 | 空参数处理 |
|------|------|-----------|
| `__VA_ARGS__` | C99 | ❌ 空时逗号多余 |
| `##__VA_ARGS__` | GNU 扩展 | ✅ 空时吃掉逗号 |
| `__VA_OPT__(,) __VA_ARGS__` | C23 | ✅ 空时不展开，非空时展开为逗号 |

> **HFT 建议**：C23 项目用 `__VA_OPT__`；C99/GNU 项目继续用 `##__VA_ARGS__`。

## 四、宏访问调用上下文

某些宏利用调用者的文件名、行号等信息：

```c
/* 调试宏：自动包含位置信息 */
#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "ASSERT %s:%d: %s\n", \
                    __FILE__, __LINE__, #cond); \
            abort(); \
        } \
    } while (0)

/* # 运算符：参数字符串化 */
#define STR(x) #x
#define XSTR(x) STR(x)    // 先展开 x 再字符串化

ASSERT(ptr != NULL);
// → fprintf(stderr, "ASSERT file.c:42: ptr != NULL\n"); abort();

#define VERSION 42
printf("version: " XSTR(VERSION));  // → printf("version: 42");
```

| 运算符 | 作用 |
|--------|------|
| `#param` | 参数字符串化（加引号） |
| `##a ##b` | 标记拼接（token pasting） |
| `__FILE__` | 当前文件名 |
| `__LINE__` | 当前行号 |
| `__func__` | 当前函数名（C99） |
| `__VA_OPT__` | 条件展开（C23） |

## 五、内核/DPDK 中的宏实例

```c
/* DPDK: rte_ring 入队宏 */
#define rte_ring_enqueue(r, obj) \
    rte_ring_mp_enqueue(r, obj)

/* 内核: min/max 宏（用 typeof 做类型安全） */
#define min(x, y) ({        \
    typeof(x) _x = (x);     \
    typeof(y) _y = (y);     \
    (void) (&_x == &_y);   /* 类型检查 */  \
    _x < _y ? _x : _y; })

/* C23 可以用 typeof 正式关键字替代 */
#define min(x, y) _Generic((x), \
    int: min_int, \
    long: min_long, \
    default: min_int \
)((x), (y))
```

> 详见 [Ch18 类型泛型编程](../ch18-type-generic-programming/README.md)。

## HFT / DPDK 关联

| 宏用途 | 例子 |
|--------|------|
| 日志/调试 | `LOG()`、`ASSERT()` 带位置信息 |
| 条件编译 | `#ifdef DEBUG` |
| 类型安全 min/max | `typeof` + 语句表达式（GNU）或 `_Generic`（C23） |
| DPDK API 包装 | `rte_ring_enqueue` → `rte_ring_mp_enqueue` |
| 位操作 | `#define BIT(n) (1U << (n))` |

## 自测题

<details><summary>1. 为什么 <code>MAX(x++, y++)</code> 宏有问题？</summary>

`MAX(a, b)` 展开为 `((a) > (b) ? (a) : (b))`，`x++` 和 `y++` 在展开中出现两次。
条件表达式的求值规则是只求值被选中的分支，所以：如果 `x > y`，`x++` 执行两次；
如果 `x <= y`，`y++` 执行两次。副作用不可预测。用 `static inline` 函数可以避免——函数参数
只求值一次。
</details>

<details><summary>2. <code>__VA_OPT__</code> 解决了什么问题？</summary>

C99 的 `__VA_ARGS__` 在变长参数为空时会导致逗号多余：`printf(fmt, )` 语法错误。
GNU 扩展用 `##__VA_ARGS__` 吃掉逗号，但不是标准。C23 的 `__VA_OPT__(,)` 在参数为空时不展开
（逗号消失），非空时展开为逗号，标准化解决了这个问题。
</details>

<details><summary>3. 什么时候用宏、什么时候用 <code>static inline</code> 函数？</summary>

用宏：条件编译、字符串操作（`#`/`##`）、需要 `__FILE__`/`__LINE__` 等调用上下文、编译期常量。
用 `static inline` 函数：有类型检查、参数只求值一次、可以在调试器中单步跟踪、不依赖预处理器。
现代 C 倾向用函数替代宏——宏是"最后的手段"而非首选。
</details>
