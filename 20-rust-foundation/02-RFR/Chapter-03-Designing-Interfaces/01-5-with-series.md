# 1.1.5 · `with_` 系列 — 建造者模式

> 所属：**Unsurprising / 命名惯例** · [← 01 命名 hub](./01-naming-practices.md)

← [01-4 `try_`](./01-4-try-series.md) · 下一节 [02 通用 Trait](./02-common-traits-for-types.md)

Demo → `cargo run --manifest-path naming-series-demo/Cargo.toml with`

> ER [Item 07 Builders](../../01-ER/Chapter-01-Types/Item-07-builders/README.md) · Book [8.1 Vec](../../00-Book/08-collections/8.1-vector.md)

---

## 核心逻辑

**`with_xxx`** 用**方法链**替代「参数很多、顺序易错」的构造函数 — 可读、可组合、可省略默认值。

| 形态 | 典型 |
|------|------|
| **关联函数** | `Vec::with_capacity(n)` — 构造时带关键参数 |
| **Builder 链** | `.with_port(443).with_tls(true).build()` |
| **生态链式** | `Client::new().get(url).header(...).send()` — 同思路 |

```text
new()           → 默认值 / 必填项
.with_xxx()      → 改一项，返回 Self 继续链
.build() / send → 消耗 builder，产出最终对象
```

| 系列对比 | 阶段 | 所有权 |
|----------|------|--------|
| **`with_*`** | 构造 / 配置 | 新建或 builder 中间态 |
| **`as_*`** | 已存在对象 | 借用 |
| **`into_*`** | 已存在对象 | 消耗 |

---

## 可运行示例 1：`Vec::with_capacity`

`Vec::new()` 从空容量开始；`push` 超限会 realloc + 拷贝。已知约 **N** 个元素时预分配：

```rust
const N: usize = 1_000_000;

let mut v = Vec::with_capacity(N);
for i in 0..N {
    v.push(i);
}
assert_eq!(v.len(), N);
assert!(v.capacity() >= N);
```

| 点 | 说明 |
|----|------|
| **容量 ≠ 长度** | `with_capacity(N)` 后 `len()` 仍为 0 |
| **同类** | `String::with_capacity` · `HashMap::with_capacity` · `Vec::reserve` |

---

## 可运行示例 2：手写 `with_*` Builder

```rust
#[derive(Debug, PartialEq)]
struct ServerConfig {
    host: String,
    port: u16,
    workers: usize,
    tls: bool,
}

struct ServerConfigBuilder {
    host: String,
    port: u16,
    workers: usize,
    tls: bool,
}

impl ServerConfigBuilder {
    fn new(host: impl Into<String>) -> Self {
        Self {
            host: host.into(),
            port: 8080,
            workers: 4,
            tls: false,
        }
    }

    fn with_port(mut self, port: u16) -> Self {
        self.port = port;
        self
    }

    fn with_workers(mut self, n: usize) -> Self {
        self.workers = n;
        self
    }

    fn with_tls(mut self, enable: bool) -> Self {
        self.tls = enable;
        self
    }

    fn build(self) -> ServerConfig {
        ServerConfig {
            host: self.host,
            port: self.port,
            workers: self.workers,
            tls: self.tls,
        }
    }
}

let cfg = ServerConfigBuilder::new("127.0.0.1")
    .with_port(443)
    .with_tls(true)
    .with_workers(8)
    .build();
```

→ demo：`naming-series-demo/src/with_series.rs`

---

## 可运行示例 3：reqwest 链（生态 Builder）

```rust
use std::time::Duration;

async fn fetch_api(url: &str, token: &str) -> reqwest::Result<String> {
    reqwest::Client::new()
        .get(url)
        .header("Authorization", format!("Bearer {token}"))
        .timeout(Duration::from_secs(10))
        .send()
        .await?
        .text()
        .await
}
```

无字面量 `with_`，但 **Client → 配置链 → send 消费** 与 Builder 同构。

---

## 何时用

| 场景 | 建议 |
|------|------|
| 已知元素个数 | `with_capacity` / `reserve` |
| 可选字段 ≥ 3 | `XxxBuilder` + `with_*` |
| 1～2 个参数 | `new` 即可，别过度 Builder |

---

## 速记

**口诀**：构造 / 配置 · 链式 `with_*` → `build` · `with_capacity` · reqwest `.header().send()`

→ 下一节：[02 通用 Trait](./02-common-traits-for-types.md)
