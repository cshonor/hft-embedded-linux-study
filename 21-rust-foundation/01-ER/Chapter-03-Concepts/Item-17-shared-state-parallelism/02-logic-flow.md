# Item 17 · 逻辑脉络

← [Item 17 目录](./README.md)

```text
借用规则（多个 & 或 一个 &mut）≈ 消除 data race 的逻辑前提
         ↓
安全 Rust：编译期无 data race
         ↓
共享可变状态：Arc（多线程引用计数）+ Mutex（内部可变）→ Arc<Mutex<T>>
         ↓
多 Mutex / 锁顺序不一致 → 死锁（编译期不管）
         ↓
对策：优先消息传递；合并 State；极小锁域
```

### 锁倒置（Lock inversion）

- 线程 A：`锁1 → 锁2`
- 线程 B：`锁2 → 锁1`
- → 经典死锁。

---
