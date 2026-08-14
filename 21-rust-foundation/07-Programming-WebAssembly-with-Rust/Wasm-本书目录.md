# Programming WebAssembly with Rust — 本书目录

> **Programming WebAssembly with Rust** · Kevin Hoffman · Pragmatic Bookshelf, 2019  
> 仓库：[07-Programming-WebAssembly-with-Rust/](./README.md)

**全书主旨**：利用 Rust 的**安全性、性能与表达力**构建 **WebAssembly (Wasm)** 应用 — 从底层 **WAT/Wast** 栈式虚拟机与**线性内存**，到 **wasm-bindgen** 实现 Rust 与 **JavaScript** 互操作，经 **Checkers**、**Roguelike**、**Yew 聊天**等实战，延伸到浏览器外宿主（控制台、**ARM/树莓派**、**WARoS 机器人**）。强调**位标志**、内存布局等对性能的影响，并展望 Wasm 对 Web 与未来应用形态的意义。

**笔记状态**：📄 骨架 · 三层练手目录已建 → [三层学习架构.md](./三层学习架构.md)

---

## 三层练手 ↔ 原书对照

| 层 | 练手目录 | 原书章节 |
|:---:|----------|----------|
| **1** | [layer01_rust-llvm-to-wasm/](./layer01_rust-llvm-to-wasm/README.md) | 第 **1** 章 |
| **2** | [layer02_orderbook-query-wasm/](./layer02_orderbook-query-wasm/README.md) | 第 **2～4** 章 |
| **3** | [layer03_quant-ma-strategy/](./layer03_quant-ma-strategy/README.md) | 第 **5～8** 章 + 附录 |

---

## Part I · 建立基础 (Building a Foundation)

### 第 1 章 · WebAssembly 基础 (WebAssembly Fundamentals)

**章导读**：介绍 WebAssembly、理解 WebAssembly 架构（栈式 VM、线性内存）、构建首个 WebAssembly 应用程序。

| 节 | 原书主题 | 要点 | 笔记 |
|:---:|----------|------|------|
| **1.1** | Introducing WebAssembly | Wasm 是什么/不是什么；模块与宿主边界 | [1.1-introducing-wasm.md](./chapter01_wasm_fundamentals/1.1-introducing-wasm.md) |
| **1.2** | Understanding WebAssembly Architecture | **栈式虚拟机**、数据类型、控制流、**线性内存** | [1.2-wasm-architecture.md](./chapter01_wasm_fundamentals/1.2-wasm-architecture.md) |
| **1.3** | Building a WebAssembly Application | 用跨平台工具链手写/编译首个模块 | [1.3-building-wasm-app.md](./chapter01_wasm_fundamentals/1.3-building-wasm-app.md) |
| **1.4** | Wrapping Up | 本章小结 | [1.4-wrap-up.md](./chapter01_wasm_fundamentals/1.4-wrap-up.md) |

↔ **Layer 1** · [Crafting Interpreters · VM](../../06_Compilers-and-LLVM-Learning/01_Crafting-Interpreters/part01_welcome/chapter02_map-of-the-territory/01-7-virtual-machine.md)

### 第 2 章 · 构建 WebAssembly 跳棋 (Building WebAssembly Checkers)

**章导读**：应对 Wasm **数据结构限制**、实现跳棋**游戏规则**与**玩家移动**逻辑、**测试** Wasm 跳棋。

| 节 | 原书主题 | 要点 | 笔记 |
|:---:|----------|------|------|
| **2.1** | Playing Checkers, the Board Game | 棋盘规则与状态 | [2.1-checkers-board-game.md](./chapter02_wasm_checkers/2.1-checkers-board-game.md) |
| **2.2** | Coping with Data Structure Constraints | Wasm 内存下的数据结构 / **位标志** | [2.2-data-structure-constraints.md](./chapter02_wasm_checkers/2.2-data-structure-constraints.md) |
| **2.3** | Implementing Game Rules | 规则引擎 | [2.3-game-rules.md](./chapter02_wasm_checkers/2.3-game-rules.md) |
| **2.4** | Moving Players | 走子与状态转移 | [2.4-moving-players.md](./chapter02_wasm_checkers/2.4-moving-players.md) |
| **2.5** | Testing Wasm Checkers | 测试 Wasm 模块 | [2.5-testing-wasm-checkers.md](./chapter02_wasm_checkers/2.5-testing-wasm-checkers.md) |
| **2.6** | Wrapping Up | 本章小结 | [2.6-wrap-up.md](./chapter02_wasm_checkers/2.6-wrap-up.md) |

↔ **Layer 1→2** · 位布局 → [订单簿档位](./layer02_orderbook-query-wasm/README.md)

---

## Part II · 与 JavaScript 交互 (Interacting with JavaScript)

### 第 3 章 · 使用 Rust 涉足 WebAssembly (Wading into WebAssembly with Rust)

**章导读**：介绍与安装 Rust、构建 Rust 版 Hello Wasm、Rust 跳棋引擎与 **Wasm 接口**、在 JavaScript 中运行游玩。

| 节 | 原书主题 | 要点 | 笔记 |
|:---:|----------|------|------|
| **3.1** | Introducing Rust | 为何选 Rust 作 Wasm 源语言 | [3.1-introducing-rust.md](./chapter03_rust_wasm/3.1-introducing-rust.md) |
| **3.2** | Installing Rust | 工具链安装 | [3.2-installing-rust.md](./chapter03_rust_wasm/3.2-installing-rust.md) |
| **3.3** | Building Hello WebAssembly in Rust | 首个 Rust → Wasm | [3.3-hello-wasm-rust.md](./chapter03_rust_wasm/3.3-hello-wasm-rust.md) |
| **3.4** | Creating Rusty Checkers | Rust 版跳棋 | [3.4-rusty-checkers.md](./chapter03_rust_wasm/3.4-rusty-checkers.md) |
| **3.5** | Coding the Rusty Checkers WebAssembly Interface | 导出接口 / FFI 边界 | [3.5-rusty-checkers-ffi.md](./chapter03_rust_wasm/3.5-rusty-checkers-ffi.md) |
| **3.6** | Playing Rusty Checkers in JavaScript | 浏览器中调用 | [3.6-rusty-checkers-js.md](./chapter03_rust_wasm/3.6-rusty-checkers-js.md) |
| **3.7** | Wrapping Up | 本章小结 | [3.7-wrap-up.md](./chapter03_rust_wasm/3.7-wrap-up.md) |

↔ **Layer 2**

### 第 4 章 · 将 WebAssembly 与 JavaScript 集成 (Integrating WebAssembly with JavaScript)

**章导读**：更好的 Hello World、基于 Wasm 的 **Rogue** 游戏、深入实验与集成。

| 节 | 原书主题 | 要点 | 笔记 |
|:---:|----------|------|------|
| **4.1** | Creating a Better Hello, World | **wasm-bindgen** 入门 | [4.1-better-hello-world.md](./chapter04_js_integration/4.1-better-hello-world.md) |
| **4.2** | Building the Rogue WebAssembly Game | **Roguelike** 实战 | [4.2-rogue-wasm-game.md](./chapter04_js_integration/4.2-rogue-wasm-game.md) |
| **4.3** | Experimenting Further | 扩展实验 | [4.3-experimenting-further.md](./chapter04_js_integration/4.3-experimenting-further.md) |
| **4.4** | Wrapping Up | 本章小结 | [4.4-wrap-up.md](./chapter04_js_integration/4.4-wrap-up.md) |

↔ **Layer 2** · Go 订单簿 HTTP + bindgen

### 第 5 章 · 使用 Yew 进行高级 JavaScript 集成 (Advanced JavaScript Integration with Yew)

**章导读**：Yew 框架入门、构建完整**实时聊天**应用。

| 节 | 原书主题 | 要点 | 笔记 |
|:---:|----------|------|------|
| **5.1** | Getting Started with Yew | **Yew** 框架 | [5.1-getting-started-yew.md](./chapter05_yew/5.1-getting-started-yew.md) |
| **5.2** | Building a Live Chat Application | 实时聊天应用 | [5.2-live-chat-app.md](./chapter05_yew/5.2-live-chat-app.md) |
| **5.3** | Wrapping Up | 本章小结 | [5.3-wrap-up.md](./chapter05_yew/5.3-wrap-up.md) |

↔ **Layer 3** · 聊天 UI → **回测仪表盘**

---

## Part III · 使用非 Web 宿主 (Working with Non-Web Hosts)

### 第 6 章 · 在浏览器外部托管模块 (Hosting Modules Outside the Browser)

**章导读**：如何成为合格宿主、用 Rust **解释/执行** Wasm、构建**控制台**跳棋宿主。

| 节 | 原书主题 | 要点 | 笔记 |
|:---:|----------|------|------|
| **6.1** | How to Be a Good Host | 宿主职责与 API | [6.1-good-host.md](./chapter06_nonweb_hosts/6.1-good-host.md) |
| **6.2** | Interpreting WebAssembly Modules with Rust | 用 Rust 解释/加载 Wasm | [6.2-interpreting-wasm-rust.md](./chapter06_nonweb_hosts/6.2-interpreting-wasm-rust.md) |
| **6.3** | Building a Console Host Checkers Player | 控制台宿主跳棋 | [6.3-console-checkers.md](./chapter06_nonweb_hosts/6.3-console-checkers.md) |
| **6.4** | Wrapping Up | 本章小结 | [6.4-wrap-up.md](./chapter06_nonweb_hosts/6.4-wrap-up.md) |

↔ **Layer 3** · wasmtime 嵌入均线策略

### 第 7 章 · 探索 WebAssembly 物联网 (Exploring the Internet of WebAssembly Things)

**章导读**：通用**指示器模块**、为 **ARM** 构建 Rust 应用、在**树莓派**上托管指示器模块。

| 节 | 原书主题 | 要点 | 笔记 |
|:---:|----------|------|------|
| **7.1** | Overview of the Generic Indicator Module | 通用指示器模块 | [7.1-indicator-overview.md](./chapter07_iot/7.1-indicator-overview.md) |
| **7.2** | Creating Indicator Modules | 编写指示器 Wasm | [7.2-creating-indicators.md](./chapter07_iot/7.2-creating-indicators.md) |
| **7.3** | Building Rust Applications for ARM Devices | **ARM** 交叉编译 | [7.3-rust-arm.md](./chapter07_iot/7.3-rust-arm.md) |
| **7.4** | Hosting Indicator Modules on a Raspberry Pi | **树莓派** 部署 | [7.4-raspberry-pi-host.md](./chapter07_iot/7.4-raspberry-pi-host.md) |
| **7.5** | Hardware Shopping List | 硬件清单 | [7.5-hardware-list.md](./chapter07_iot/7.5-hardware-list.md) |
| **7.6** | Endless Possibilities | 扩展场景 | [7.6-endless-possibilities.md](./chapter07_iot/7.6-endless-possibilities.md) |
| **7.7** | Wrapping Up | 本章小结 | [7.7-wrap-up.md](./chapter07_iot/7.7-wrap-up.md) |

↔ **Layer 3** · 边缘节点 · [ER Item 33 no_std](../../01-ER/Chapter-06-Beyond-Standard-Rust/Item-33-no-std/README.md)

### 第 8 章 · 构建 WARoS——WebAssembly 机器人系统 (Building WARoS)

**章导读**：致敬 **Crobots**、设计 WARoS API、构建**比赛引擎**、创建 Wasm **机器人**、**云端**机器人部署。

| 节 | 原书主题 | 要点 | 笔记 |
|:---:|----------|------|------|
| **8.1** | An Homage to Crobots | Crobots 背景 | [8.1-homage-crobots.md](./chapter08_waros/8.1-homage-crobots.md) |
| **8.2** | Designing the WARoS API | 机器人对战 API | [8.2-waros-api.md](./chapter08_waros/8.2-waros-api.md) |
| **8.3** | Building the WARoS Match Engine | 对战引擎 | [8.3-match-engine.md](./chapter08_waros/8.3-match-engine.md) |
| **8.4** | Creating WebAssembly Robots | Wasm 机器人模块 | [8.4-wasm-robots.md](./chapter08_waros/8.4-wasm-robots.md) |
| **8.5** | Robots in the Cloud | 云端托管 | [8.5-robots-cloud.md](./chapter08_waros/8.5-robots-cloud.md) |
| **8.6** | Wrapping Up | 本章小结 | [8.6-wrap-up.md](./chapter08_waros/8.6-wrap-up.md) |
| **8.7** | Conclusion | **全书结论** | [8.7-conclusion.md](./chapter08_waros/8.7-conclusion.md) |

↔ **Layer 3** · 策略模块 = 机器人 · 引擎 = tick 调度

---

## 附录

| 节 | 原书主题 | 章导读 | 笔记 |
|:---:|----------|--------|------|
| **A1** | WebAssembly and Serverless | Serverless 基础 · Wasm 结合 · 云端应用 · **OpenFaaS** | [A1-serverless.md](./appendix/A1-serverless.md) |
| **A2** | Securing WebAssembly Modules | 安全顾虑 · 浏览器攻击 · **签名与加密** | [A2-security.md](./appendix/A2-security.md) |

↔ **Layer 3** · 实盘策略模块签名（必做）

---

## 章节文件夹

```text
07-Programming-WebAssembly-with-Rust/
├── 三层学习架构.md              ← 三层练手 ↔ 原书映射
├── Wasm-本书目录.md
├── README.md
├── 学习路径与知识链.md
├── layer01_rust-llvm-to-wasm/   ← Layer 1 · 编译链路
├── layer02_orderbook-query-wasm/← Layer 2 · Go 订单簿
├── layer03_quant-ma-strategy/   ← Layer 3 · 均线策略
├── chapter01_wasm_fundamentals/ ← 第 1 章
├── chapter02_wasm_checkers/     ← 第 2 章
├── chapter03_rust_wasm/         ← 第 3 章
├── chapter04_js_integration/    ← 第 4 章
├── chapter05_yew/               ← 第 5 章
├── chapter06_nonweb_hosts/      ← 第 6 章
├── chapter07_iot/               ← 第 7 章
├── chapter08_waros/             ← 第 8 章
└── appendix/
```
