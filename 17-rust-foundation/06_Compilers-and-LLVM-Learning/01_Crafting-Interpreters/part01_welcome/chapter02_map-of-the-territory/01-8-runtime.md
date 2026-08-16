# §2.1.8 运行时（Runtime）
← [§2.1 hub](./01-the-parts-of-a-language.md) · 上一节 · [§2.1.7](./01-7-virtual-machine.md) · 下一节 · [§2.2 捷径](./02-shortcuts-and-alternate-routes.md)

---


> **原书编号 §2.1.8** · 程序**真正执行起来**后，语言实现必须提供的**服务层**（常与 VM 重叠，但概念更广）。

| 项目 | 说明 |
|------|------|
| **何时** | OS `exec` / 用户启动 `java`·`python`·`./clox` **之后** |
| **输入** | 已加载的机器码或 bytecode + VM 状态 |
| **输出** | 程序行为（IO、计算、副作用） |
| **核心动作** | 内存管理、动态语义、内置 API、调度 |
| **代码来源** | **同样经编译期 codegen** 链入二进制（或随 VM 进程常驻） |

```text
编译期:  runtime 源码 ──LLVM/gcc──→ 机器码 ──链接──→ 可执行文件
运行期:  OS 加载 ──→ runtime 初始化 ──→ 你的 main/脚本逻辑
```

→ 对照 [05 编译期 LLVM vs Runtime](./05-compile-time-llvm-vs-runtime.md)

---

#### 例子 1 · 启动链：谁最先跑

**Rust 原生（极简）：**

```text
OS loader
  → _start (crt0)
  → __libc_start_main / rust runtime 薄层
  → main()
```

**clox REPL：**

```text
./clox
  → C runtime (libc)
  → main() 初始化 VM、注册 native 函数
  → interpret(chunk)  进入 VM 循环
```

**Java：**

```text
java MyApp
  → JVM 进程启动（HotSpot runtime 已是机器码）
  → 类加载 → bytecode 解释/JIT
  → MyApp.main()
```

---

#### 例子 2 · Runtime 提供什么（对照表）

| 服务 | Lox/clox | Rust | Go |
|------|----------|------|-----|
| **内存** | GC Mark-Sweep ch26 | 所有权 / `alloc` | **内置 GC** |
| **动态类型** | 运行期 tag + 检查 ch18 | 编译期（无） | 反射 / interface |
| **错误** | Runtime error + 栈回溯 | **`panic!` + unwinding** | panic + defer |
| **并发调度** | 无（单线程 VM） | 线程 + 可选 **Tokio** | **goroutine 调度器** |
| **内置 IO** | `print` native fn | **`std::io`** | `fmt` / net |

---

#### 例子 3 · clox 运行期：动态类型检查

**源码：**

```lox
print "hi" - 3;
```

| 阶段 | 结果 |
|------|------|
| Scan / Parse / 编译 bytecode | ✅ 全过 |
| VM 执行 `OP_SUBTRACT` | ❌ **Runtime error**: Operands must be numbers |

**实现**：`OP_ADD` / `OP_SUBTRACT` 前 **`checkNumber()`** — 看栈顶 Value 的 **tag**。

```c
// 概念
typedef struct {
  ValueType type;   // NUMBER, STRING, NIL, ...
  union { double number; Obj* obj; } as;
} Value;
```

→ [ch18 Types of Values](../../part03_clox/chapter18_types-of-values/00-overview.md)

---

#### 例子 4 · GC：Runtime 的核心服务之一

**源码：**

```lox
while (true) {
  var s = "temporary";
}
// 无限分配字符串 → 必须回收
```

**ch19 朴素版**：进程退出才 `freeObjects` — 跑不久就 OOM。  
**ch26 GC**：

```text
Mark:  从根（栈、全局、CallFrame）出发标记可达 Obj
Sweep: 未标记 Obj 从 vm.objects 链表移除并 free
```

| 根集合示例 | 说明 |
|------------|------|
| **Value stack** | 仍被 Lox 代码引用的对象 |
| **globals 表** | 全局变量名 → Obj* |
| **CallFrame** | 闭包、Upvalue 链 |

→ [ch26 GC overview](../../part03_clox/chapter26_garbage-collection/00-overview.md)

---

#### 例子 5 · jlox：Runtime 借宿 JVM

```text
jlox Interpreter (Java)
  ├── 你的 Lox 语义逻辑
  └── 依赖 JVM runtime:
        ├── GC（Java 对象、Lox 包装）
        ├── 异常栈
        └── 线程（若扩展）
```

**对比 clox**：**自己实现** GC、Value 表示、错误 — Runtime **更薄、更可控**，适合嵌入式/学习。

---

#### 例子 6 · Rust：无 GC 的「Runtime」长什么样

**默认 std：**

| 组件 | 作用 |
|------|------|
| **`alloc`** | 堆分配器（可换 jemalloc） |
| **panic runtime** | 栈展开、打印 backtrace |
| **`std::rt`** | 启动/退出钩子 |
| **I/O / 线程** | OS 抽象 |

**`no_std`（HFT 常见）：**

```text
剥离: 堆默认禁用、无 std panic 格式化、无内置 IO
保留: 可选自定义 allocator、裸 panic = abort
```

→ [RFR 第 1 章内存区域](../../../../02-RFR/Chapter-01-Foundations/03-memory-regions.md)

**Tokio（库级 async runtime，非语言内置）：**

```rust
#[tokio::main]  // 运行期：Runtime::block_on 驱动 Future
async fn main() { /* IO 多路复用、任务调度 */ }
```

---

#### 例子 7 · Go vs Rust：语言级 vs 库级 Runtime

| | **Go** | **Rust + Tokio** |
|---|--------|------------------|
| 绑定 | **每个** `.exe` **嵌入** Go runtime | 不用 async 可 **零** Tokio |
| 调度 | M:N goroutine **内置** | Tokio 线程池 + task queue |
| 编译 | runtime 源码 → 机器码进二进制 | Tokio crate → 机器码进二进制 |
| 多实例 | 进程内通常 **一个** Go runtime | 可建多个 `Runtime::new()` |

**共同点**：runtime **源码都先在编译期变成机器码**，不是 LLVM 在运行期提供服务。

---

#### 例子 8 · Runtime vs VM vs 编译期（易错汇总）

| 易混 | 纠正 |
|------|------|
| Runtime = LLVM | **LLVM 仅编译期**；Runtime 跑在 OS 加载后 |
| 只有 GC 语言有 runtime | Rust 有 **panic/alloc**；C 有 **crt0/libc** |
| VM 包含全部 runtime | VM 负责 **执行 bytecode**；GC/类型检查是 runtime **服务**，常写在同一 C 文件里 |
| 动态类型在 Parser 检查 | **运行期** `OP_ADD` 才知两侧是否 number |
| `print` 是 opcode 全部 | clox：`OP_PRINT` 调 **native C 函数** — 跨出 VM 进 libc runtime |

---

**一句话**：Runtime = 程序活着时的**管家**（内存、类型、调度、内置 API）；VM 是管家里的**「执行 bytecode 的工人」**（若走字节码路线）。

**本书对应**：jlox → **JVM** · clox → **ch18～26**（值/GC/对象）· Rust → **std / Tokio / panic**

→ **Rust/HFT 分层对照**：[04-rust-hft-编译流水线对照.md](./04-rust-hft-编译流水线对照.md) · [05 编译期 LLVM vs Runtime](./05-compile-time-llvm-vs-runtime.md)

---
