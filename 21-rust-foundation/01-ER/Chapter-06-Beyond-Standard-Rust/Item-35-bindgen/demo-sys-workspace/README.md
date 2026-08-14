# Item 35：`-sys` + safe 双层 workspace

```text
er-add-sys/   bindgen + C，仅 raw FFI
er-add/       safe 封装，业务依赖此 crate
```

```bash
cd 01-ER/Chapter-06-Beyond-Standard-Rust/Item-35-bindgen/demo-sys-workspace
cargo test --workspace
```

对照 [item-35-bindgen](../item-35-bindgen/)（单 crate 内封装）与本模板（生产惯用拆分）。
