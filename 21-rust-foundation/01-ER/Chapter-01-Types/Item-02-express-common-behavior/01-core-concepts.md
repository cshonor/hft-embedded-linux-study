# Item 2 · 核心知识点

← [Item 2 目录](./README.md)

### 方法与函数

| 形式 | 说明 |
|------|------|
| **函数** | 自由函数，组织代码复用 |
| **方法** | 与类型绑定的函数，写在 `impl` 块中 |
| `&self` | 不可变借用，只读 |
| `&mut self` | 可变借用，可改实例 |
| `self` | 按值接收，**消耗**所有权 |

→ 展开（三种 self、struct/enum 示例、关联函数）→ [09-methods-and-self.md](./09-methods-and-self.md)

### 函数指针 `fn`

- 指向代码地址，**不捕获环境**。
- 实现 `Copy`、`Eq`，可当作普通值传递、比较（在显式转为 `fn` 类型之后）。

### 闭包（Closures）

- 可捕获上下文的匿名函数。
- 编译器为每个闭包生成**唯一的匿名 struct**，保存捕获的引用或移动进来的值。

### 闭包 Trait（由捕获方式决定）

| Trait | 捕获方式 | 调用 |
|-------|----------|------|
| **`FnOnce`** | 移动或消耗环境 | 通常只能调用一次 |
| **`FnMut`** | `&mut` 借用环境 | 可多次调用并修改环境 |
| **`Fn`** | `&` 借用环境 | 可多次只读调用 |

**向下兼容**（API 要求越弱，传入选择越多）：

| 参数要求 | 可传入 |
|----------|--------|
| `F: FnOnce` | `Fn` / `FnMut` / `FnOnce` 闭包都行 |
| `F: FnMut` | `FnMut` / `Fn` |
| `F: Fn` | 仅 `Fn` |

→ `FnOnce<()>` 里 `<>` 的含义、与 `'env` 的区别：[06-trait-generic-params.md](./06-trait-generic-params.md)

### 静/动态分发（与 Item 12 衔接）

| 写法 | 分发方式 |
|------|----------|
| 泛型 / `F: FnOnce()` | **静态分发**（单态化，编译期定调用） |
| `Box<dyn Fn()>` / `&dyn Trait` | **动态分发**（运行期查 vtable） |

大白话版 → [Item 12 §06](../../Chapter-02-Traits/Item-12-generics-vs-trait-objects/06-dispatch-beginner-guide.md)

### Trait（特征）

- 一组**共享行为的契约**；类似 Go/Java 接口、C++ 纯虚类。
- **标记 Trait**：无方法的空 trait（如 `StableSort`），在类型层面编码「签不了名」的语义。

---
