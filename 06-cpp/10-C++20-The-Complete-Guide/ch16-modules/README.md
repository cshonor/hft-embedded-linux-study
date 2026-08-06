# 第 16 章 模块

**Modules**

## 本章讲什么

C++20 的**模块（Modules）**是替代 `#include` 头文件的新机制——解决头文件的预处理慢、宏污染、重复解析问题。这是 C++ 四十年来最大的编译模型变革。

## 要点

### 头文件的问题

- **预处理慢**：`#include <iostream>` 展开数万行代码，每个 TU 都重新解析。
- **宏污染**：头文件里的宏定义影响所有 include 它的代码。
- **重复解析**：同一个头文件在多个 TU 中重复解析，编译慢。
- **顺序依赖**：头文件顺序影响行为（宏定义先后）。

### 模块基本语法

```cpp
// math.cppm（模块接口单元）
export module math;

export int add(int a, int b) { return a + b; }   // 导出
int helper() { return 0; }                         // 不导出（模块内部）

// main.cpp
import math;
int main() {
    return add(1, 2);   // OK
    // helper();        // 错误：未导出
}
```

### 模块的三种文件

| 文件 | 作用 |
|------|------|
| 模块接口单元（`.cppm`） | `export module X;` 声明导出的接口 |
| 模块实现单元（`.cpp`） | `module X;` 实现非导出部分 |
| 模块分区（partition） | `export module X:part;` 模块内部拆分 |

```cpp
// 接口 + 实现合一
export module math;
export int add(int a, int b);

// 实现单元
module math;
int add(int a, int b) { return a + b; }
```

### 模块的优势

| 维度 | 头文件 | 模块 |
|------|--------|------|
| 编译速度 | 慢（每个 TU 重新解析） | 快（预编译为 BMI，一次解析） |
| 宏污染 | 有 | 无（宏不跨模块） |
| 重复解析 | 每个 TU 都解析 | BMI 共享 |
| 顺序依赖 | 有 | 无 |
| 封装 | 全部暴露（只有 `static`/anon namespace） | 导出控制（`export`） |

### 头文件单元（Header Units）

```cpp
// 兼容旧头文件
import <iostream>;   // 把 iostream 编为头文件单元
// 等价 #include <iostream> 但更快、无宏污染
```

过渡方案：旧头文件用 `import` 导入，享受模块好处而不改写。

### 导入标准库

```cpp
import std;   // C++23：导入整个标准库（C++20 部分库支持）
import std.compat;  // 兼容 C 头文件
```

C++20 标准库的模块化支持不完整（取决于编译器），C++23 的 `import std` 才完整。

## HFT 关联

- **编译速度**：大项目编译从分钟级降到秒级——HFT 代码库大、迭代频繁，模块收益显著。
- **宏隔离**：不同模块的宏不互相污染，减少"改一个头文件影响全局"的意外。
- **封装**：`export` 只暴露接口，内部实现细节不暴露——策略库的私有实现可真正隐藏。
- **迁移成本高**：C++20 模块工具链（编译器 + 构建系统）仍不成熟，HFT 项目迁移要评估 CMake/Ninja 支持。
- **过渡用头文件单元**：`import <vector>` 先享受模块速度，再逐步改写自定义头。
- **编译缓存**：BMI（Binary Module Interface）可缓存，改一个模块只重编译该模块——CI/CD 友好。

## 自测题

1. 模块相比头文件的四个优势？
2. `export module` 和 `module` 的区别？什么是模块实现单元？
3. 头文件单元（`import <header>`）是什么？有什么过渡价值？
4. 模块如何解决宏污染问题？
5. HFT 项目迁移模块的难点是什么？过渡策略？
