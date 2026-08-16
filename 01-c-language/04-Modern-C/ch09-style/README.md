# Ch9 · Style（风格）

> **Level 2 · 相知** · 策略：**🟡 略读**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

格式化、命名约定、现代 C 风格哲学。作者作为 ISO C 标准委员会成员给出的建议与内核风格
（`linux/Documentation/process/coding-style`）对照阅读，互补而非矛盾。

## 一、命名约定

### Modern C 建议 vs 内核风格

| 方面 | Modern C 建议 | Linux 内核风格 | HFT 实践 |
|------|--------------|----------------|----------|
| 变量名 | `snake_case` | `snake_case` | 一致 |
| 函数名 | `snake_case` | `snake_case` | 一致 |
| 宏名 | `UPPER_CASE` | `UPPER_CASE` | 一致 |
| 类型名 | `TypeName_t`（typedef 加 `_t`） | `struct type_name`（不加 typedef） | DPDK 用 `_t` 后缀 |
| 布尔变量 | `is_ready` / `has_data` | 同 | 同 |
| 全局变量 | `g_counter`（或不加前缀） | 不加前缀，靠作用域 | HFT 加 `g_` 前缀 |

### C23 `constexpr` 对命名的影响

```c
/* C23 之前：宏常量 */
#define MAX_CONNECTIONS 1024

/* C23：constexpr 更清晰的命名空间 */
constexpr int max_connections = 1024;
```

## 二、格式化

### 关键分歧点

| 方面 | Modern C | Linux 内核 | 实际选择 |
|------|----------|------------|----------|
| 缩进 | 4 空格 | 8 空格（Tab=8） | 团队统一；DPDK 用 4 空格 |
| 行宽 | 80–100 | 80 | 80 是安全选择 |
| 大括号位置 | K&R（`{` 同行） | K&R | 一致 |
| 指针星号 | `int *p`（贴名） | `int *p`（贴名） | 一致 |
| `return` 加不加括号 | `return x;` | `return x;`（不加括号） | 不加 |

### `const` 正确性

```c
/* Modern C 强调 const 正确性 */
size_t strlen(const char *s);           // 不修改 s 指向的内容
char *strchr(const char *s, int c);     // 返回非 const 指针（C 的历史包袱）

/* HFT 建议：函数参数该加 const 就加 */
int ring_enqueue(struct ring *r, const void *item);  // 不修改 item 内容
const void *ring_peek(const struct ring *r);          // 不修改 ring，返回只读指针
```

## 三、现代 C 风格清单

| 规则 | 说明 |
|------|------|
| `const` 正确性 | 不修改的参数加 `const`，编译器帮你检查 |
| `bool` 替代 `int` | C23 的 `bool`/`true`/`false` 比 `int flag = 1` 清晰 |
| `constexpr` 替代 `#define` | 类型安全的编译期常量 |
| `static inline` 小函数 | 替代宏做类型安全的内联 |
| 指定初始化器 | `= {.field = value}` 比顺序初始化清晰 |
| `static_assert` | 编译期校验结构布局和常量关系 |
| `[[nodiscard]]` | 标注返回值不能忽略的函数 |
| 不用 VLA | 用 `malloc` 或定长数组 |
| 不用 `atoi` | 用 `strtol`（可检测错误） |
| 不用 `sprintf` | 用 `snprintf`（有长度限制） |

## HFT / DPDK 关联

- DPDK 风格与 Modern C 建议高度一致（`snake_case`、`_t` 后缀、`static inline`）
- 内核 8 空格缩进是唯一显著分歧——HFT 用户态代码通常跟随 DPDK 用 4 空格
- `const` 正确性在多线程代码中尤为重要：`const` 参数暗示函数线程安全（至少对该参数）

## 自测题

<details><summary>1. 为什么 Modern C 推荐用 <code>constexpr</code> 替代 <code>#define</code>？</summary>

`#define` 是预处理器的文本替换：无类型、无作用域、调试器看不到、没有命名空间。
`constexpr` 声明有类型的编译期常量：有作用域、类型安全、调试器可见、可取地址。
</details>

<details><summary>2. 函数参数加 <code>const</code> 有什么实际好处？</summary>

① 编译器帮你检查函数体内是否意外修改了参数；② 调用者看到 `const` 知道函数不会修改数据，
在多线程环境下更安心；③ 编译器可基于 const 做更激进的优化。HFT 代码应养成习惯：不修改的
指针/引用参数一律加 `const`。
</details>
