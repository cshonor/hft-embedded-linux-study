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

**复习：** → [14.2 Define](./14.2-宏定义.md)

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

**复习：** → [14.2 Define](./14.2-宏定义.md) — 可变参数宏
