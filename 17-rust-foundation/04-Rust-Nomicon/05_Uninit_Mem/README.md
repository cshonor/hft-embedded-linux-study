# 05 · Working With Uninitialized Memory

> **The Rustonomicon** · [03 Rust Nomicon](../README.md) · [全书笔记](../notes.md)

## 状态

- [x] 已读（笔记整理）
- [x] 示例 crate（checked / drop flags / MaybeUninit / ptr）

---

## 一句话

**未初始化内存章** — Safe 分支分析 vs 逻辑 move-out、Drop flags、MaybeUninit 标准路径、`ptr::write` / `copy` 底层操作。

---

## 专项笔记

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| 1 | 安全受检 | [01-checked.md](./01-checked.md) |
| 2 | drop flags | [02-drop-flags.md](./02-drop-flags.md) |
| 3 | 非受检（MaybeUninit + ptr） | [03-unchecked.md](./03-unchecked.md) |
| 4 | 与后续 Vec 关系 | [04-vec-prelude.md](./04-vec-prelude.md) |
| — | 速记 · 自测 |

---

## 示例源码

| 文件 | 演示 |
|------|------|
| [src/checked.rs](./src/checked.rs) | 编译期受检的条件初始化 |
| [src/drop_flags.rs](./src/drop_flags.rs) | 条件 Drop（drop flags 直觉） |
| [src/maybe_uninit.rs](./src/maybe_uninit.rs) | `MaybeUninit` 数组、Vec `set_len` |
| [src/ptr_ops.rs](./src/ptr_ops.rs) | `write` / `copy` / `copy_nonoverlapping` |
| [src/main.rs](./src/main.rs) | 运行入口 |

```bash
cd 04-Rust-Nomicon/05_Uninit_Mem
cargo run
cargo run -- --drop-flags   # 观察条件 Drop 输出
```

---

## 与仓库其他部分

| 主题 | 对照 |
|------|------|
| MaybeUninit | [RFR 03-calling-unsafe](../../02-RFR/Chapter-09-Unsafe-Code/03-calling-unsafe-functions.md) |
| ptr::write | [Book 19.1 unsafe](../../00-Book/19-advanced-features/19.1-不安全Rust.md) |
| 上一章 | [04_Type_Cast](../04_Type_Cast/README.md) |
| 下一章 | [06_OBRM_RAII](../06_OBRM_RAII/README.md) · OBRM |

---

## 逻辑脉络

Safe 受检未初始化 → Drop flags → MaybeUninit / ptr 突破 Safe 限制 → 进入 OBRM。

---

## 速记

## 三句背诵

1. **Safe 用分支分析阻止读未初始化栈变量；move-out 后原变量逻辑未初始化。**
2. **Drop flags 跟踪「是否该 Drop」；复杂路径才需运行时 flag。**
3. **MaybeUninit 不 Drop 内部；ptr::write 盲写绕过 Drop — 新代码勿用 `uninitialized`。**

## 自测

- [ ] 能解释条件初始化为何需要全路径赋值
- [ ] 能说明 drop flags 何时有运行时开销
- [ ] 能对比 `MaybeUninit` 与普通变量的 Drop 行为
- [ ] 能说出 `ptr::write` / `copy` / `copy_nonoverlapping` 的分工
- [ ] 能描述 Vec 增长如何组合本章 API（见 [04-vec-prelude.md](./04-vec-prelude.md)）

## 术语表

| 术语 | 含义 |
|------|------|
| 逻辑未初始化 | move-out 后变量槽位不可再读 |
| drop flag | 栈上跟踪值是否已初始化、是否应 Drop |
| MaybeUninit | 合法标记「可能未初始化」的包装类型 |
| assume_init | 断言已初始化后转为 `T`（误用即 UB） |
| ptr::write | 向可能未初始化地址写入，不 Drop 旧值 |

