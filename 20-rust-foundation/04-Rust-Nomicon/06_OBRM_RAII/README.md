# 06 · The Perils Of OBRM

> **The Rustonomicon** · [03 Rust Nomicon](../README.md) · [全书笔记](../notes.md)

## 状态

- [x] 已读（笔记整理）
- [x] 示例 crate（Drop / forget / proxy / guard / poison）

---

## 一句话

**OBRM 风险章** — 极简构造与递归 Drop、`mem::forget` 与泄漏、代理类型陷阱、panic Guard、Mutex 投毒。

---

## 专项笔记

| 节 | 主题 | 阅读 |
|:--:|------|------|
| **0.1** | **RAII / OBRM** | [00-1-RAII与OBRM辨析.md](./00-1-RAII与OBRM辨析.md) · [Book 15.3.0](../../00-Book/15-smart-pointers/15.3.0-RAII与OBRM辨析.md) |
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| 1 | 构造与析构 | [01-construct-drop.md](./01-construct-drop.md) |
| 2 | 泄漏与 forget | [02-forget-leak.md](./02-forget-leak.md) |
| 3 | 代理类型 | [03-proxy-types.md](./03-proxy-types.md) |
| 4 | 栈展开与异常安全 | [04-unwinding.md](./04-unwinding.md) |
| 5 | 数据投毒 | [05-poisoning.md](./05-poisoning.md) |
| — | 速记 · 自测 |

---

## 示例源码

| 文件 | 演示 |
|------|------|
| [src/construct_drop.rs](./src/construct_drop.rs) | 递归 Drop、`ManuallyDrop`、`Option::take` |
| [src/forget_leak.rs](./src/forget_leak.rs) | `mem::forget` 泄漏但内存安全 |
| [src/proxy_types.rs](./src/proxy_types.rs) | `Drain` + leak amplification |
| [src/panic_guard.rs](./src/panic_guard.rs) | RAII Guard 恢复中间状态 |
| [src/poisoning.rs](./src/poisoning.rs) | `Mutex` poison |
| [src/main.rs](./src/main.rs) | 运行入口 |

```bash
cd 04-Rust-Nomicon/06_OBRM_RAII
cargo run
cargo run -- --drop-order    # 观察递归 Drop 顺序
```

---

## 与仓库其他部分

| 主题 | 对照 |
|------|------|
| Drop / RAII·OBRM | [Book 15.3.0](../../00-Book/15-smart-pointers/15.3.0-RAII与OBRM辨析.md) · [15.3 Drop](../../00-Book/15-smart-pointers/15.3-使用Drop运行清理代码.md) · [15.3.2 Socket](../../00-Book/15-smart-pointers/15.3.2-Drop与网络Socket-RAII.md) |
| 未初始化 / set_len | [05_Uninit_Mem](../05_Uninit_Mem/README.md) |
| 上一章 | [05_Uninit_Mem](../05_Uninit_Mem/README.md) |
| 下一章 | [07_Concurrency_Atomic](../07_Concurrency_Atomic/README.md) |

---

## 逻辑脉络

RAII/OBRM 概念（00.1 · Book 15.3.0）→ 构造析构模型 → forget 与代理危险 → panic Guard → 投毒护栏 → 进入 Concurrency。

---

## 速记

## 三句背诵

0. **RAII = 构造拿资源；OBRM = RAII + 唯一所有权 + Drop**（详见 [00.1](./00-1-RAII与OBRM辨析.md)）
1. **Rust 移动 = memcpy，无地址感知构造；Drop 递归清理字段。**
2. **`forget` 泄漏资源但不算内存 UB；代理类型 forget 可能 UAF。**
3. **panic 栈展开须 minimal exception safety；无法恢复则 poison 阻断。**

## 自测

- [ ] 能解释为何纯栈侵入式链表在 Rust 中难安全实现
- [ ] 能说明 `ManuallyDrop` / `Option::take` 打破递归 Drop 的用途
- [ ] 能对比普通泄漏与 `Drain`/`Rc` 代理泄漏的风险差异
- [ ] 能描述 RAII Guard 如何防 double-drop
- [ ] 能解释 `Mutex` poison 的设计意图

## 术语表

| 术语 | 含义 |
|------|------|
| OBRM / RAII | 获取即构造、析构即释放资源 |
| leak amplification | 代理泄漏时放大泄漏以保容器基本安全 |
| minimal exception safety | panic 后仍保持内存安全 |
| Guard / Hole | panic 时由 Drop 恢复中间状态的 RAII 类型 |
| poison | 标记资源处于不可信状态，阻止继续使用 |

