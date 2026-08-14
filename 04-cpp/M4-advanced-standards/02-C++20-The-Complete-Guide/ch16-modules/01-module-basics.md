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
