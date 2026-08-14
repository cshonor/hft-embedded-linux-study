# 0.1 · RAII 与 OBRM（本章前置辨析）

← [00 本章定位](./00-overview.md) · 完整版 → [Book 15.3.0 RAII/OBRM](../../00-Book/15-smart-pointers/15.3.0-RAII与OBRM辨析.md)

> Nomicon 官方标题含 **OBRM / RAII** — 先弄清二者关系，再读本章「**Perils（陷阱）**」。  
> demo · Book：`cargo run -- obrm`（[15.3-drop-demo](../../00-Book/15-smart-pointers/15.3-drop-demo/)）· Nomicon：`cd 06_OBRM_RAII && cargo run`

---

## 拼写纠正

| 错 | 对 |
|----|-----|
| RALII | **RAII** — **R**esource **A**cquisition **I**s **I**nitialization |
| — | **OBRM** — **O**wnership-**B**ased **R**esource **M**anagement |

---

## 核心关系

```text
RAII  = C++ 起源的设计模式（构造拿资源，析构放资源）
OBRM  = Rust/Nomicon：RAII + 唯一所有权 + move + 编译期检查
日常说「Rust 的 RAII」≈ OBRM
```

| | RAII（C++） | OBRM（Rust） |
|---|-------------|--------------|
| 释放时机 | 析构 / 出作用域 | `Drop` trait · 出作用域 |
| 所有权 | 可浅拷贝 → double free / UAF | **唯一所有者** · move |
| 借用 | 无规范 | `&T` / `&mut T` |
| 与 GC | 无关（**确定性**释放） | 无关 |

**一句话**：RAII 是思想；OBRM 是 Rust 把 RAII **锁死、规范化、编译期强制安全**的落地形态。

→ 对照表 · C++ `FileGuard` · 误区 · 与 Deref 串联：[Book 15.3.0 全文](../../00-Book/15-smart-pointers/15.3.0-RAII与OBRM辨析.md)

---

## 与 Book 15 / 所有权三规则

| OBRM 规则 | Rust |
|-----------|------|
| 唯一所有者 | [RFR Ch01 §04.1 三条规则](../../02-RFR/Chapter-01-Foundations/04-1-three-rules.md) |
| 出作用域 Drop | [15.3 Drop](../../00-Book/15-smart-pointers/15.3-使用Drop运行清理代码.md) |
| 智能指针 | [15.1 Box](../../00-Book/15-smart-pointers/15.1-使用Box指向堆上的数据.md) · [15.2 Deref](../../00-Book/15-smart-pointers/15.2-通过Deref将智能指针当作引用.md) |
| 访问 ≠ 释放 | `&*b` 借用不 Drop；只有所有者 `b` 出作用域才释放 → `cargo run -- obrm` |

---

## 本章（Nomicon 06）讲什么、不讲什么

| ✅ 本章讲（Perils） | ❌ 本章不重复讲 |
|---------------------|-----------------|
| 递归 Drop · `ManuallyDrop` · `take` | RAII/OBRM 概念全貌 → **15.3.0** |
| `mem::forget` 与泄漏 | Drop 基础语法 → **15.3** |
| 代理类型（Drain/Rc）放大泄漏 | Socket RAII 工程例 → **15.3.2** |
| panic Guard · Mutex poison | |

```text
15.3.0 懂 OBRM 是什么
  → 15.3 / 15.3.2 看 Drop 怎么用
  → Nomicon 06 看 OBRM 用错了会怎样（forget / 代理 / 展开 / 投毒）
  → 07 Concurrency
```

---

## 两个 demo 对照

### Book：所有者 vs 借用

```bash
cd 00-Book/15-smart-pointers/15.3-drop-demo
cargo run -- obrm
# 只有 Box 所有者出作用域才 LoudDrop::drop
```

### Nomicon：递归 Drop 顺序

```bash
cd 04-Rust-Nomicon/06_OBRM_RAII
cargo run -- --drop-order
# Outer body → Inner field …
```

---

## 极简速记

1. **RAII** — 构造拿、析构放。  
2. **OBRM** — RAII + 唯一所有权 + 编译检查。  
3. **`&` / `Deref`** — 只访问，**不改变** Drop 时机。  
4. **Nomicon 06** — OBRM 的**风险面**，不是入门定义章。

→ 下一节：[01 构造与析构](./01-construct-drop.md) · 速记：