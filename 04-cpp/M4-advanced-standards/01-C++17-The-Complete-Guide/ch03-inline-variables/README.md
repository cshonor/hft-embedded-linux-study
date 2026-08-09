# 第 3 章 内联变量

**Inline Variables**

## 本章讲什么

C++17 之前只有内联函数（`inline` 函数可跨多个 TU 定义不冲突），但**变量不行**——头文件里定义全局变量会导致每个 include 它的 TU 都有一份，链接时多重定义错误。C++17 引入 `inline` 变量解决这个问题。

## 要点

### 问题背景

```cpp
// header.h
// C++14：多重定义错误
const int MAX = 1024;   // 每个 include 的 TU 都有一份，链接冲突
// 只能写 extern 声明 + .cpp 定义，麻烦

// C++17：inline 变量
inline const int MAX = 1024;   // 所有 TU 共享同一份
```

### 用法

```cpp
// 头文件中
inline constexpr int BUF_SIZE = 4096;   // constexpr 隐式 inline（C++17）
inline static std::string version = "1.0";   // 类外静态成员也行

class Config {
    static inline int count = 0;   // 类内定义静态成员（C++17，不用再写 .cpp）
};
```

### 关键点

| 规则 | 说明 |
|------|------|
| 跨 TU 唯一 | `inline` 变量在所有 TU 中共享同一份（像 inline 函数） |
| 类内静态成员 | C++17 可在类内直接定义 `static inline`，不用分离到 .cpp |
| `constexpr` 隐式 inline | `constexpr` 变量自动是 inline，无需重复写 |
| 初始化顺序 | 仍要注意跨 TU 的动态初始化顺序（static init order fiasco） |

### 替代了什么

```cpp
// C++14 旧法：声明 + 定义分离
// header.h
extern const int MAX;
// source.cpp
const int MAX = 1024;

// C++17 新法：一行搞定
inline const int MAX = 1024;
```

## HFT 关联

- **配置常量集中在头文件**：缓冲大小、超时、阈值用 `inline constexpr` 写在公共头，所有 TU 共享，无多重定义。
- **类内静态成员简化**：策略类的统计计数器 `static inline int order_count = 0;` 不用再写 .cpp 定义，减少文件数。
- **header-only 库**：HFT 工具库做成 header-only，inline 变量让全局状态（如全局配置单例的标记）也能放头文件。
- **constexpr 隐式 inline**：编译期常量天然 inline，热路径配置用 `constexpr` 既编译期求值又跨 TU 共享。

## 自测题

1. C++17 之前为什么不能在头文件里定义全局变量？
2. `inline` 变量和 `inline` 函数的"inline"含义一样吗？
3. `constexpr` 变量需要写 `inline` 吗？为什么？
4. `static inline` 类成员解决了什么麻烦？
5. HFT 配置常量为什么用 `inline constexpr` 写在头文件？

## 代码自测

### Q1: 头文件全局变量
```cpp
// header.h
// C++14: 需要一个 .cpp 定义
// extern const int MAX = 100;  // 声明
// const int MAX = 100;  // 多次包含 → 重复定义

// C++17: inline 变量
inline const int MAX = 100;  // 多个 TU 包含也 OK
inline static std::string VERSION = "1.0";
```
> C++17 之前在头文件定义全局变量有什么问题？inline 变量如何解决？

<details>
<summary>答案与复习指引</summary>

**C++17 之前的问题**：头文件被多个翻译单元（TU）包含时，非 inline 全局变量会重复定义 → 链接错误（multiple definition）。旧方案：在 .cpp 中定义 + .h 中 extern 声明，或用 `static`/匿名命名空间（但每个 TU 一份拷贝，浪费内存）。

**`inline` 变量**（C++17）：编译器允许多个 TU 定义同名 inline 变量，链接器合并为一份。和 inline 函数同理。

**用途**：头文件中的常量、配置参数、静态类成员的类内初始化。

**复习：** → [inline 变量](./README.md)
</details>
