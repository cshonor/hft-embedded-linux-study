# 07 · Programming WebAssembly with Rust

> **口头简称**：`07_WebAssembly` / **Wasm 专题**  
> **书名**：*Programming WebAssembly with Rust: Unified Development for Web, Mobile, and Embedded Applications*  
> **作者**：Kevin Hoffman（Pragmatic Bookshelf, 2019）  
> **完整目录**：[Wasm-本书目录.md](./Wasm-本书目录.md) · **三层练手**：[三层学习架构.md](./三层学习架构.md) · **知识链**：[学习路径与知识链.md](./学习路径与知识链.md)

用 **Rust** 的安全性与性能构建 **WebAssembly (Wasm)** 应用 — 从 **WAT/Wast** 栈式虚拟机与**线性内存**，到 **wasm-bindgen**、**Yew**，再到浏览器外宿主（控制台、**树莓派**、机器人系统）。

---

## 三层练手主线（HFT 向）

> 原书 `chapter01_*`～`appendix/` 按书序记笔记；练手按下列三层推进，与 [06 LLVM](../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/README.md) 和 Go 订单簿项目衔接。

| 层 | 目录 | 目标 |
|:---:|------|------|
| **1** | [layer01_rust-llvm-to-wasm/](./layer01_rust-llvm-to-wasm/README.md) | **Rust + LLVM → Wasm** 编译链路，复用 `llvm_insight_lab` |
| **2** | [layer02_orderbook-query-wasm/](./layer02_orderbook-query-wasm/README.md) | Wasm 模块对接 **Go 订单簿** 查询 API |
| **3** | [layer03_quant-ma-strategy/](./layer03_quant-ma-strategy/README.md) | **均线策略**：Yew 浏览器回测 + wasmtime 实盘/边缘嵌入 |

详表 → [三层学习架构.md](./三层学习架构.md)

---

## 阅读定位

| 阶段 | 本仓库资源 | 与本书关系 |
|------|------------|------------|
| Rust 语法 | [`00-Book`](../00-Book/Book-本书目录.md) | 先能写 Rust 再编译到 Wasm |
| 内存 / unsafe | [`04-Rust-Nomicon`](../04-Rust-Nomicon/README.md) · [`02-RFR`](../02-RFR/RFR-本书目录.md) | 理解线性内存、指针边界 |
| 并发 / 性能直觉 | [`05-01-atomic`](../05-Async-Concurrency-Network/01-atomic/README-学习区.md) | 位布局、可预测性 — 见 [知识链 · HFT](./学习路径与知识链.md#与高频--性能优化的衔接) |
| 编译目标 | [`06 Compilers`](../06_Compilers-and-LLVM-Learning/README.md) | Wasm 作为 IR / 虚拟机对照 |
| **本书** | **本目录** | Rust → Wasm 全栈 + JS 互操作 + 非 Web 宿主 |

**建议时机**：**05-atomic 前若干章** 或 Nomicon 通读后；Part II 可与 **05-async / 网络** 并行；**06 LLVM 之前** 读完 Part I（栈式 VM + Checkers）收益最大。

---

## 三部分结构（原书）

| Part | 主题 | 章 | 三层 | 入口 |
|------|------|:---:|------|------|
| **I** | Building a Foundation | 1～2 | **L1** → **L2** | [chapter01_wasm_fundamentals/](./chapter01_wasm_fundamentals/README.md) · [chapter02_wasm_checkers/](./chapter02_wasm_checkers/README.md) |
| **II** | Interacting with JavaScript | 3～5 | **L2** · **L3** | [chapter03_rust_wasm/](./chapter03_rust_wasm/README.md) … [chapter05_yew/](./chapter05_yew/README.md) |
| **III** | Working with Non-Web Hosts | 6～8 | **L3** | [chapter06_nonweb_hosts/](./chapter06_nonweb_hosts/README.md) … [chapter08_waros/](./chapter08_waros/README.md) |

附录：[appendix/](./appendix/README.md)（**L3** · Serverless / 安全）

---

## 笔记结构

- **README** = 章索引  
- **每小节一个 `.md`** = 笔记本体（原书要点 · 源码/ WAT · 我的笔记 · 相关）  
- 批量生成骨架：`python scripts/scaffold-wasm-rust-notes.py`

**当前进度**：📄 全书骨架已建，正文待刷书填写。

---

## 相关

- 原书：[PragProg khrust](https://pragprog.com/titles/khrust/programming-webassembly-with-rust/)
- 仓库总索引 → [`README.md`](../README.md)
