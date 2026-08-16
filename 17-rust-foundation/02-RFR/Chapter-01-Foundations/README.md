# 第 1 章：基础 (Foundations)

> **Rust for Rustaceans** · [全书目录](../RFR-本书目录.md)  
> 原书章名：**Foundations** — 从内存模型与类型系统重建所有权、借用、生命周期的心理模型。

## 本章结构（与原书对齐）

**4 个主节** · 连同二级子节共 **11 个部分**（2 个带子的主节标题 + 3 + 1 + 4 + 1）。

| 主节 | 英文 | 子节 / 笔记 |
|------|------|-------------|
| **1** | Talking About Memory | [01 内存术语](./01-memory-terminology.md) · [02 变量深入](./02-variables-in-depth.md) · [03 内存区域](./03-memory-regions.md)（[03.1 Rust 模型](./03-1-rust-memory-model.md) · [03.2 OS/LLVM 布局](./03-2-os-memory-layout.md)） |
| **2** | Ownership | [04 所有权](./04-ownership.md)（[04.1](./04-1-three-rules.md) · [04.2](./04-2-move-copy-clone.md) · [04.3](./04-3-drop.md) · [04.4](./04-4-drop-order.md) · [04.5](./04-5-refs-and-panic.md) · [04.6](./04-6-pitfalls.md)） |
| **3** | Borrowing and Lifetimes | [05](./05-shared-references.md) · [06](./06-mutable-references.md) · [06.1 方法接收者](./06-1-method-self-receivers.md) · [07](./07-interior-mutability.md)（[07.1](./07-1-external-vs-interior.md)～[07.5](./07-5-comparison-pitfalls.md)）· [08](./08-lifetimes.md) |
| **4** | Summary | [09 小结](./09-summary.md) |

## 阅读顺序

```text
01 → 02 → 03 → 03.1 → 03.2 → 04 → 05 → 06 → 06.1 → 07 → 07.1 → 07.2 → 07.3 → 07.4 → 07.5 → 08 → 09
```

（03 为索引；03.1 Safe Rust 三分类，03.2 OS/LLVM 五分区 — 可按需跳过 03.2。）

## 与 The Book / ER 对照

| 主题 | 本仓库 |
|------|--------|
| 所有权、移动 | [4.1 什么是所有权](../../00-Book/04-ownership/4.1-什么是所有权.md) |
| 引用与借用 | [4.2 引用与借用](../../00-Book/04-ownership/4.2-引用与借用.md) |
| 生命周期 | [10.3 生命周期](../../00-Book/10-generics-traits-lifetimes/10.3-生命周期与引用有效性.md) |
| RefCell / 内部可变 | [15.5 RefCell](../../00-Book/15-smart-pointers/15.5-RefCell与内部可变性.md) |
| 类型表达结构 | [ER Item 01](../../01-ER/Chapter-01-Types/Item-01-express-data-structures/README.md) |
| 生命周期（ER） | [ER Item 14](../../01-ER/Chapter-03-Concepts/Item-14-lifetimes/README.md) |

## 旧版单文件

早期合并稿已拆入上表各文件；如需对照历史结构见 git 中的 `1-基础-Foundations-深度解析.md`。

---

## 速记

## 四节极简背诵（自用复习）

1. **`&T`**：共享只读，多读者；LLVM 假定不可变。
2. **`&mut T`**：编译期独占可变，**no-aliasing** 给优化空间。
3. **内部可变性**：`UnsafeCell` 底层，`let` 锁绑定、`borrow_mut` 改盒内；校验下沉运行时（`RefCell` panic）。
4. **Lifetimes**：约束引用存活区间；**NLL** 打破词法作用域；**方差** 防类型替换漏洞。

---

## 一张表：05–08 对照

| 节 | 核心 | 校验 | 优化假设 | 违反后果 |
|:--:|------|------|----------|----------|
| **05 `&T`** | 共享只读 | 编译期 | 不可变（readonly） | 编译错误 |
| **06 `&mut`** | 独占可写 | 编译期 | noalias | 编译错误 |
| **07 内部可变** | `&` 外表、内改 | 运行时/锁 | opt-out（`UnsafeCell`） | panic / 死锁 |
| **08 生命周期** | 借用活多久 | 编译期 | — | 编译错误 |
| **08 方差** | 泛型能否替换 | 编译期 | soundness | 编译错误 |

## 别名组合（同一内存、同一时刻）

| 组合 | 允许？ |
|------|:------:|
| 多个 `&T` | ✅ |
| 一个 `&mut` | ✅ |
| `&T` + `&mut` | ❌ |
| 多个 `&mut` | ❌ |

## 方法接收者（06.1）

| 写法 | 一句话 |
|------|--------|
| `self` | 消耗实例，所有权移入方法 |
| `&self` | 只读借用，可多份共存 |
| `&mut self` | 独占可变借用，调用方须 `let mut` |

→ 详述 [06.1](./06-1-method-self-receivers.md)

## 选型：改数据用谁？

| 场景 | 选用 |
|------|------|
| 单线程、独占修改清晰 | `&mut T` / `let mut` |
| 签名被钉死 `&self` 却要改字段 | `Cell` / `RefCell` 包字段（见 [06.1](./06-1-method-self-receivers.md)） |
| 多 `&` 句柄、单线程改内部 | `RefCell<T>`（复杂 T）或 `Cell<T>`（Copy 小值） |
| `Copy` 小值、不要内部引用 | `Cell<T>` |
| 多线程共享改 | `Mutex<T>` / `RwLock<T>` |

## 生命周期 vs 作用域

| | 作用域 | 生命周期（NLL） |
|---|--------|-----------------|
| 结束点 | 通常 `}` | **最后一次使用** |
| 所有权 drop | `}` | 仍在 `}`（与借用结束可不同步） |
| 典型收益 | — | `println!(r)` 后可立刻 `&mut` |

## 方差三字诀

| 型变 | 记忆 | 典型 |
|------|------|------|
| 协变 | 同向替换 | `&'a T` 对 `'a` |
| 不变 | 禁止替换 | `&mut T` 对 `T` |
| 逆变 | 反向 | `fn` 参数 |

## 四大锚点

| 概念 | 一句话 |
|------|--------|
| 所有权 | 谁负责 drop |
| 借用 | 临时读/写权 |
| 生命周期 | 借用须有效的代码区间 |
| 方差 | 防替换出悬垂 |

## 自测

- [ ] 能写 NLL 示例：`&s` 用后立刻 `&mut s`
- [ ] 能解释 `&mut T` 对 `T` 为何必须不变
- [ ] 说明 `Cell` 无计数器、`RefCell` 有计数器，互斥铁律是否相同（[07.3](./07-3-cell-vs-refcell.md)）
- [ ] 能说出 `&T` 与 `&mut` 的 LLVM 假设差异

