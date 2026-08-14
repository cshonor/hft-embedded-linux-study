# 7.1 包与 Crate demo

笔记：[7.1](../7.1-包和crate.md) · … · [7.1.4](../7.1.4-src-bin与多exe.md)

---

## 一、Package 布局（7.1 / 7.1.4）

| 目录 | 结构 | 命令 |
|------|------|------|
| `only_bin/` | 仅 `main.rs`（默认 1 个 exe） | `cargo run` |
| `only_lib/` | 仅 `lib.rs` | `cargo build` |
| `bin_plus_lib/` | `lib.rs` + `main.rs` + `math/` | `cargo run` |
| `multi_bin/` | 1 lib + main + 每个 `bin/*.rs` 各 1 exe | `cargo run --bin cli1` |

`src/bin/` 详解 → [7.1.4](../7.1.4-src-bin与多exe.md)

---

## 二、`pub` 与跨项目依赖（7.1.1）

| 目录 | 说明 | 命令 |
|------|------|------|
| `user_demo/` | `path` 依赖 `only_lib` | `cargo run` → 30 |
| `compile_fail/private_mod/` | `mod math;` 无 `pub` | `cargo check` → E0603 |

---

## 三、main 调 `a.rs`（7.1.2）

| 目录 | 结构 | 命令 |
|------|------|------|
| `via_lib/` | 有 lib：`lib.rs` 中转导出 | `cargo run` |
| `via_main_only/` | 无 lib：`main.rs` 里 `mod a` | `cargo run` |

---

## 四、Actix-web 式目录骨架（7.1.3）

| 目录 | 说明 | 命令 |
|------|------|------|
| `webserver_layout/` | 1 lib + 1 main · domain/routes · `tests/api/` | `cargo run` · `cargo test` |

```bash
cd webserver_layout && cargo run && cargo test
```

```bash
# 布局
cd only_bin && cargo run
cd ../bin_plus_lib && cargo run

# main 调 a.rs
cd ../via_lib && cargo run
cd ../via_main_only && cargo run

# 反面
cd ../bin_plus_lib/compile_fail/private_mod && cargo check
```
