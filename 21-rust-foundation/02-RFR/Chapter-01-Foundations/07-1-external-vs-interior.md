# 3.3.1 · 外部可变性 vs 内部可变性

> 所属：**Borrowing and Lifetimes · 内部可变性** · [← 07 hub](./07-interior-mutability.md)

← [07 hub](./07-interior-mutability.md) · 上一节 [06 可变引用](./06-mutable-references.md) · 下一节 [07.2 UnsafeCell](./07-2-unsafecell-and-containers.md)

---

## 一、外部可变性（前置）

`let` / `let mut` 控制**绑定层**能不能改 — **编译期**静态检查：

| 声明 | 绑定 | 通过该绑定拿 `&mut` |
|------|------|---------------------|
| `let a = 10` | 不可 `a = 20` | ❌ 不能 `&mut a` |
| `let mut a = 10` | 可重新赋值 | ✅ 可 `&mut a`（仍受借用规则约束） |

### 借用铁律（复习）

同一时刻**二选一**：

1. 任意多个 `&T`  
2. **至多一个** `&mut T`

有活跃 `&T` → 禁止修改；冲突 **编译失败**，零运行开销。

→ [05 共享引用](./05-shared-references.md) · [06 可变引用](./06-mutable-references.md)

### 外部可变性的短板

| 痛点 | 例子 |
|------|------|
| API 只能 `&self` | trait 方法固定，内部却要改计数 |
| `Rc` 只有 `&T` | 多所有者，静态规则拿不到 `&mut` |
| `mut` 粒度太粗 | 只想改缓存字段，不想整结构 `mut` |

---

## 二、内部可变性 · 定义

**可变性封装在容器内**：外层常用 `let`（绑定不变），通过容器 API 改**内部**数据。

**大白话**：**外面的绑定不动，只改盒子里的东西**。

| | **外部可变性** | **内部可变性** |
|---|----------------|----------------|
| 谁管权限 | 外部 `mut` | 容器内部（运行时/锁） |
| 检查时机 | **编译期** | **运行时**（`RefCell` panic）或锁 |
| 表层借用 | `&mut T` / `mut` 绑定 | 常是 `&RefCell<T>`、`&self` |

### 容器路线图

| 场景 | 容器 | 子节 |
|------|------|------|
| 底层 opt-out | `UnsafeCell<T>` | [07.2](./07-2-unsafecell-and-containers.md) |
| 单线程 · `Copy` | `Cell<T>` | [07.3](./07-3-cell-vs-refcell.md) |
| 单线程 · 通用 | `RefCell<T>` | [07.3](./07-3-cell-vs-refcell.md) |
| 多线程 | `Mutex` / `RwLock` | [07.2](./07-2-unsafecell-and-containers.md) |

---

## 三、速记

1. **外部**：编译器看 `mut`，冲突编译期拦。  
2. **内部**：外层 `let`，盒内由容器管。  
3. **为何需要**：`&self`、`Rc`、细粒度字段修改。

→ 下一节：[07.2 UnsafeCell 与容器速查](./07-2-unsafecell-and-containers.md)
