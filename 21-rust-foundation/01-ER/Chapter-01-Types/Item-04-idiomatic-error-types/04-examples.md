# Item 4 · 案例与代码

← [Item 4 目录](./README.md)

### Newtype + 空 `Error` impl

```rust
#[derive(Debug)]
pub struct MyError(String);

impl std::fmt::Display for MyError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.0)
    }
}

impl std::error::Error for MyError {}
```

### 穷尽枚举 + 溯源

```rust
#[derive(Debug)]
pub enum MyError {
    Io(std::io::Error),
    Utf8(std::string::FromUtf8Error),
    General(String),
}

impl std::error::Error for MyError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match self {
            MyError::Io(e) => Some(e),
            MyError::Utf8(e) => Some(e),
            MyError::General(_) => None,
        }
    }
}
```

### `From` + `?`（示意）

```rust
impl From<std::io::Error> for MyError {
    fn from(e: std::io::Error) -> Self {
        MyError::Io(e)
    }
}

fn read_config() -> Result<Config, MyError> {
    let s = std::fs::read_to_string("cfg.toml")?; // 自动 Io → MyError
    Ok(parse(&s)?)
}
```

### `thiserror`：自动实现 `Error`（库推荐）

手写上面那些 `impl Display` / `impl Error` / `source()` 很重复。`thiserror` 用宏帮你生成：

```rust
use thiserror::Error;

#[derive(Debug, Error)]
enum HftError {
    #[error("Network timeout when connecting to exchange")]
    NetworkTimeout,

    #[error("Order rejected: {0}")]
    OrderRejected(String),

    #[error("IO error")]
    Io(#[from] std::io::Error), // 自动生成 From<io::Error>，配合 ? 使用
}
```

- `#[derive(Error)]` → 自动 `impl std::error::Error`
- `#[error("...")]` → 自动生成 `impl Display`（`{}` 打印的内容）
- `#[from]` → 自动生成 **`From`** + **`source()` 指向该字段**（单父指针）；`?` 可直接向上传播 → [RFR §thiserror #[from]](../../../02-RFR/Chapter-04-Error-Handling/01-error-source-chain.md#thiserrorsourcefrom-自动挂父指针)

对比手写：功能等价，代码量通常少一大半。注意库对外 API 尽量暴露**具体 enum 类型**，不要把 `thiserror` 的宏细节 leak 给下游（见 Item 4 重点结论）。

---
