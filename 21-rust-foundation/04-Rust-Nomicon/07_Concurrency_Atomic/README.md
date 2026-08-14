# 07 · Concurrency and Parallelism

> **The Rustonomicon** · [03 Rust Nomicon](../README.md) · [全书笔记](../notes.md)

## 状态

- [x] 已读（笔记整理）
- [x] 示例 crate（data race 边界 / Send·Sync / atomics）

---

## 一句话

**并发章** — Safe 消除数据竞争但不防死锁；Send/Sync 标记；C++20 内存序与 Relaxed / Acquire-Release / SeqCst。

---

## 专项笔记

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| 1 | 数据竞争 | [01-data-races.md](./01-data-races.md) |
| 2 | Send / Sync | [02-send-sync.md](./02-send-sync.md) |
| 3 | 原子操作 | [03-atomics.md](./03-atomics.md) |
| — | 速记 · 自测 |

---

## 示例源码

| 文件 | 演示 |
|------|------|
| [src/data_races.rs](./src/data_races.rs) | 安全访问 vs TOCTOU + unsafe 风险 |
| [src/send_sync.rs](./src/send_sync.rs) | `Send`/`Sync` 编译期边界 |
| [src/atomics.rs](./src/atomics.rs) | Relaxed 计数、Release/Acquire、SeqCst |
| [src/main.rs](./src/main.rs) | 运行入口 |

```bash
cd 04-Rust-Nomicon/07_Concurrency_Atomic
cargo run
```

---

## 与仓库其他部分

| 主题 | 对照 |
|------|------|
| Send/Sync 入门 | [Book 16.4 demo](../../00-Book/16-fearless-concurrency/16.4-send-sync-demo/) |
| 内部可变性 | [RFR 07-interior-mutability](../../02-RFR/Chapter-01-Foundations/07-interior-mutability.md) |
| 上一章 | [06_OBRM_RAII](../06_OBRM_RAII/README.md) |
| 下一章 | [08_Impl_Vec_Arc](../08_Impl_Vec_Arc/README.md) · 实现 Arc |

---

## 逻辑脉络

数据竞争边界 → Send/Sync → 原子与内存序 → 进入 Implementing Vec/Arc。

---

## 速记

## 三句背诵

1. **Safe 杜绝数据竞争（并发写+无同步=UB）；死锁等竞争条件仍可能发生。**
2. **Send = 可跨线程移动；Sync = 可跨线程共享引用（&T: Send ⇔ T: Sync）。**
3. **普通 load/store 不能同步；Relaxed 仅原子性，Release/Acquire 配对，SeqCst 全局序。**

## 自测

- [ ] 能区分数据竞争与一般竞争条件（死锁、TOCTOU）
- [ ] 能解释 `Rc` 为何非 Send/Sync
- [ ] 能描述 TOCTOU + `get_unchecked` 的 UB 路径
- [ ] 能说出 Release/Acquire 与 Relaxed 的选用场景
- [ ] 能对照 [atomics.rs](./src/atomics.rs) 指出各 memory ordering 示例

## 术语表

| 术语 | 含义 |
|------|------|
| 数据竞争 | 并发访问同一内存且至少一方写、无同步 → UB |
| TOCTOU | 检查时有效、使用时已变（time-of-check vs time-of-use） |
| Send / Sync | 编译期线程安全标记 trait |
| happens-before | 跨线程可见性偏序关系 |
| memory ordering | 原子操作的同步强度（Relaxed / Acquire-Release / SeqCst） |

