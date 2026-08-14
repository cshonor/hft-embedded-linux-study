存放**同一 Rust 源**在优化等级、Pass 或内存序约束前后的 IR **对比**（可配合 `diff` / `meld`）。

建议子命名：`同一basename_O0.ll`、`同一basename_O3.ll`。

## 已有样本

| 文件 | 源 | 看点 |
|------|-----|------|
| `04_dispatch_O0.ll` | `lib.rs` · `process_static` / `process_dyn` | 单态化 **直接 call** vs vtable **间接 call** |
| `04_dispatch_O3.ll` | 同上 · `-C opt-level=3` | 与 O0 对比优化是否 inline / 消除间接 |

笔记 → [`part02/chapter04/notes/04_dispatch_static_vs_dyn.md`](../../part02_src_to_machine/chapter04_ir_basic/notes/04_dispatch_static_vs_dyn.md)
