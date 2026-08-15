# 第 14 章 预处理器

**Preprocessor**

## 本章讲什么

编译**第一阶段**纯文本处理：**#include**、**#define**、条件编译、**`#`/`##`**、可变参数日志。DPDK/内核源码半数逻辑在宏里；调试开关与跨平台依赖本章。

## 学习重点

- 宏 = **文本替换**，无类型检查
- 表达式宏：**整体与参数加括号**
- 副作用 → **static inline** 替代函数宏
- **`do {} while(0)`** 多语句宏
- **`LOG(fmt, ...)`** + **`##__VA_ARGS__`**
- include guard / **`#pragma once`**
- **`__FILE__` / `__LINE__` / `__func__`**
- **`#if DEBUG`** 零开销关日志
- 复杂计算用 inline，常量/日志/拼接用宏

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| DPDK | RTE 常量、rte_log、PMD 条件编译 |
| 内核 | pr_info、寄存器掩码、架构 #if |
| HFT | 调试开关、崩溃行号、x86/ARM 兼容 |

## 线上陷阱（汇总）

1. 宏缺括号优先级错  
2. 参数重复求值  
3. 头文件无保护重复定义  
4. 宏未用 do-while 语法断裂  
5. 无类型校验传错参  
6. **`##`** 拼出非法标识符  
7. 过度宏化无法 gdb  

## 实操（建议完成）

1. 无括号 vs 有括号 MUL  
2. MIN(a++) 副作用  
3. 可变参数 LOG + `##__VA_ARGS__`  
4. 无 guard 重复包含  
5. DEBUG 开/关看汇编  
6. `#` + `##` 批量枚举  
7. MIN 宏 vs inline min  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch02 标识符；ch07 inline；ch10 pack |
| 后序 | ch15 printf 日志；ch18 运行时 |
| 配套 | 《C陷阱与缺陷》ch06 |

## 小节

- [14.0 `gcc -E` 预处理实操](./14.0-gcc-E-预处理.md) ← 看 `.i`、四阶段、排查宏/include
- [14.1 预定义符号](./14.1-预定义符号.md)
- [14.2 #define](./14.2-define/14.2-define.md)
- [14.3 条件编译](./14.3-条件编译.md)
- [14.4 文件包含](./14.4-file-inclusion/14.4-文件包含.md)
- [14.5 其他指令](./14.5-其他指令.md)


---

## 章节自测

> 看代码 → 想答案 → 点开验证。

### Q1: 多语句宏 do-while(0)

```c
// 写法 A (危险)
#define SWAP(a, b) \
    int tmp = a; a = b; b = tmp

// 写法 B (安全)
#define SWAP2(a, b) \
    do { int tmp = a; a = b; b = tmp; } while(0)

if (cond)
    SWAP(x, y);   // (1) 会怎样？
else
    other();
```

> 写法 A 的 `SWAP` 在 `if` 里展开后会怎样？

<details>
<summary>答案与复习指引</summary>

**答案：** 写法 A 展开后：
```c
if (cond)
    int tmp = x; x = y; b = tmp;  // 声明在 if 的作用域内，只有第一条语句被 if 控制
;                                  // 分号是空语句
else                               // else 找不到 if → 语法错误
    other();
```

写法 B 用 `do { ... } while(0)` 把多条语句包成一个语句，`if` 正确控制整个块。

**教训：** 多语句宏必须用 `do { ... } while(0)` 包裹。

**复习：** → [14.2 Define](14.2-define)

</details>


### Q3: 可变参宏 __VA_ARGS__

```c
#define LOG(fmt, ...) printf("[LOG] " fmt "\n", __VA_ARGS__)

LOG("count=%d", 42);           // A
LOG("hello");                  // B：没有额外参数
LOG("a=%d b=%d", 1, 2);       // C
```

> B 能编译通过吗？如何修复让无额外参数也能工作？

<details>
<summary>答案与复习指引</summary>

**答案：** B **编译失败**（或产生 UB）——`__VA_ARGS__` 展开为空，变成 `printf("[LOG] hello\n",)`——末尾多了逗号。

**修复（GNU 扩展）：** `##__VA_ARGS__`——当没有可变参数时自动删除前面的逗号：

```c
#define LOG(fmt, ...) printf("[LOG] " fmt "\n", ##__VA_ARGS__)
```

**C99+ 标准：** 使用 `__VA_OPT__`（C20）或在 GNU 扩展下用 `##`。

**用途：** 调试日志宏——有参数时打印参数，无参数时只打印格式串。

**复习：** → [14.2 宏定义](14.2-define) · [14.5 其他指令](./14.5-其他指令.md)

</details>

### Q4: 条件编译调试开关

```c
#define DEBUG_LEVEL 2

#if DEBUG_LEVEL >= 2
    #define DBG2(fmt, ...) fprintf(stderr, "[DBG2] " fmt "\n", ##__VA_ARGS__)
#elif DEBUG_LEVEL >= 1
    #define DBG2(fmt, ...)
#else
    #define DBG2(fmt, ...)
#endif

#if DEBUG_LEVEL >= 1
    #define DBG1(fmt, ...) fprintf(stderr, "[DBG1] " fmt "\n", ##__VA_ARGS__)
#else
    #define DBG1(fmt, ...)
#endif

DBG1("starting");       // A
DBG2("detail: x=%d", x); // B
```

> DEBUG_LEVEL=0 时 A 和 B 会生成代码吗？这种模式有什么好处？

<details>
<summary>答案与复习指引</summary>

**答案：** DEBUG_LEVEL=0 时，A 和 B 都展开为**空**——不生成任何代码，零运行时开销。

**好处：**
- 发布版 `DEBUG_LEVEL=0`，调试日志完全消失（不占代码空间、不影响性能）
- 开发版 `DEBUG_LEVEL=2`，详细日志全开
- 通过编译选项 `-DDEBUG_LEVEL=2` 控制，不需改源码

**对比 `if (debug) printf(...)`：** 运行时判断有分支开销；条件编译在预处理阶段移除——HFT 热路径用条件编译。

**复习：** → [14.3 条件编译](./14.3-条件编译.md)

</details>


### Q2: ## 粘合与 LOG 宏

```c
#define LOG(level, fmt, ...) \
    printf("[ " level " ] " fmt "\n", ##__VA_ARGS__)

LOG("INFO", "x=%d", x);     // (1)
LOG("INFO", "startup");     // (2) 没有 ... 参数
```

> `##__VA_ARGS__` 的 `##` 做什么？

<details>
<summary>答案与复习指引</summary>

**答案：** `##` 在 `__VA_ARGS__` 为空时**删除前面的逗号**。

`(2)` 没有可变参数，`##` 让 `printf("[ INFO ] startup\n")`（去掉多余逗号）。不加 `##` → `printf("[ INFO ] startup\n",)` → 语法错误。

**内核/DPDK 实例：** `pr_err`、`RTE_LOG` 都用类似模式。

**复习：** → [14.2 Define](14.2-define) — 可变参数宏

---

## 代码自测

**题目 1：** 以下宏有什么问题？如何修复？
```c
#define MAX(a, b) a > b ? a : b
int x = MAX(1, 2) + 3;
```

<details>
<summary>参考答案</summary>

展开为 1 > 2 ? 1 : 2 + 3，由于运算符优先级，实际解析为 1 > 2 ? 1 : (2 + 3) = 5（而不是预期的 max(1,2)+3 = 5）。碰巧结果相同，但如果 MAX(2, 1) + 3 展开为 2 > 1 ? 2 : 1 + 3 = 2（错误，应为 5）。修复：#define MAX(a, b) ((a) > (b) ? (a) : (b))。

</details>
