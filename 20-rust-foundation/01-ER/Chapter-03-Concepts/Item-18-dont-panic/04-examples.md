# Item 18 · 案例与代码

← [Item 18 目录](./README.md)

### 反模式：`catch_unwind` 掩盖除零

```rust
fn divide_recover(a: i64, b: i64, default: i64) -> i64 {
    let result = std::panic::catch_unwind(|| divide(a, b));
    match result {
        Ok(x) => x,
        Err(_) => default, // panic=abort 时进程直接死，走不到这里
    }
}
```

正解：`divide` 返回 `Result<i64, DivideError>`，由调用者决定默认值或向上传播。

### 隐性 panic 来源（不写 `panic!` 也会 panic）

| 来源 | 例子 |
|------|------|
| 显式 | `panic!`、`unreachable!` |
| `Option`/`Result` | `.unwrap()`、`.expect()`、`.unwrap_err()` |
| 边界检查 | `slice[i]` 越界 |
| 算术 | 调试 build 下 `x / 0`（release 行为见语言/优化） |

→ 不能靠 Code Review 盯；用类型、`Result`、测试、**`no_panic`**（§6）交给机器。

---
