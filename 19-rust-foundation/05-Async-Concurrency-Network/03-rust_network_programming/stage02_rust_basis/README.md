# Stage 02 — Rust 语言与生态前置

**对应书籍**：第 2 章 — *Introduction to Rust and its Ecosystem*（Rust 及其生态系统简介）

## 章节目录（原书 10 节）

| 书 § | 英文 | 中文 |
|------|------|------|
| 2.1 | The Rust ecosystem | Rust 生态系统 |
| 2.2 | Getting started with Rust | Rust 入门 |
| 2.3 | Introduction to the borrow checker | 借用检查器简介 |
| 2.4 | Generics and the trait system | 泛型与特征系统 |
| 2.5 | Error handling | 错误处理 |
| 2.6 | The macro system | 宏系统 → 语法宏 / 过程宏 |
| 2.7 | Functional features in Rust | 函数式特性 → 高阶函数 / 迭代器 |
| 2.8 | Concurrency primitives | 并发原语 |
| 2.9 | Testing | 测试 |
| 2.10 | Summary | 总结 |

**脉络**：2.1～2.2 入门 → 2.3～2.5 核心机制 → 2.6～2.7 进阶语法 → 2.8～2.9 并发与测试 → 2.10 收尾。

## 学习定位

- **查漏补缺**：已有 [`01-atomic/`](../../01-atomic/)、[`02-async_tokio/`](../../02-async_tokio/) 时，重点 **§2.5、§2.8** 与网络 I/O。  
- 遇编译错误按上表回查对应 §。

## 优先级与代码

| 项目 | 建议 |
|------|------|
| 优先级 | **低～中** |
| 是否必写 Demo | **否** |

## 笔记

| 资料 | 说明 |
|------|------|
| [本章学习笔记.md](./本章学习笔记.md) | **§2.1～2.10 索引表** |
| [Ch02 精读笔记](notes/Ch02-Rust及其生态系统简介-学习笔记.md) | 按书 10 节组织的正文 |

## 学习检查

- [ ] §2.5：`Result` + `?` 在 I/O 边界  
- [ ] §2.8：`Send` / `Sync` / `'static` 与 `spawn`  
- [ ] §2.4：`Read`/`Write` trait  
