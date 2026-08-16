# ChA · Transitional code（过渡代码）

> **附录** · 策略：**🟡 略读**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本附录讲什么

从旧标准迁移到 C23 的实用技巧：兼容性宏、过渡头文件、条件编译策略。

## 一、跨标准条件编译

```c
/* 检测编译器支持的 C 标准版本 */
#if defined(__STDC_VERSION__)
  #if __STDC_VERSION__ >= 202311L
    #define C23_AVAILABLE 1
  #elif __STDC_VERSION__ >= 201710L
    #define C17_AVAILABLE 1
  #elif __STDC_VERSION__ >= 201112L
    #define C11_AVAILABLE 1
  #elif __STDC_VERSION__ >= 199901L
    #define C99_AVAILABLE 1
  #endif
#endif

/* 使用特性前检查 */
#ifdef C23_AVAILABLE
    // 用 nullptr、constexpr、bool 关键字
#else
    #include <stdbool.h>
    #include <stdint.h>
    #define nullptr NULL
#endif
```

## 二、C23 特性的兼容性 polyfill

```c
/* nullptr 兼容 */
#ifndef __cplusplus
  #if __STDC_VERSION__ >= 202311L
    /* C23: nullptr 是关键字 */
  #else
    #define nullptr NULL
  #endif
#endif

/* constexpr 兼容 */
#if __STDC_VERSION__ < 202311L
  #define constexpr const          /* 退化为 const（不是编译期常量，但行为接近） */
  /* 或用 enum / #define 替代 */
#endif

/* alignas/alignof 兼容 */
#if __STDC_VERSION__ >= 202311L
  /* C23: alignas/alignof 是关键字 */
#elif __STDC_VERSION__ >= 201112L
  #include <stdalign.h>
  /* C11: alignas/alignof 是宏，映射到 _Alignas/_Alignof */
#else
  #define alignas(x) __attribute__((aligned(x)))
  #define alignof(x) _Alignof(x)
#endif
```

## 三、实际迁移策略

| 步骤 | 操作 |
|------|------|
| 1. 升级编译器 | gcc 14+ / clang 18+ 支持 C23 大部分特性 |
| 2. 切换标准 | `-std=c11` → `-std=c2x`，修复编译警告 |
| 3. 逐步替换 | `#define` → `constexpr`、`NULL` → `nullptr`、`_Bool` → `bool` |
| 4. 删除兼容代码 | 确认不再支持旧标准后删掉 `#include <stdbool.h>` 等 |
| 5. 测试 | 特别注意内存序相关代码在 x86/ARM 上的行为差异 |

> **HFT 注意**：DPDK 目前用 C11（`-std=c11`）。迁移到 C23 需要等 DPDK 官方支持，
> 不要在 DPDK 代码里自行切换标准（可能与 DPDK 内部的条件编译冲突）。

## 自测题

<details><summary>1. 如何检测编译器是否支持 C23？</summary>

检查 `__STDC_VERSION__` 宏：`>= 202311L` 表示 C23。可以用 `#if __STDC_VERSION__ >= 202311L`
做条件编译，在 C23 代码中使用 `nullptr`/`constexpr`/`bool` 关键字，旧标准中退化为宏。
</details>
