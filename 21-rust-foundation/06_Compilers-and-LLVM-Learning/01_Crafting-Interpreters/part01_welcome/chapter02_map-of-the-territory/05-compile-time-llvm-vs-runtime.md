# 第 2 章 · 编译期 LLVM vs 运行期 Runtime（彻底分开）

← [04 Rust/HFT 编译流水线](./04-rust-hft-编译流水线对照.md) · [§2.1 流水线各站](./01-the-parts-of-a-language.md) · [04 Learn LLVM 17](../../../04_Learn-LLVM-17/README.md)

---

## 结论（先给你）

> **LLVM 是编译工具链里的翻译官**（程序没跑起来才工作）；**Runtime 是程序被 OS 加载后才启动的底层管家**（调度、GC、async 任务等）。  
> Runtime **源码**也要经 LLVM 编译成机器码，**打包进二进制**；LLVM **不提供**运行能力，IR **不会**交给 Runtime。

---

## 一、编译期（静态 · 程序未启动 · LLVM 全程翻译）

**时间线**：你敲 `cargo build` / `rustc` → 进程退出前 — **CPU 还没跑你的业务逻辑**。

```text
源码
  → 编译器前端（rustc 等）
  → 自定义 IR / MIR / LLVM IR
  → LLVM 接收全部 IR
       ├─ 你的业务代码
       ├─ Tokio / std / 所有依赖库
       └─ （Go 则含 language runtime 源码）
  → LLVM 优化 + 后端代码生成
  → 统一翻译成机器码
  → 链接 → 单个可执行二进制（ELF / PE 等）
```

| 项 | 说明 |
|----|------|
| **LLVM 做什么** | 读 IR → 优化 Pass → 目标 CPU 机器码 |
| **LLVM 不做什么** | 不调度 goroutine、不 poll IO、不管理 async 任务 |
| **IR 命运** | **编译中间产物**；链接完成后消失，**不会**进入运行期 |
| **产物** | 磁盘上的 `.exe` / 无扩展名二进制 — 仍是**静态数据**，尚未「运行」 |

→ 流水线各站：[04 Rust/HFT 编译流水线](./04-rust-hft-编译流水线对照.md) · [04 Learn LLVM 17](../../../04_Learn-LLVM-17/README.md)

---

## 二、运行期（动态 · OS 加载后 · Runtime 才存在）

**时间线**：用户 `./my_app` → OS 把二进制映射进内存 → CPU 从入口 `_start` / `main` 开始执行。

```text
OS 加载二进制到内存
  → CPU 执行机器码（含提前编好的 runtime 片段）
  → Runtime 层开始工作（若存在）
       ├─ 调度执行上下文
       ├─ 内存管理（GC / 分配器）
       ├─ IO 多路复用 / 线程池
       └─ 系统调用封装
  → 你的业务逻辑 + runtime 协同 → 程序输出结果
```

### 两类 Runtime 对照

| | **语言级 Runtime** | **库级 Runtime** |
|---|-------------------|-------------------|
| **例子** | Go runtime、JVM、.NET CLR | Rust **Tokio**、async-std |
| **绑定** | 编译器/工具链原生绑定 | 第三方 crate，**手动** `Cargo.toml` 引入 |
| **进程内** | 通常**唯一**、自动启用 | 可多实例、可替换、可不引入 |
| **谁编译它** | 其**源码**同样经 LLVM（或 JVM 字节码路线）→ 机器码 | 同上 — Tokio 就是普通 Rust 库，编译进二进制 |
| **Rust 默认** | **无**内置语言 runtime | 需要 async 时才选 Tokio 等 |

### Go vs Rust（运行期视角）

| | **Go** | **Rust** |
|---|--------|----------|
| Runtime | **内置** — goroutine 调度、GC、栈管理 | **无**语言级 runtime |
| 并发模型 | runtime 管 M:N 调度 | 原生线程 + 可选 **Tokio**（库级） |
| 二进制里有什么 | 业务代码 + **Go runtime 机器码** | 业务代码 + std（极薄）+ 可选 Tokio 机器码 |
| `main` 之前 | runtime 初始化已嵌入启动链 | `_start` → 可选 std init → `main` |

```rust
// Tokio：库级 runtime — 仍是「一段编译好的机器码」
#[tokio::main]
async fn main() {
    // tokio::runtime::Runtime::block_on 等在运行期调度 Future
}
```

→ Async 深入：[RFR 第 8 章](../../../../02-RFR/Chapter-08-Asynchronous-Programming/README.md)

---

## 三、关键区分（解决常见疑惑）

### 1. LLVM ≠ Runtime

| | **LLVM** | **Runtime** |
|---|----------|-------------|
| **阶段** | **编译期** | **运行期** |
| **角色** | 翻译 IR → 机器码 | 程序活着时的调度/内存/IO 管家 |
| **何时存在** | `cargo build` 时 | `./app` 被 OS 加载后 |
| **类比** | 印刷厂把书印成纸 | 读者打开书并阅读 |

### 2. Runtime 本身也要经 LLVM 编译

Tokio、Go runtime、JVM 的 native 部分 — **全是源码 → IR → 机器码**，和你的业务代码**同一套编译链路**。  
不是「LLVM 提供 runtime 能力」，而是「runtime **被编译进**可执行文件」。

### 3. IR 不会直接交给 Runtime

```text
编译期：源码 ──→ IR ──→ 机器码 ──→ 二进制文件
运行期：二进制 ──→ OS 加载 ──→ CPU 执行机器码
              ↑
         IR 已不存在
```

Runtime 读的是**内存里的机器指令**，不是 LLVM IR 文件。

### 4. 两类 Runtime 再强调

- **语言级**：Go/Java — 不写 `import` 也有；进程启动即启用  
- **库级**：Rust Tokio — 显式依赖；不用 async 可以零 Tokio 代码

---

## 四、与 clox / jlox 的对照

| 体系 | 「编译期」 | 「运行期」 |
|------|------------|------------|
| **clox** | 前端编译为**字节码 Chunk**（clox 的 IR） | **VM** 解释字节码 + **手动 GC runtime** |
| **jlox** | 解析建 AST（极简「编译」） | **树遍历**直接执行 AST |
| **Rust + LLVM** | 前端 + **LLVM IR** + 优化 +  codegen | **机器码** + 可选 Tokio/std runtime |
| **Go** | gc 编译器 + LLVM/自有后端 | **内置 runtime** 机器码 |

clox 的 VM 相当于 **字节码路线的 runtime**；Rust 默认**没有**这类 VM，CPU 直接跑机器码。

→ [03-a-map-of-the-territory.md](./03-a-map-of-the-territory.md)

---

## 五、极简一句话串流程

```text
源码 → IR → LLVM 把「业务 + 所有库（含 runtime 源码）」译成机器码 → 打包成二进制
     → OS 加载 → 内置运行时代码启动 → 调度执行 → 结果
```

---

## 六、易混区分

| 易混 | 纠正 |
|------|------|
| 「用了 LLVM 就有 runtime」 | LLVM 只负责**编译**；runtime 是**跑起来**的代码 |
| 「Tokio 是编译器自带的」 | **库级**；`cargo add tokio` 才进二进制 |
| 「IR 文件程序运行时会读」 | IR 是**编译中间态**；运行期只有机器码 |
| 「Rust 没有 runtime」 | 无**语言级** GC runtime；有 **std / alloc / panic** 等薄层 + 可选 Tokio |
| 「Go runtime 和 LLVM 是一家的」 | Go runtime 是 **Go 源码**；经编译器后端（常含 LLVM 类优化）变成机器码 |

---

## 七、速记

1. **编译期 = LLVM 翻译；运行期 = OS 加载 + runtime 调度。**  
2. **Runtime 源码也会被 LLVM 编译，打进二进制 — 不是 LLVM 在运行时服务你。**  
3. **Go = 语言级 runtime；Rust = 无内置，Tokio = 库级可选。**

---

## 对照阅读

- 上山/下山流水线 → [04 Rust/HFT 编译流水线](./04-rust-hft-编译流水线对照.md)
- LLVM IR / Pass → [04 Learn LLVM 17](../../../04_Learn-LLVM-17/README.md)
- Rust 工具链 → [RFR 第 13 章](../../../../02-RFR/Chapter-13-Tooling/13-工具链-Tooling-深度解析.md)
- clox VM + GC runtime → [ch15 VM](../../part03_clox/chapter15_a-virtual-machine/) · [ch26 GC](../../part03_clox/chapter26_garbage-collection/)

---

## 自测

- [ ] 用一句话说明 LLVM 和 Runtime 分别在哪一阶段工作  
- [ ] 解释 Tokio 源码在编译期和运行期各扮演什么角色  
- [ ] 说明 IR 会不会被 `./my_app` 读取  
- [ ] 对比 Go 内置 runtime 与 Rust 库级 Tokio 的引入方式
