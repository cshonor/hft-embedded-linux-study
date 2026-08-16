# 第 6 章 — 在互联网上进行 HTTP 通信（Talking HTTP in the Internet）

> **按节学习（笔记 + demo）**：[本章学习笔记.md](../本章学习笔记.md) → `6.1-introducing-hyper.md` … `6.4-summary.md`  
> 下文为**全章贯通稿**（与各节文件内容重叠处以便连续阅读，扩展时优先改各节 `.md`）。

> 《Network Programming with Rust》· HTTP 是 WWW 的骨干  
> 阶段目录：[stage06_http_programming](../README.md) · 异步网络：[stage07](../../stage07_tokio_async_net/README.md) · Serde：[Ch04](../../stage04_serde_parse/notes/Ch04-数据序列化反序列化与解析-学习笔记.md)

本章介绍用 Rust 及生态库编写 **HTTP 服务器**与**客户端**，重点三个 crate：**Hyper**、**Rocket**、**reqwest**。

---

## HTTP 在栈中的位置（复习）

```text
应用：HTTP 语义（方法、头、状态码、Body）
  ↓
传输：TCP（常见）或 TLS（HTTPS）
  ↓
… Ch1 分层、Ch3 Socket …
```

- **HTTP/1.1**：文本头 + 长度或 chunked 体；一问一答为主（可 keep-alive）。  
- **HTTP/2**：多路复用、头部压缩；**gRPC 默认跑在 H2 上**（见 [Ch05](../../stage05_app_protocol/notes/Ch05-应用层协议-学习笔记.md)）。  
- 本章以 **HTTP/1.1 风格的服务/客户端 API** 为主；现代 `hyper` 已支持 H1/H2。

---

## 1. 使用 Hyper 编写 HTTP 服务器

### Hyper 简介

**Hyper** 是 Rust 生态里成熟、底层的 **HTTP 实现/库**（偏框架基石，而非「自带路由的全家桶」）。

| 特点 | 说明 |
|------|------|
| 角色 | 解析/生成 HTTP、驱动连接；常与 **Tower** 中间件组合 |
| 并发模型 | 服务端基于 **Tokio + async/await**（书中自 `tokio-core`/`futures` 时代演进而来） |
| 适用 | 高并发、需精细控制连接与协议的场景 |

**现代对照**：书内可能仍出现 `tokio-core`、`net2` 等旧名；新项目用 **`tokio` 1.x + `hyper` 1.x`**，概念不变（事件循环、非阻塞 I/O）。

### 单线程 Hyper 服务器

- 一个运行时线程处理 accept + 读写的 **poll**。  
- **优点**：模型简单，便于理解 `Service`/`Handler` 回调链。  
- **缺点**：单核 CPU 打满后吞吐受限，无法吃满多核。

```text
客户端请求 → Hyper 解析 HTTP → 你的 service 返回 Response
```

### 多线程 / 多监听：压榨多核

书中扩展路径：

1. **`tokio` 运行时**：在多个线程上跑任务（或每个线程一个 runtime，视版本与模式而定）。  
2. **`SO_REUSEPORT`（`net2` 等设置）**：多个线程/进程在**同一端口**上 `bind`，内核把新连接**分摊**到不同监听套接字 → 连接级负载均衡。  
3. 结果：书中示例 **吞吐约翻倍**（视硬件与负载而定）。

```text
         ┌─ thread 1 ─ Hyper + Tokio
:8080 ───┼─ thread 2 ─ Hyper + Tokio   （REUSEPORT）
         └─ thread N ─ …
```

**心智**：这与 Ch3「每连接一线程」不同，仍是 **异步 + 多路复用**，只是用多个 reactor 或 reuseport 吃多核。

**延伸**：[stage07](../../stage07_tokio_async_net/README.md) · 仓库 `02-async_tokio/`。

---

## 2. 使用 Rocket 构建 Web 应用和 API

### Rocket 简介

**Rocket**：高层 **Web 框架**，强调声明式路由与易用性（体验上常被比作 Python **Flask**）。

| 特点 | 说明 |
|------|------|
| 路由 | **`#[get("/path")]`、`#[post(...)]`** 等属性宏声明 |
| 编译器 | 书中时代常依赖 **nightly**（宏/特性需求）；新版本请查官方文档是否已 stable |
| 定位 | 快速做网站 / REST API，少写底层 HTTP 解析 |

### REST API + 数据库：`diesel` + `r2d2`

书中 **博客 CRUD** 示例典型栈：

| 组件 | 作用 |
|------|------|
| **`diesel`** | ORM：类型安全的 SQL、迁移、与 SQLite/Postgres 等对接 |
| **`r2d2`** | **连接池**：复用 DB 连接，避免每请求 `open` 一次 |
| **Rocket 路由** | `GET/POST/PUT/DELETE` 映射到 handler |

**Data Guards + Serde**：

- 入参 JSON 自动 **反序列化** 为结构体（非法 JSON/缺字段 → 400）。  
- 与 [Ch04](../../stage04_serde_parse/) 的 `Deserialize` 同一套机制，由框架在边界调用。

```rust
// 概念
#[post("/posts", data = "<post>")]
fn create(post: Json<NewPost>) -> Json<Post> { ... }
```

### 动态 HTML：`rocket_contrib` + Tera

- **`Tera`**：模板引擎（类似 Jinja2），在 HTML 里填动态变量。  
- **`rocket_contrib`**（书中）：集成模板渲染；新版本可能拆到独立 crate，以文档为准。

**Managed State + `AtomicUsize`**：

- Rocket 的 **托管状态**：应用级单例（如访问计数器）。  
- 用 **`AtomicUsize`** 实现**线程安全**自增，无需 `Mutex` 即可统计 PV（高并发下注意语义与溢出，仅演示足够）。

```text
REST JSON API  ←→  diesel + r2d2
动态 HTML      ←→  Tera 模板 + Managed State
```

**选型提示**：生产 Rust Web 还有 **axum**、**actix-web** 等；Rocket 重在理解「路由宏 + 提取器 + 状态」，不必绑死单一框架。

---

## 3. 使用 reqwest 编写 HTTP 客户端

### reqwest 简介

**reqwest**：高层 **HTTP 客户端**，API 设计接近 Python **`requests`**。

| 能力 | 说明 |
|------|------|
| 方法 | `get`、`post`、自定义头、超时、代理等 |
| 体 | JSON、表单、multipart |
| TLS | 通过 feature 启用 **rustls** / **native-tls** |

### 同步与异步

**同步**（阻塞当前线程，适合脚本/CLI）：

```rust
// 概念（reqwest 0.11+ 常见写法）
let client = reqwest::blocking::Client::new();
let resp = client.get("https://httpbin.org/get").send()?;
let body = resp.text()?;
```

**JSON + Serde**（与 Ch04 一致）：

```rust
#[derive(Serialize)]
struct Payload { name: String }

#[derive(Deserialize)]
struct ApiResult { ok: bool }

let res: ApiResult = client
    .post("https://api.example.com/items")
    .json(&Payload { name: "a".into() })
    .send()?
    .json()?;
```

**异步**（书中 `tokio_core` + `Future` 链；现代写法）：

```rust
#[tokio::main]
async fn main() -> Result<(), reqwest::Error> {
    let body = reqwest::get("https://httpbin.org/get")
        .await?
        .text()
        .await?;
    Ok(())
}
```

书中强调：**同一套 Serde** 贯穿 POST 序列化与 GET 反序列化，无需手写 Content-Type 与 body 拼接（框架会处理）。

### 与仓库其他示例

- 根 workspace / `async_tokio`：**`ch01_reqwest_join`** 等 — 并发发多个 HTTP 请求，可与本章客户端 API 对照。

---

## 三库分工（本章总表）

| Crate | 角色 | 典型用途 |
|-------|------|----------|
| **Hyper** | HTTP 协议 + 异步 I/O 底座 | 自建服务器、网关、底层代理 |
| **Rocket** | 全栈 Web 框架 | 博客 API、模板页、快速 REST |
| **reqwest** | HTTP 客户端 | 调第三方 API、爬虫、微服务间 HTTP |

```text
写服务：Hyper（底层） 或  Rocket（高层）
写客户端：reqwest
数据格式：Serde JSON（Ch4）
运行时：Tokio（Ch3 阻塞 → Ch6/7 异步）
```

---

## 本章自检（Stage 06）

- [ ] 能说明 Hyper 偏底层、Rocket 偏全栈、reqwest 偏客户端  
- [ ] 理解单线程 reactor 与 `SO_REUSEPORT` 多监听提升吞吐的思路  
- [ ] 知道 Rocket Data Guard 与 Serde 在入参校验上的关系  
- [ ] 会用 `reqwest` 发 GET/POST JSON（同步或 `async` 一种即可）  

---

## 学习建议

- 不做 Web 网关时：**先掌握 `reqwest` + Serde**，Hyper/Rocket 后移。  
- 与 **stage07** 一起学：Tokio 搞懂后，Hyper/reqwest 异步代码才顺手。

---

## 下一步

| 方向 | 路径 |
|------|------|
| Tokio 异步网络 | [stage07](../../stage07_tokio_async_net/README.md) |
| HTTPS / TLS | [stage08_network_security](../../stage08_network_security/README.md) |
| gRPC on HTTP/2 | [Ch05](../../stage05_app_protocol/notes/Ch05-应用层协议-学习笔记.md) |
