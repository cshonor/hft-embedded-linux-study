# 02 · Data Representation in Rust

> **The Rustonomicon** · [03 Rust Nomicon](../README.md) · [全书笔记](../notes.md)

## 状态

- [x] 已读（笔记整理）
- [x] 示例 crate（repr / DST / ZST / niche）

---

## 一句话

**布局章** — 默认 `repr(Rust)` 的对齐、重排与 niche 优化；DST / ZST / 空类型；`repr(C)` / `transparent` / `packed` / `align` 等替代布局。

---

## 专项笔记

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| 1 | `repr(Rust)` | [01-repr-rust.md](./01-repr-rust.md) |
| 2 | DST / ZST / 空类型 | [02-exotic-types.md](./02-exotic-types.md) |
| 3 | 替代 `repr` | [03-alt-repr.md](./03-alt-repr.md) |
| — | 速记 · 自测 |

---

## 示例源码

| 文件 | 演示 |
|------|------|
| [src/repr_rust.rs](./src/repr_rust.rs) | `repr(Rust)` vs `repr(C)`、niche |
| [src/exotic.rs](./src/exotic.rs) | DST 胖指针、ZST、Void |
| [src/repr_alt.rs](./src/repr_alt.rs) | `repr(C)` / `transparent` / `u8` / `packed` / `align` |
| [src/main.rs](./src/main.rs) | 运行入口 |

```bash
cd 04-Rust-Nomicon/02_Data_Layout
cargo run
```

---

## 与仓库其他部分

| 主题 | 对照 |
|------|------|
| 对齐与三种 repr | [RFR 02-layout](../../02-RFR/Chapter-02-Types/02-layout.md) |
| 实测输出 | [layout-demo](../../02-RFR/Chapter-02-Types/layout-demo/) |
| 上一章 | [01_Safe_Unsafe](../01_Safe_Unsafe/README.md) |
| 下一章 | [03_Lifetime_Variance](../03_Lifetime_Variance/README.md) · 生命周期 |

---

## 逻辑脉络

默认布局与 niche → 非常规尺寸类型 → 按需选用 `repr` → 进入 Ownership and Lifetimes。

---

## 速记

## 三句背诵

1. **`repr(Rust)` 默认可重排字段、插 padding；不要假设字段顺序。**
2. **DST 只能跟胖指针；ZST 偏移是 no-op、alloc(0) 危险。**
3. **跨 C/固定二进制格式 → `repr(C)`；勿滥用 `packed`。**

## 自测

- [ ] 能解释 `Option<&T>` 为何与 `&T` 同尺寸
- [ ] 能说出 ZST 在 MyVec 中的特殊处理（见 ch08）
- [ ] 能对照 [repr_rust.rs](./src/repr_rust.rs) 读出 `repr(C)` 与默认布局 size 差异

## 术语表

| 术语 | 含义 |
|------|------|
| niche | 利用无效位模式省 tag（如 null = None） |
| DST | 编译期未知 size 的类型 |
| ZST | size = 0 的类型 |
| padding | 对齐填充字节 |

