# Item 35 · 案例与代码

← [Item 35 目录](./README.md)

### Allowlist 提取大库子集

```bash
bindgen --allowlist-function="add.*" \
        --allowlist-type=FfiStruct \
        -o src/generated.rs \
        lib.h
```

### 引入生成文件

```rust
include!("generated.rs");
```

或在 **`build.rs`** 里构建时生成（§6）。

### `build.rs` 骨架（待深入）

```rust
// build.rs — 概念示例
let bindings = bindgen::Builder::default()
    .header("lib.h")
    .allowlist_function("add.*")
    .generate()?;
bindings.write_to_file(out_dir.join("generated.rs"))?;
```

`cargo build` 时扫描本地头文件 → 适应跨平台路径。

---
