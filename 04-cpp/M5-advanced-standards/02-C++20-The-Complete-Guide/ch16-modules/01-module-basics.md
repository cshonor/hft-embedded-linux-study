# C++20 模块

## 模块 vs 头文件

```cpp
// 传统头文件：预处理拷贝、宏污染、编译慢
// math.h:
#pragma once
int add(int a, int b);

// 模块：语义导入、无宏污染、编译快
// math.cppm（模块接口文件）
export module math;

export int add(int a, int b) {
    return a + b;
}

// 使用
import math;
int x = add(1, 2);
```

## 模块声明

```cpp
// ── math.cppm ──
export module math;  // 声明模块名

// 导出的声明（对外可见）
export int add(int a, int b);
export double sqrt_val(double x);

// 非导出的声明（模块内部）
int helper(int x) { return x * 2; }

// 模块分区（模块内部组织）
export module math:geometry;  // 分区
export double circle_area(double r);
```

## 导入

```cpp
// main.cpp
import math;           // 导入整个模块
import math:geometry;  // 导入特定分区

int x = add(1, 2);
double a = circle_area(3.0);
```

## 模块的优势

```cpp
// 1. 编译速度：模块只解析一次，不像头文件每次拷贝
//    #include <iostream> 在每个 .cpp 中展开 → 编译慢
//    import std; 只解析一次 → 编译快

// 2. 无宏污染：模块不导出宏
//    头文件的 #define 会污染所有包含它的文件
//    模块不传播宏

// 3. 更好的封装
//    头文件的所有声明都对外可见（即使不在 public API 中）
//    模块只导出 export 的声明

// 4. 循环依赖更好处理
//    头文件循环 #include 需要前向声明 + #pragma once
//    模块可以更优雅地处理
```

## 实际编译

```bash
# GCC
g++ -std=c++20 -fmodules-ts math.cppm -c   # 编译模块
g++ -std=c++20 -fmodules-ts main.cpp math.o  # 编译使用模块的代码

# MSVC
cl /std:c++20 /interface /c math.cppm  # 编译模块接口
cl /std:c++20 /c main.cpp              # 编译使用方
```

## HFT 应用

```cpp
// 模块化策略引擎
export module strategy.engine;

import market.data;
import order.types;
import risk.control;

export class StrategyEngine {
    // 对外只暴露 StrategyEngine
    // 内部实现细节不导出
};
```

## 自测题

1. 模块和头文件的区别？模块有哪些优势？
2. `export module` 和 `import` 的关系？
3. 模块如何实现封装？（导出 vs 非导出）
4. 模块为什么能加快编译速度？
5. 模块分区（partition）是什么？怎么用？

<details>
<summary>参考答案</summary>

1. 头文件是**预处理期的文本包含**：`#include` 把整份文本拷进每个 TU 再重新解析。
模块是**语言级的一等公民**：模块接口只编译一次，使用方 `import` 的是编译好的模块信息。
优势：
   1. **编译更快**：不重复展开解析（见第 4 题）；
   2. **无宏污染**：宏不会跨模块导出；
   3. **更好的封装**：只有 `export` 的声明对外可见，内部实现完全隐藏；
   4. **依赖更干净**：不受 include 顺序、重复包含影响，循环依赖也更容易处理；
   5. **隔离性**：模块之间同名实体互不干扰，不再需要 `#pragma once` 与各种 include 技巧。
2. `export module M;` 出现在**模块接口单元**（通常 `.cppm`）里，作用是"**定义并发布**一个名为 M 的模块"——它标志着这个 TU 是模块 M 的接口，其中 `export` 修饰的声明成为模块的公开接口。
`import M;` 出现在**使用方**里，作用是"**消费**这个模块"——导入后就能使用 M 导出的名字（未导出的看不到）。
二者是发布与消费的关系：一个模块有且仅有一个主接口单元用 `export module` 声明，任意多个 TU 用 `import` 使用它。
3. 只有带 `export` 的声明才对模块的使用者可见；未加 `export` 的实体是**模块内部实现**，在模块外既不可见也不可访问，连名字都不会泄漏。
```cpp
export module strategy.engine;

import market.data;          // 导入的不导出（除非 export import）

export class StrategyEngine { /* 公开接口 */ };
void internal_helper();      // 未导出 → 模块外看不到
export void run();           // 导出

export {                     // 也可以成块导出
    struct Config { /* ... */ };
};
```
还可以写 `export import other;` 把另一个模块**再导出**（转手暴露给使用方）。
4. 因为模块接口单元**只被编译一次**，编译结果（BMI/CMI 之类的模块中间表示）可被所有 `import` 它的 TU 直接复用——使用方不需要再做预处理展开、词法/语法分析，也无需重新实例化模板元信息。
而 `#include` 是把整份文本在每个 TU 里**重新展开并解析一遍**，成本随包含它的 TU 数量线性增长（`<iostream>` 之类的重头文件尤其明显）。
另外模块不泄漏宏，改动模块内部实现也不会触发使用方重编译（只要接口没变），进一步减少了连锁重建。
5. **模块分区（partition）**是把一个模块拆成多个从属单元，用于拆分大模块、控制可见性和并行编译。
   - 分区名以**冒号**开头，从属于同一个具名模块：`export module M:part;`（可导出的分区接口）或 `module M:part;`（内部实现分区）。
   - 主接口单元用 **`export import :part;`** 把分区再导出，其他分区之间用 **`import :part;`** 互相引用（分区内引用时冒号前缀必须写）。
   - 分区**不能**被模块外的 TU 直接 import，只能经由所属模块暴露。
   - 此外还有**模块实现单元**：`module M;`（不带 export），提供实现但不参与接口。
```cpp
// engine.cppm
export module strategy.engine;
export import :core;      // 导出 core 分区
import :detail;           // 内部分区

// core.cppm
export module strategy.engine:core;
export class StrategyEngine { /* ... */ };
```

</details>
