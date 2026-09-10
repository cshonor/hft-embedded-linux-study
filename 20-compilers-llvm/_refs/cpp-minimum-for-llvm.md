# 为 LLVM 准备的 C++ 最小子集

> **原则**：LLVM 用的是**非典型 C++**。它自己的 coding standards 禁用异常、禁用 RTTI，热路径用自研 ADT
> （`SmallVector` / `StringRef` / `DenseMap` / `ilist`）而非 `std::` 容器，并自研 RTTI（`isa<>` / `dyn_cast<>` / `cast<>`）。
> 因此**不需要**先把 C++ 系统学完——按本清单补到位即可开课，边做边补。
>
> 目标路径：**A · 工具性最小子集**（约 30–40 小时，且是**已有笔记的复习**，非从零学）。
> 全部笔记都在本仓 [`04-cpp`](../../04-cpp/README.md)，**无需**去姊妹仓 cpp-learning-notes（其内容早已复制进 04-cpp）。

---

## P0 · 必做（约 15–20h）

| # | 章节 | 本地入口 | 为什么 LLVM 需要 | 自测 |
|:--:|------|----------|------------------|------|
| 1 | 类 | [`M0/01-C++Primer/ch07-classes`](../../04-cpp/M0-entry-syntax/01-C%2B%2BPrimer/ch07-classes/) | ctor/dtor 在 IR 里直接可见 | 说出 `A a;` 在 IR 里触发哪些调用 |
| 2 | 动态内存 | [`M0/ch12-dynamic-memory`](../../04-cpp/M0-entry-syntax/01-C%2B%2BPrimer/ch12-dynamic-memory/) | new/delete → LLVM 的对象所有权约定 | 区分栈对象与堆对象在 IR 的差异 |
| 3 | 拷贝控制 | [`M0/ch13-copy-control`](../../04-cpp/M0-entry-syntax/01-C%2B%2BPrimer/ch13-copy-control/) | 三/五法则 → 理解 IR 里的拷贝/移动构造 | 说出三法则与五法则的差别 |
| 4 | OOP | [`M0/ch15-oop`](../../04-cpp/M0-entry-syntax/01-C%2B%2BPrimer/ch15-oop/) | 虚函数与多态 —— **LLVM Pass 层次全靠它** | 画出含单继承的 vtable 布局 |
| 5 | 模板 | [`M0/ch16-templates`](../../04-cpp/M0-entry-syntax/01-C%2B%2BPrimer/ch16-templates/) | 模板特化 = `isa<>` / `dyn_cast<>` 的实现基础 | 看懂 `template<typename T> bool isa(const Value*)` |
| 6 | 类型推导 | [`M1/ch01-deducing-types`](../../04-cpp/M1-modern-cpp/01-Effective-Modern-C%2B%2B/ch01-deducing-types/) | LLVM 代码满屏 `auto` | 说出 `auto` 与 `auto&` 推导差异 |
| 7 | auto | [`M1/ch02-auto`](../../04-cpp/M1-modern-cpp/01-Effective-Modern-C%2B%2B/ch02-auto/) | 同上 | 判断何时 `auto` 会丢引用/const |
| 8 | 智能指针 | [`M1/ch04-smart-pointers`](../../04-cpp/M1-modern-cpp/01-Effective-Modern-C%2B%2B/ch04-smart-pointers/) | `unique_ptr` 所有权 → LLVM 的 owning 约定 | 解释 `make_unique` 为何优于裸 new |
| 9 | 右值 / 移动 / 转发 | [`M1/ch05-rvalue-move-forwarding`](../../04-cpp/M1-modern-cpp/01-Effective-Modern-C%2B%2B/ch05-rvalue-move-forwarding/) | `std::move` 在 LLVM ADT 里遍地 | 区分 `std::move` 与 `std::forward` |
| 10 | lambda | [`M1/ch06-lambda-expressions`](../../04-cpp/M1-modern-cpp/01-Effective-Modern-C%2B%2B/ch06-lambda-expressions/) | Pass 遍历、回调、RAII 惯用法 | 写出带捕获的 lambda 并说明捕获语义 |

---

## P1 · 建议（约 15–20h）

| # | 章节 | 本地入口 | 为什么 |
|:--:|------|----------|--------|
| 11 | 顺序容器 | [`M0/ch09-sequential-containers`](../../04-cpp/M0-entry-syntax/01-C%2B%2BPrimer/ch09-sequential-containers/) | 对照 LLVM `SmallVector` 看差异（小容量栈上、无异常） |
| 12 | 关联容器 | [`M0/ch11-associative-containers`](../../04-cpp/M0-entry-syntax/01-C%2B%2BPrimer/ch11-associative-containers/) | 对照 LLVM `DenseMap`（开放寻址，非红黑树） |
| 13 | 对象模型 · 虚函数布局 | [`M3/01-Cpp-Object-Model`](../../04-cpp/M3-deep-principles/01-Cpp-Object-Model/) | **LLVM 禁用 RTTI 自己造**，必须真懂 vtable 布局 |

---

## ⛔ 明确跳过（对 LLVM 性价比低）

| 内容 | 跳过理由 |
|------|----------|
| `M4/03-Effective-STL` | LLVM 热路径用自研 ADT，不用 `std::` 容器 |
| `M4/04-STL-Source-Analysis` | 同上；啃下来对读 LLVM 代码帮助有限 |
| `M5` C++17 / C++20 | LLVM 17 要求 C++17，**够用**；C++20 特性用不上 |
| `M3/02-Cpp-Concurrency` | 编译器前端不需要并发（那是 `14-hft-engineering` 的事） |
| `M0/ch08` iostream · ch17/18/19 特殊工具 | 与编译器工程无关 |
| `M4/01-Effective-C++` 大部分条款 | 可缓；P0 覆盖后已能读 LLVM 代码，遇坑再查 |

> `M4/01-Effective-C++` 里**资源管理**相关条款（三/五法则那几条）已在 P0 #3 覆盖。

---

## 验收自测（P0 完成后做）

能不查资料说出下面三件事，就算过关：

1. **这段代码在干什么**
   ```cpp
   if (auto *AI = dyn_cast<AllocaInst>(&I)) {
       // ...
   }
   ```
   答要点：`dyn_cast` = 带类型检查的安全向下转换（`isa` + `cast`），失败返回 `nullptr`；
   它替代 C++ 的 `dynamic_cast`，因为 LLVM 编译时关掉了 RTTI。

2. **`std::move` 到底做了什么**
   答要点：只是把左值强转成右值引用（**不移动任何东西**），真正的移动由移动构造/赋值函数完成。

3. **LLVM 为什么不用 `std::vector<std::string>` 做热路径**
   答要点：`std::` 容器会抛异常（LLVM 全局禁用）、有额外分配开销；
   `SmallVector` 小容量内联在栈上、`StringRef` 只是 (ptr, len) 非拥有视图。

---

## 节奏建议

```text
开课前 1 周：P0 #1–#5（M0 核心 5 章）
   ↓
刷课 1–5 讲（词法/语法，不碰 C++ API）
   ↓
第 6 讲前补完：P0 #6–#10（M1 现代 C++）
   ↓
刷课 7–11 讲 → 01 任务验收
   ↓
进 02 前补：P1 #11–#13
```

**不必等 C++ 全补完再开课**——第 6 讲才第一次碰 `IRBuilder`，前面有 5 讲缓冲。
