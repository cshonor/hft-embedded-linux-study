# Item 16 · 案例与代码

← [Item 16 目录](./README.md)

### 标准库：内部 unsafe、对外 safe

| 类别 | 例子 |
|------|------|
| 智能指针 | `Rc` / `RefCell` / `Arc` — 内部裸指针 |
| 同步原语 | `Mutex` / `RwLock` — OS 级调用 |
| 内存原语 | `Pin`、`Cow`、`mem::take` / `swap` / `replace` |

### crates.io 优质替代（优先复用）

| crate | 用途 |
|-------|------|
| `once_cell` / `std::sync::OnceLock` | 单次初始化全局 |
| `rand` | OS / 硬件随机源 |
| `byteorder` | 字节序 ↔ 数值 |
| `cxx` | Rust ↔ C++ 互操作（类型安全边界） |

---
