# 附录 · 标准库的设计哲学

> 所属：[第 2 章](./README.md) · 前：[第 1 章 三层架构](../chapter01_std_overview/README.md) · 后：[附录 · 读源码](./appendix-reading-stdlib-source.md)

---

## 一句话

标准库不是「API 大全」，而是三条原则的产物：**零成本抽象**、**内存安全**、**封装性** — 与 [RFR](../../02-RFR/RFR-本书目录.md) 里的类型 / 所有权思想一脉相承。读第 2 章各节源码时，用这三条当透镜。

---

## 三大原则

### 1. 零成本抽象（Zero-Cost Abstraction）

| 含义 | std 里的例子 |
|------|--------------|
| 抽象不应比手写底层代码更慢（理想状态下） | 迭代器 `map`/`filter` 可被优化成与手写循环相近的机器码 |
| 不为用不到的泛型特性付运行时代价 | `Option<T>` 与 `T` 同布局的 niche 优化（见 RFR 布局章） |
| trait 抽象可单态化 | `impl Read` 的泛型调用在编译期展开 |

**与 RFR 呼应**：RFR 强调「抽象要有机器层面的解释」— 读 std 源码时多问：**这层包装在 LLVM 眼里还剩什么？**

→ [RFR Ch02 类型与布局](../../02-RFR/Chapter-02-Types/README.md) · [06 Compilers LLVM IR](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/README.md)（按需）

---

### 2. 内存安全（Memory Safety）

| 含义 | std 里的例子 |
|------|--------------|
| Safe API 默认不引入 UB | `Vec::push` 负责容量与 aliasing，调用方无需 `unsafe` |
| `unsafe` 集中在实现边界 | `Vec` 内部 `unsafe` 扩容 / `set_len`；对外仍是 Safe |
| 并发类型用 `Send`/`Sync` 约束 | `Arc<T>` 仅在 `T: Send + Sync` 时跨线程共享 |

**与 RFR / Nomicon 呼应**：你在 RFR 学的借用、生命周期，在 std 里体现为 **「对外 Safe、对内证明 invariant」**。

→ [RFR Ch09 Unsafe](../../02-RFR/Chapter-09-Unsafe-Code/README.md) · [Nomicon 01](../../04-Rust-Nomicon/01_Safe_Unsafe/README.md)

---

### 3. 封装性（Encapsulation）

| 含义 | std 里的例子 |
|------|--------------|
| 不变量藏在类型内部 | `RefCell` 运行时借用检查；`Mutex` Poison 状态不暴露裸锁 |
| 平台差异藏在模块后 | `std::os::unix` / `windows` 扩展 trait，核心 API 跨平台 |
| 实现可换、接口稳定 | `HashMap` 默认 hasher 可换；`Read`/`Write` trait 统一 I/O |

**读源码提示**：看到 `pub` 结构体字段往往是私有的 — **invariant 在模块内维护**，这正是 [ER](../../01-ER/ER-本书目录.md) 里「隐藏表示」的工程版。

---

## 三原则对照表

| 原则 | 你作为调用方感受到的 | 读源码时去找的 |
|------|----------------------|----------------|
| 零成本抽象 | API 好用、不慢 | `#[inline]`、泛型、`iter` 适配器实现 |
| 内存安全 | 少写 `unsafe` | `unsafe` 块旁的注释与 `debug_assert!` |
| 封装性 | 不能乱改内部字段 | `mod` 私有子模块、`pub(crate)` 实现细节 |

---

## 相关

- [附录 · 如何阅读标准库源码](./appendix-reading-stdlib-source.md)
- [2.7 泛型编程](./2.7-generics-in-stdlib.md)
- [ER Item 01 表达数据结构](../../01-ER/Chapter-01-Types/Item-01-express-data-structures/README.md)
