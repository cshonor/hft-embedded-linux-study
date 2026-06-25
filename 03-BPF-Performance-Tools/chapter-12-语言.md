# Ch 12 语言 · Languages

> **BPF Performance Tools** · Brendan Gregg · **跳过 ⚪**

> 本章定位：**按语言类型决定怎么挂 BPF** — 追踪前先问：底层是 **编译型 / JIT / 解释型**？符号从哪来？栈怎么 walk？参数怎么读？  
> **HFT：** 热路径 **C/C++/Rust** 必读 **帧指针 + 符号** 两节；共置 **Java/Go 辅助服务** 按需；**Go `uretprobe` 禁用**。与 [Ch 2 § 栈遍历](./chapter-02-技术背景.md)、[Ch 13 应用案例](./chapter-13-应用程序.md) 衔接。  
> **上一章：** [chapter-11-安全.md](./chapter-11-安全.md) · **下一章：** [chapter-13-应用程序.md](./chapter-13-应用程序.md)

---

## 1. 本章核心问题

| 语言类型 | 机器上跑的是什么 | BPF 主要手段 |
|----------|------------------|--------------|
| **编译型** C/C++/Rust/Go | ELF 机器码 | `uprobe`/`uretprobe`、**USDT** |
| **JIT** Java、Node.js | 运行时生成的代码 | **perf-map**、JVM/ V8 特殊选项 |
| **解释型** Bash、Python、Ruby | 解释器内部 C 函数 | 追 **解释器** + 理想情况 **USDT** |

```
要追语言 X
    → X 的 runtime 是什么？
    → 符号在哪（ELF / perf.map / 无）？
    → 栈 walk 需要 frame pointer 吗？
    → 探针频率是否可承受？
```

---

## 2. 编译型语言 (C, C++, Rust, Go…)

### 追踪方式

| 手段 | 说明 |
|------|------|
| **`uprobe` / `uretprobe`** | 二进制任意函数入口/返回 |
| **USDT** | 源码预埋静态探针 — **首选**（稳定、低开销） |

```bash
# uprobe 示例（需符号未 strip）
sudo bpftrace -e 'uprobe:/path/to/bin:my_func { @[ustack] = count(); }'
```

→ USDT 总论：[Ch 2 § USDT](./chapter-02-技术背景.md)

### 符号表 (Symbols)

BPF 用 **`.symtab` / `.dynsym`** 将地址 → 函数名。

| 生产现状 | 对策 |
|----------|------|
| **`strip(1)` 剥离符号** | 发布保留符号分区，或安装 **`debuginfo`/`dbgsym` 包** |
| 仅动态符号 | `readelf -s` / `nm` 确认 |

**HFT：** 策略 **`.so` 发布包** 建议保留 **至少动态符号 + debuginfo 分离包** — 否则 `profile`/`offcputime` 栈为 `[unknown]`。

### 调用栈 (Frame Pointer)

栈 walk 依赖 **帧指针链**（x86-64 `RBP`）— 见 [Ch 2 § 栈遍历](./chapter-02-技术背景.md)。

| 编译选项 | 效果 |
|----------|------|
| **默认 `-fomit-frame-pointer`** | 栈 **断裂** / `[unknown]` |
| **`-fno-omit-frame-pointer`** | 火焰图可用 |
| **`-g` + DWARF** | 更准但更慢（可选） |

```bash
# GCC/Clang
CFLAGS="-fno-omit-frame-pointer -g"
```

**HFT 构建链：** 与 [SysPerf Ch 6 CPU](../02-Systems-Performance-2nd/chapter-06-cpus/) **FPO 讨论同构** — 性能与可观测性 trade-off，热路径可 **USDT 替代高频 uprobe**。

### Golang 特殊陷阱 ⚠️

Go 是编译型，但 **动态栈** + 独特 ABI：

| 做法 | 风险 |
|------|------|
| **`uretprobe` on Go 函数** | **极危险** — 破坏栈，**崩溃/数据损坏** |
| `uprobe` | 相对可行，仍须谨慎、低频 |
| 官方方向 | **USDT**、`runtime/trace`、Go pprof |

**HFT：** 共置 **Go 微服务** 用 **应用内 metrics + pprof** 为主；BPF 仅 **syscall/网络层**（Ch 10）辅助。

### Rust

与 C/C++ 同类：**ELF + frame pointer**；名称 **mangling**（demangle 用 `rustfilt`）。无 GC — 栈行为比 Go 简单。

→ 本仓库：[16-Rust-Quant-Trading-Guide](../16-Rust-Quant-Trading-Guide/)

### C++ 注意点

| 问题 | 说明 |
|------|------|
| **Name mangling** | `c++filt` / bpftrace 符号 demangle |
| **成员函数** | **`arg0` 常是 `this`** — 实参从 `arg1` 起 |
| **内联** | 热点可能 **无独立符号** — 看编译单元或 LTO 行为 |

---

## 3. JIT 编译型 (Java, Node.js…)

字节码在 **运行时** JIT 为机器码 — 地址 **移动**，符号 **不在 ELF**。

### 挑战

| 问题 | 后果 |
|------|------|
| 方法地址变化 | uprobe 按固定地址会 **失效** |
| 无 ELF 符号 | `profile` 只见 **匿名地址** |
| 频繁重编译 | 旧 map **过期** |

### Java：符号解析

| 组件 | 作用 |
|------|------|
| **`perf-map-agent`** | 注入 JVM，生成 **`/tmp/perf-<PID>.map`** |
| **`jmaps`（Gregg 封装）** | 自动化 map 生成 |

**流程：**

```
启动 Java（见下方 JVM  flags）
    → jmaps / perf-map-agent
    → /tmp/perf-PID.map
    → profile / perf 读 map → 火焰图含 Java 方法名
```

**火焰图前必须 **实时** 生成 map** — JIT 重编译后旧 map 无效。

### Java：调用栈

| JVM 参数 | 必需性 |
|----------|--------|
| **`-XX:+PreserveFramePointer`** | **必须** — 否则 BPF 栈无 Java 帧 |

### Java：USDT 与开销

JVM 内置 USDT（GC、类加载、线程…）：

| 探针类 | 开销 |
|--------|------|
| GC / 线程等 **低频** | 可接受 |
| **`method__entry` 等高频** | 需 `-XX:+ExtendedDTraceProbes` — 书中：**10×+ 惩罚**，**勿生产** |

**HFT：** 共置 **风控/报表 Java** — 用 **JFR/async profiler** 为主；BPF 看 **TCP/ syscall** 层即可。

### Node.js (V8)

| 要点 | 说明 |
|------|------|
| 引擎 | **V8 JIT** — 同 Java 需 **`/tmp/perf-PID.map`** |
| USDT | 需 **`--with-dtrace` 源码编译** Node |
| 场景 | 辅助 Web/脚本服务 — 非 tick 热路径 |

---

## 4. 解释型语言 (Bash, Python, Ruby…)

**业务代码不直接变成机器码** — CPU 上跑的是 **解释器** 的 C 函数。

### 追踪策略

| 层级 | 做法 |
|------|------|
| **错误** | `uprobe` 追 Python 用户函数名（不存在于 ELF） |
| **可行** | uprobe 追 **`python`/`bash` 内部 C 函数** |
| **最佳** | 解释器源码预埋 **USDT** |

### Bash 示例（书中思路）

追踪 bash 内部如 **`execute_function` / `execute_builtin`** — 得脚本层函数名与延迟。

| 坑 | 说明 |
|----|------|
| 系统 `/bin/bash` 常 **strip** | uprobe 挂内部符号 **失败** |
| 对策 | 带符号 bash 构建，或 USDT |

→ 安全向：`bashreadline` [Ch 11](./chapter-11-安全.md)

### Python / Ruby

| 手段 | 说明 |
|------|------|
| uprobe CPython 内部 | 可行但 **脆弱**（版本绑定） |
| **`python -m pyperf` / `py-spy`** | 往往比 BPF 更省心 |
| USDT | 若发行版提供（少见） |

**HFT：** 研究脚本/运维自动化 — 非策略热路径；用 **专用 profiler** 优先。

---

## 5. 语言类型决策树

```
编译型 (C/C++/Rust)
  ├─ 符号：debuginfo / 勿 strip
  ├─ 栈：-fno-omit-frame-pointer
  └─ 探针：USDT > uprobe >> uretprobe(Go 禁用)

JIT (Java/Node)
  ├─ perf-PID.map + jmaps（实时）
  ├─ Java：-XX:+PreserveFramePointer
  └─ 禁 ExtendedDTraceProbes 高频 method 探针

解释型 (Bash/Python)
  ├─ 追解释器内部 或 USDT
  └─ 生产优先语言自带 profiler
```

---

## 6. 与全书工具的关系

| 工具 | 语言依赖 |
|------|----------|
| `profile` / `offcputime` | **ustack 需要 FP + 符号** |
| `memleak` | uprobe `malloc` — C/C++ 常用 |
| `bashreadline` | Bash 专用 |
| Java MySQL 等 | [Ch 13](./chapter-13-应用程序.md) 叠加上层 |

---

## 7. HFT 读者 Takeaway

1. **策略核心（C++/Rust）** — 构建链：**frame pointer + debuginfo**；否则 Ch 6 `profile` 半盲。
2. **USDT > uprobe** — 高频路径预埋静态探针；与 [Ch 2](./chapter-02-技术背景.md) 原则一致。
3. **Go：禁止 uretprobe** — 共置 Go 服务用 **pprof/trace**，BPF 只看内核边界。
4. **Java/Node 辅助服务** — `PreserveFramePointer` + perf-map；**勿开** ExtendedDTrace 级 method 探针。
5. **Python/Bash** — 运维脚本层；BPF 追 bash 内部仅 **取证/调试**。
6. **C++ `this`** — 读 uprobe 参数时 **arg0 偏移**。
7. 下一章 **MySQL 等** 把语言 + 资源域工具 **组合成应用方法论** → [Ch 13](./chapter-13-应用程序.md)。

---

## 相关章节

- 上一章：[chapter-11-安全.md](./chapter-11-安全.md)
- 下一章：[chapter-13-应用程序.md](./chapter-13-应用程序.md)
- 栈与 USDT：[chapter-02-技术背景.md](./chapter-02-技术背景.md)
- CPU profile：[chapter-06-CPU.md](./chapter-06-CPU.md)
- CSAPP 编译：[chapter-05-optimizing-performance](../01-CSAPP-3rd/chapter-05-optimizing-performance/)
- Rust 工程：[16-Rust-Quant-Trading-Guide](../16-Rust-Quant-Trading-Guide/)
