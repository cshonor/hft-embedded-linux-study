# 2 · 线程安全标记：`Send` 和 `Sync`

← [本章目录](./README.md) · 上一节：[01-data-races.md](./01-data-races.md) · 下一节：[03-atomics.md](./03-atomics.md)

---

**Unsafe marker traits**，并发故事基石：

| Trait | 含义 |
|-------|------|
| **`Send`** | 值**移动**到另一线程安全 |
| **`Sync`** | 多线程**共享**安全；等价于 **`&T: Send` 则 `T: Sync`** |

复合类型字段均满足则自动实现。重要**非线程安全**例外：

- **原生指针** — 无护栏  
- **`UnsafeCell`** — 无同步的内部可变性  
- **`Rc`** — 非原子 refcount，多线程 clone 可溢出 / UAF  

→ 源码：[src/send_sync.rs](./src/send_sync.rs)
