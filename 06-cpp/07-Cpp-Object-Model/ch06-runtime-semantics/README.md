# 第 6 章 运行时语义

**Runtime Semantics**

## 本章讲什么

程序运行时的对象模型行为：`new`/`delete` 的完整底层链路、RTTI（`dynamic_cast`/`typeid`）如何实现、异常处理机制的开销。本章把这些"运行时才发生"的机制讲透。

## 要点

### `new`/`delete` 完整链路

`new T(args)` = `operator new(sizeof(T))` 分配 + `T(args)` placement 构造。`delete p` = `p->~T()` 析构 + `operator delete(p)` 释放。`operator new` 可全局/类级重载，接 mempool/hugepage。

数组 `new[]` 在头部存元素数（额外开销），`delete[]` 据此逐个析构——`delete` 误配 `delete[]` 是 UB。

### RTTI（运行时类型识别）

- `dynamic_cast<D*>(b)`：向下转型，运行时检查对象真实类型。代价 = 查类型信息表（`type_info`）+ 字符串比较（类名）。失败返回 `nullptr`（指针）/抛 `bad_cast`（引用）。
- `typeid(obj)`：返回 `type_info` 引用，`.name()` 给修饰名。

RTTI 只对**多态类型**（有虚函数）有效——靠 vtable 里的 type_info 槽定位。非多态类型 RTTI 是静态的。

### 异常处理开销

- **无异常抛出时**：C++ 零开销异常模型（table-based）在正常路径几乎零开销（异常表在 .gcc_except_table，不污染热路径）。
- **抛异常时**：昂贵——栈展开 + 析构 + 查异常表，比正常返回慢 2-3 个数量级。
- `-fno-exceptions` 关闭异常可减小二进制 + 略加速，但禁用 `try`/`throw`。

## HFT 关联

- **禁 `dynamic_cast` 热路径**：RTTI 的类型表查找有 cache miss + 字符串比较代价。HFT 用 `enum` 标签 + `static_cast`（编译期保证安全）替代。
- **异常零开销模型的真相**：正常路径零开销，但抛异常极慢——HFT 把异常当"致命错误"用（崩溃重启），不当控制流。
- **`-fno-exceptions`**：部分 HFT 引擎整体关异常，换二进制体积 + 确定性。但会失去 STL 的异常保证（`bad_alloc` 等）。

## 自测题

1. `new T` 的两步是什么？`new[]` 为什么有额外开销？
2. `dynamic_cast` 只对什么类型有效？它的运行时代价是什么？
3. C++ 异常的"零开销模型"是什么意思？抛异常时代价如何？
4. HFT 热路径为什么禁 `dynamic_cast`？用什么替代？
5. `-fno-exceptions` 有什么利弊？
