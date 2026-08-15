# 第 2 章 — Rust 及其生态系统简介（Introduction to Rust and its Ecosystem）

> 《Network Programming with Rust》· 网络编程前的语言基础  
> 阶段目录：[stage02_rust_basis](../README.md) · 并发深挖：仓库 [`01-atomic/`](../../../01-atomic/) · 异步：[`02-async_tokio/`](../../../02-async_tokio/)

本章讲解 Rust 核心概念与独特机制，为后续用 Rust 写网络程序打语言地基。

---

## 1. Rust 生态系统与入门

| 组件 | 作用 |
|------|------|
| **`rustc`** | Rust 编译器；发布渠道：**nightly**（最新实验）、**beta**、**stable**（推荐生产与学习） |
| **`rustup`** | 官方工具链安装器，管理多版本 `rustc`、组件（clippy、rustfmt 等） |
| **`Cargo`** | 包管理 + 构建：新建项目、依赖、编译、测试、运行 |

### Cargo 常用流程

```bash
cargo new my_net_app
cd my_net_app
# 在 Cargo.toml 的 [dependencies] 里添加 crates.io 上的 crate，例如 term
cargo build
cargo run
```

- **crates.io**：社区包注册中心，在 `Cargo.toml` 声明版本后由 Cargo 拉取。  
- 书中示例可能引入如 **`term`** 等 crate 做终端输出；网络项目中常见 **`tokio`**、**`serde`**、**`bytes`** 等，思路相同。

**与网络编程的关系**：几乎每个 Rust 网络项目都是 Cargo 工程；先熟悉 `cargo run` / `cargo test`，再写 Socket 代码会顺很多。

---

## 2. 借用检查器与所有权（Borrow Checker）

Rust **无 GC** 仍能保证内存安全，靠的是**所有权 + 借用 + 生命周期**，在**编译期**拦住悬垂指针和数据竞争。

### 所有权（Ownership）

- 每个值在任意时刻有**唯一**所有者。  
- 所有者离开作用域 → 值被 **drop**（释放资源）。  
- **Move**：堆上复杂类型（如 `String`、`Vec`）赋值/传参时**转移所有权**，旧绑定失效。  
- **Copy**：栈上简单类型（如 `i32`、`bool`）按位拷贝，原变量仍可用。

```rust
let s1 = String::from("hello");
let s2 = s1;   // s1 不再可用（move）
// println!("{}", s1); // 编译错误
```

### 借用（Borrowing）

通过**引用**临时访问而不拿走所有权：

| 类型 | 写法 | 规则（同一作用域内） |
|------|------|----------------------|
| 不可变借用 | `&T` | 可有**多个** |
| 可变借用 | `&mut T` | **至多一个**；且不能与任何 `&T` 共存 |

→ 编译期排除「同时读写」类数据竞争，是「无畏并发」的语法基础。

### 生命周期（Lifetimes）

标注引用**能活多久**（如 `'a`、`'static`），避免**悬垂引用**。

```rust
fn longest<'a>(x: &'a str, y: &'a str) -> &'a str {
    if x.len() > y.len() { x } else { y }
}
```

- **`'static`**：整个程序期间都有效（如字符串字面量、泄漏的 `Box`）。  
- **网络里常见**：`thread::spawn` 要求闭包 `'static`；异步任务也常要求 `'static` Future（见 `02-async_tokio/`）。

更系统：[`atomic/Chapter-01`](../../../01-atomic/Chapter-01-Rust-Concurrency-Basics/本章学习笔记.md) §1.3～1.6。

---

## 3. 泛型与特征（Generics & Traits）

### 泛型

用 `<T>` 写一份代码，复用于多种类型：

```rust
fn print_twice<T: std::fmt::Display>(x: T) {
    println!("{x} {x}");
}
```

### 特征（Traits）

定义**类型共有的行为**（类似其他语言的 interface）：

- 为自定义类型实现标准特征：`Display`、`Debug`、`Add` 等。  
- 可**自定义 trait**，再 `impl Trait for MyType`。  
- 泛型上常写 **trait bound**：`T: Read + Write`。

**与网络编程的关系**：`Read` / `Write` / `AsyncRead` / `AsyncWrite` 等都是 trait；协议解析、缓冲封装都建立在 trait 抽象上。

---

## 4. 错误处理（Error handling）

Rust **不默认忽略错误**，用枚举显式表达「可能失败」。

| 类型 | 含义 | 变体 |
|------|------|------|
| **`Result<T, E>`** | 可恢复错误 | `Ok(T)` / `Err(E)` |
| **`Option<T>`** | 可能缺失 | `Some(T)` / `None` |

常用写法：

```rust
use std::fs::File;
use std::io::{self, Read};

fn read_config(path: &str) -> io::Result<String> {
    let mut f = File::open(path)?;  // ? 把 Err 向上返回
    let mut s = String::new();
    f.read_to_string(&mut s)?;
    Ok(s)
}
```

| 方式 | 适用 |
|------|------|
| **`?`** | 库代码、可传播错误 |
| **`match` / `if let`** | 需要分支处理 |
| **`unwrap()` / `expect()`** | 原型、确定不会失败时（生产慎用） |
| **`panic!`** | 不可恢复、逻辑不应到达的分支 |

**网络 I/O**：`connect`、`read`、`write` 几乎都返回 `Result`；异步里还有 `io::ErrorKind`（对端重置、超时等）。习惯在边界用 `?`，在 `main` 或顶层统一 `fn main() -> Result<(), Box<dyn Error>>`。

---

## 5. 宏系统（The macro system）

- 调用形式常带 **`!`**：`println!`、`vec!`、`panic!`。  
- **卫生宏（Hygienic）**：宏里定义的变量默认不「泄漏」污染调用方作用域。

| 种类 | 定义方式 | 特点 |
|------|----------|------|
| **声明宏（语法宏）** | `macro_rules!` | 模式匹配 + 模板展开 |
| **过程宏** | `#[derive(...)]`、`#[tokio::main]` 等 | 操作 AST，用于 derive、属性宏、函数式宏 |

网络代码里常见：`println!`、`format!`、`async_trait` / `tokio::select!` 等；理解「宏在编译期展开成普通 Rust」即可，不必先写复杂宏。

---

## 6. 函数式特性（Functional features）

### 闭包（Closures）

匿名函数，可捕获环境：

```rust
let add = |a, b| a + b;
let v = vec![1, 2, 3];
let sum: i32 = v.iter().fold(0, |acc, x| acc + x);
```

- **`move`**：强制闭包**取得**捕获变量的所有权（`thread::spawn`、异步任务里极常见）。

### 迭代器（Iterators）

- **惰性**：适配器链（`map`、`filter`）在 `collect` / `for` 消费时才真正执行。  
- 自定义类型实现 **`Iterator`**，提供 `next() -> Option<Item>`，即可用 `for` 与适配器。

```rust
for n in 1..5 {
    println!("{n}");
}
```

**网络场景**：按行读 socket（`lines()`）、解析协议字段、批量处理连接 ID 等，迭代器 + `collect` 很常用。

---

## 7. 并发原语（Concurrency primitives）

Rust 目标：**无畏并发（fearless concurrency）**。

| 要点 | 说明 |
|------|------|
| **1:1 线程模型** | 一个 Rust 线程 ≈ 一个 OS 线程 |
| **创建** | `std::thread::spawn` |
| **消息传递** | `mpsc::channel`：发送端 / 接收端，常比共享内存更易推理 |
| **共享状态** | `Arc<Mutex<T>>` 等 + **`Send`** / **`Sync`** trait 标记可否跨线程 |

- **`Send`**：所有权可安全移到另一线程。  
- **`Sync`**：可通过 `&T` 在多线程间共享（即 `&T` 是 `Send`）。

在借用规则下，非法跨线程共享会在**编译期**报错。

**`unsafe`**：可绕过部分检查，由程序员保证不变量；FFI、自写数据结构、与 C 库交互时会遇到。网络栈底层或零拷贝缓冲可能涉及，但应用层应尽量少用。

**本仓库延伸**：

| 主题 | 路径 |
|------|------|
| 线程、`Mutex`、`Condvar` | [`atomic/Chapter-01`](../../../01-atomic/Chapter-01-Rust-Concurrency-Basics/) |
| 异步、`spawn`、`JoinHandle` | [`02-async_tokio/`](../../../02-async_tokio/) |

---

## 8. 测试（Testing）

| 方式 | 写法 | 作用 |
|------|------|------|
| **单元测试** | 模块内 `#[cfg(test)] mod tests { #[test] fn ... }` | `cargo test` |
| **集成测试** | 项目 `tests/*.rs` | 测对外 API |
| **文档测试** | `///` 或 `//!` 代码块中的示例 | 保证文档示例可编译、可运行 |

```rust
#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
    }
}
```

网络项目里常见：用 `std::net` 起本地回环服务测协议；异步用 `tokio::test`；大流量用集成测试 + 模拟对端。

---

## 本章自检（Stage 02）

- [ ] 能说明 `rustc` / `rustup` / `Cargo` 各做什么  
- [ ] 能解释 move vs copy，以及 `&` / `&mut` 借用规则  
- [ ] 能读懂 `Result` + `?` 在 I/O 函数里的用法  
- [ ] 知道 `trait` 与泛型 bound 在网络抽象（`Read`/`Write`）里的角色  
- [ ] 能区分 `mpsc` 与 `Arc<Mutex<_>>` 两种并发风格  
- [ ] 知道 `Send`/`Sync` 与 `thread::spawn`、Tokio 任务的关系  

---

## 下一步

| 方向 | 路径 |
|------|------|
| 书 Ch1 网络概念 | [stage01 — Ch01 笔记](../../stage01_network_basic/notes/Ch01-客户端服务器网络简介-学习笔记.md) |
| 阻塞 TCP/UDP | [stage03_std_tcp_udp](../../stage03_std_tcp_udp/README.md) |
| 异步网络 | [stage07_tokio_async_net](../../stage07_tokio_async_net/README.md) |
