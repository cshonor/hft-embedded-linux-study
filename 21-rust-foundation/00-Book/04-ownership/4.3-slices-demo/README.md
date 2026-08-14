# 4.3 切片 demo — 编译对比

笔记：[4.3-切片slice.md](../4.3-切片slice.md)（子篇：[4.3.2 借用规则](../4.3.2-切片与借用规则.md) · [4.3.1 usize](../4.3.1-usize下标与悬空索引.md)）

## 运行主 demo

```bash
cargo run
cargo test
```

## `usize` 悬空索引 vs `&str` 借用（不用 playground，本地即可）

### 1. 切片方案：`clear` 编译失败（推荐先跑）

```bash
rustc --edition 2021 compile_fail/slice_blocks_clear.rs
```

应看到类似：`cannot borrow ... as mutable because it is also borrowed as immutable`。

### 2. 静态 `&str` vs 堆上临时 `&str`（`cargo run` 打印）

主程序 `static_vs_heap_str_demo()`：`"literal"` → `&'static str`；`&st[..]` → 绑定 `String` 生命周期。

### 3. 堆上 `&str` 不能比 `String` 活得更久

```bash
rustc --edition 2021 compile_fail/part_outlives_string.rs
```

### 4. 禁止 `fn f() -> str`（DST 不能作返回值）

```bash
rustc --edition 2021 compile_fail/return_str_dst.rs
```

### 5. `usize` 方案：`clear` 能编译，切片才 panic

```bash
cargo test dangling_usize_panics_at_runtime
```

演示：`idx = 5` 在 `s.clear()` 后仍「存在」，再 `&s[0..idx]` 运行时越界。
