# 2. Dynamic Memory Allocation（动态分配）

> [← 章索引](./README.md)

无全局分配器时 **不能**用 `Box` / `Vec` / `String` / `Arc` 等（除非自供 `alloc`）。

## 无堆策略

- **栈 / 静态** — 固定上限容器
- **`heapless::Vec`**、const 泛型 **`ArrayVec` 思路**
- **`[MaybeUninit<T>; N]`** 上的安全封装

## 引入堆

1. 实现 **`GlobalAlloc`**
2. **`#[global_allocator]`** 注册
3. **`extern crate alloc`** → 使用 `Box`/`Vec` 等
4. 处理 **OOM** → [05 OOM](./05-oom-handler.md)
