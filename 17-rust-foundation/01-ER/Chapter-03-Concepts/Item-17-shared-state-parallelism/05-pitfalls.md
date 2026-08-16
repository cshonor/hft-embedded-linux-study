# Item 17 · 易错细节

← [Item 17 目录](./README.md)

| 细节 | 说明 |
|------|------|
| **锁中毒 (poisoning)** | 持锁线程 panic → 数据可能半更新；`lock().unwrap()` 传播 panic 是常见务实选择 |
| **`Rc` / `RefCell` 跨线程** | 非 `Send`/`Sync` → 编译报错；须 `Arc` + `Mutex` |
| **缩小锁域过度** | 为避死锁拆锁 → 可能引入**撕裂**，比死锁更难查 |

---
