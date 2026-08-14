# Item 8 · 核心知识点

← [Item 8 目录](./README.md)

### 引用 `&T` / `&mut T`

| | |
|--|--|
| 监督 | 借用检查器 + 生命周期 |
| 保证 | 指向合法、对齐内存；无悬垂（安全 Rust） |
| 64 位大小 | **8 字节**（单一地址） |

### 胖指针（Fat Pointers，通常 16 字节）

| 类型 | 组成 |
|------|------|
| **`&[T]`** | 数据指针 + **length** |
| **`&str`** | 同上（DST 视图） |
| **`&dyn Trait`** | 数据指针 + **vtable** 指针 —— **DST，不能裸写 `dyn Trait`** → [Item 12 §07](../../Chapter-02-Traits/Item-12-generics-vs-trait-objects/07-dyn-trait-dst-carriers.md) |

### 智能指针（Smart Pointers）

| 类型 | 角色 |
|------|------|
| **`Box<T>`** | 堆分配，**独占**所有权 |
| **`Rc<T>` / `Arc<T>`** | 引用计数共享；`Arc` 原子、可跨线程 |
| **`Weak<T>`** | 弱引用，**不**增 strong count |
| **`RefCell<T>`** | 内部可变性，**运行时**借用检查 |
| **`Cell<T>`** | 内部可变性，限 **`Copy`**，按值改 |
| **`Mutex<T>` / `RwLock<T>`** | 线程安全内部可变 + 锁 |
| **`Cow<'a, T>`** | 借或拥有；改时才 **clone** |

### 裸指针 `*const T` / `*mut T`

- **无**借用检查；仅 **`unsafe`** / FFI 底层使用。

---
