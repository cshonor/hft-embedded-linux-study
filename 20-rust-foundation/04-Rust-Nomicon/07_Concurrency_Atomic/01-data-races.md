# 1 · 数据竞争与竞争条件

← [本章目录](./README.md) · [00-overview.md](./00-overview.md) · 下一节：[02-send-sync.md](./02-send-sync.md)

---

## 消除数据竞争

Safe Rust **绝对保证**无**数据竞争**（多线程并发访问同一内存，至少一方写，且无同步）→ **UB**。

根本机制：所有权 + 借用（`&mut` 不可别名）。

## 不保证消除一般竞争条件

**死锁**、逻辑资源竞争等——在通用 OS 环境下**数学上无法彻底防范**；这类问题只导致逻辑错误，不算内存 UB。

## 安全边界

竞争条件 + **`unsafe`** 可能破坏内存安全。经典 **TOCTOU**：

1. 线程 A：边界检查通过  
2. 线程 B：修改长度/索引  
3. 线程 A：`get_unchecked` → **越界 UB**

→ 源码：[src/data_races.rs](./src/data_races.rs)（安全 `get` vs TOCTOU 注释）
